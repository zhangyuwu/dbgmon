#include "WinDbgMon.h"
#include <exception>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <atlbase.h>
#include <atlstr.h>

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
    Unintialize();
}

DWORD WinDbgMon::Initialize()
{
    if (m_hDBWinMutex != NULL) {
        return 0;
    }
    else {
        DWORD errorCode = 0;
        BOOL bSuccessful = FALSE;

        SetLastError(0);

        // Mutex: DBWin
        // ---------------------------------------------------------
        CComBSTR DBWinMutex = L"DBWinMutex";
        m_hDBWinMutex = ::OpenMutex(
            MUTEX_ALL_ACCESS,
            FALSE,
            DBWinMutex
        );

        if (m_hDBWinMutex == NULL) {
            errorCode = GetLastError();
            return errorCode;
        }

        // Event: buffer ready
        // ---------------------------------------------------------
        CComBSTR DBWIN_BUFFER_READY = L"DBWIN_BUFFER_READY";
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
                errorCode = GetLastError();
                return errorCode;
            }
        }

        // Event: data ready
        // ---------------------------------------------------------
        CComBSTR DBWIN_DATA_READY = L"DBWIN_DATA_READY";
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
                errorCode = GetLastError();
                return errorCode;
            }
        }

        // Shared memory
        // ---------------------------------------------------------
        CComBSTR DBWIN_BUFFER = L"DBWIN_BUFFER";
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
                errorCode = GetLastError();
                return errorCode;
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
            errorCode = GetLastError();
            return errorCode;
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
            errorCode = GetLastError();
            return errorCode;
        }

        // set monitor thread's priority to highest
        // ---------------------------------------------------------
        bSuccessful = ::SetPriorityClass(
            ::GetCurrentProcess(),
            REALTIME_PRIORITY_CLASS
        );

        bSuccessful = ::SetThreadPriority(
            m_hMonitorThread,
            THREAD_PRIORITY_TIME_CRITICAL
        );

        return errorCode;
    }
}

void WinDbgMon::Unintialize()
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
            std::time_t t = std::time(nullptr);
            std::tm tm;
            localtime_s(&tm, &t);
            std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %T") << "] " << m_pDBBuffer->dwProcessId << ": " << m_pDBBuffer->data << std::endl;
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