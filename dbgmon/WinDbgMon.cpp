#include <exception>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <tchar.h>
#include "WinDbgMon.h"
#include "LogDbg.h"

// ----------------------------------------------------------------------------
//  PROPERTIES OF OBJECTS
// ----------------------------------------------------------------------------
//	NAME		|	DBWinMutex		DBWIN_BUFFER_READY		DBWIN_DATA_READY
// ----------------------------------------------------------------------------
//	TYPE		|	Mutex			Event					Event
//	ACCESS		|	All				All						Sync
//	INIT STATE	|	?				Signaled				Nonsignaled
//	PROPERTY	|	?				Auto-Reset				Auto-Reset
// ----------------------------------------------------------------------------

WinDbgMon::WinDbgMon(Callback onDebugMessage)
{
    OnDebugMessage = onDebugMessage;
}

WinDbgMon::~WinDbgMon()
{
    Stop();
}

DWORD WinDbgMon::Start()
{
    if (m_hDBWinMutex != NULL) {
        return 0;
    }
    else {
        SetLastError(0);

        // Mutex: DBWin
        // ---------------------------------------------------------
        auto DBWIN_MUTEX = _T("DBWinMutex");
        m_hDBWinMutex = ::OpenMutex(
            SYNCHRONIZE,
            FALSE,
            DBWIN_MUTEX
        );

        if (m_hDBWinMutex == NULL) {
            return GetLastError();
        }

        // Event: buffer ready
        // ---------------------------------------------------------
        auto DBWIN_BUFFER_READY = _T("DBWIN_BUFFER_READY");
        m_hEventBufferReady = ::OpenEvent(
            EVENT_ALL_ACCESS,
            FALSE,
            DBWIN_BUFFER_READY
        );

        if (m_hEventBufferReady == NULL) {
            m_hEventBufferReady = ::CreateEvent(
                NULL,
                FALSE,	// auto-reset
                TRUE,	// initial state: signaled
                DBWIN_BUFFER_READY
            );

            if (m_hEventBufferReady == NULL) {
                return GetLastError();
            }
        }

        // Event: data ready
        // ---------------------------------------------------------
        auto DBWIN_DATA_READY = _T("DBWIN_DATA_READY");
        m_hEventDataReady = ::OpenEvent(
            SYNCHRONIZE,
            FALSE,
            DBWIN_DATA_READY
        );

        if (m_hEventDataReady == NULL) {
            m_hEventDataReady = ::CreateEvent(
                NULL,
                FALSE,	// auto-reset
                FALSE,	// initial state: nonsignaled
                DBWIN_DATA_READY
            );

            if (m_hEventDataReady == NULL) {
                return GetLastError();
            }
        }

        // Shared memory
        // ---------------------------------------------------------
        auto DBWIN_BUFFER = _T("DBWIN_BUFFER");
        m_hDBMonBuffer = ::OpenFileMapping(
            FILE_MAP_READ,
            FALSE,
            DBWIN_BUFFER
        );

        if (m_hDBMonBuffer == NULL) {
            m_hDBMonBuffer = ::CreateFileMapping(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                sizeof(struct dbwin_buffer_t),
                DBWIN_BUFFER
            );

            if (m_hDBMonBuffer == NULL) {
                return GetLastError();
            }
        }

        m_pDBBuffer = (struct dbwin_buffer_t *)::MapViewOfFile(
            m_hDBMonBuffer,
            SECTION_MAP_READ,
            0,
            0,
            0
        );

        if (m_pDBBuffer == NULL) {
            return GetLastError();
        }

        // Monitoring thread
        // ---------------------------------------------------------
        m_bWinDebugMonStopped = FALSE;

        m_hMonitorThread = ::CreateThread(
            NULL,
            0,
            MonitorThread,
            this,
            0,
            NULL
        );

        if (m_hMonitorThread == NULL) {
            m_bWinDebugMonStopped = TRUE;
            return GetLastError();
        }

        return 0;
    }
}

void WinDbgMon::Stop()
{
    if (m_hMonitorThread != NULL) {
        m_bWinDebugMonStopped = TRUE;
        ::WaitForSingleObject(m_hMonitorThread, INFINITE);
        m_hMonitorThread = NULL;
    }

    if (m_hDBWinMutex != NULL) {
        CloseHandle(m_hDBWinMutex);
        m_hDBWinMutex = NULL;
    }

    if (m_hDBMonBuffer != NULL) {
        ::UnmapViewOfFile(m_pDBBuffer);
        CloseHandle(m_hDBMonBuffer);
        m_hDBMonBuffer = NULL;
    }

    if (m_hEventBufferReady != NULL) {
        CloseHandle(m_hEventBufferReady);
        m_hEventBufferReady = NULL;
    }

    if (m_hEventDataReady != NULL) {
        CloseHandle(m_hEventDataReady);
        m_hEventDataReady = NULL;
    }

    m_pDBBuffer = NULL;
}

DWORD WinDbgMon::ProcessData()
{
    DWORD ret = 0;

    // wait for data ready
    ret = ::WaitForSingleObject(m_hEventDataReady, TIMEOUT_WIN_DEBUG);

    if (ret == WAIT_OBJECT_0) {
        if (OnDebugMessage != NULL) {
            OnDebugMessage(m_pDBBuffer->dwProcessId, m_pDBBuffer->data);
        }
        else {
            LogDbg::kprintf("%d: %s", m_pDBBuffer->dwProcessId, m_pDBBuffer->data);
        }

        // signal buffer ready
        SetEvent(m_hEventBufferReady);
    }

    return ret;
}

DWORD WINAPI WinDbgMon::MonitorThread(void *pData)
{
    WinDbgMon *_this = (WinDbgMon *)pData;

    if (_this != NULL) {
        while (!_this->m_bWinDebugMonStopped) {
            _this->ProcessData();
        }
    }

    return 0;
}

void WinDbgMon::Wait()
{
    ::WaitForSingleObject(m_hMonitorThread, INFINITE);
}