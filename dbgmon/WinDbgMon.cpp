#include <exception>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <tchar.h>
#include "WinDbgMon.h"

// ----------------------------------------------------------------------------
//  PROPERTIES OF OBJECTS
// ----------------------------------------------------------------------------
//  NAME        |   DBWinMutex      DBWIN_BUFFER_READY      DBWIN_DATA_READY
// ----------------------------------------------------------------------------
//  TYPE        |   Mutex           Event                   Event
//  ACCESS      |   All             All                     Sync
//  INIT STATE  |   ?               Signaled                Nonsignaled
//  PROPERTY    |   ?               Auto-Reset              Auto-Reset
// ----------------------------------------------------------------------------

WinDbgMon::WinDbgMon(Callback onDebugMessage)
{
    OnDebugMessage = onDebugMessage;
}

WinDbgMon::~WinDbgMon()
{
    Stop();
}

bool WinDbgMon::IsRunning()
{
    return m_bRunning;
}

DWORD WinDbgMon::Start()
{
    if (m_hDBWinMutex == NULL) {
        // Mutex: DBWin
        auto DBWIN_MUTEX = _T("DBWinMutex");
        m_hDBWinMutex = ::OpenMutex(SYNCHRONIZE, FALSE, DBWIN_MUTEX);
        if (m_hDBWinMutex == NULL) {
            return -1;
        }

        // Event: buffer ready
        auto DBWIN_BUFFER_READY = _T("DBWIN_BUFFER_READY");
        m_hEventBufferReady = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, DBWIN_BUFFER_READY);
        if (m_hEventBufferReady == NULL) {
            m_hEventBufferReady = ::CreateEvent(
                NULL,
                FALSE,  // auto-reset
                TRUE,   // initial state: signaled
                DBWIN_BUFFER_READY
            );
            if (m_hEventBufferReady == NULL) {
                return -2;
            }
        }

        // Event: data ready
        auto DBWIN_DATA_READY = _T("DBWIN_DATA_READY");
        m_hEventDataReady = ::OpenEvent(SYNCHRONIZE, FALSE, DBWIN_DATA_READY);
        if (m_hEventDataReady == NULL) {
            m_hEventDataReady = ::CreateEvent(
                NULL,
                FALSE,  // auto-reset
                FALSE,  // initial state: nonsignaled
                DBWIN_DATA_READY
            );
            if (m_hEventDataReady == NULL) {
                return -3;
            }
        }

        // Shared memory
        // ---------------------------------------------------------
        auto DBWIN_BUFFER = _T("DBWIN_BUFFER");
        m_hDBMonBuffer = ::OpenFileMapping(FILE_MAP_READ, FALSE, DBWIN_BUFFER);
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
                return -4;
            }
        }

        m_pDBBuffer = (struct dbwin_buffer_t *)::MapViewOfFile(m_hDBMonBuffer, SECTION_MAP_READ, 0, 0, 0);
        if (m_pDBBuffer == NULL) {
            return -5;
        }

        // Start threads
        m_bRunning = TRUE;

        // 1) start monitoring thread
        m_hMonitorThread = ::CreateThread(NULL, 0, MonitorThread, this, 0, NULL);
        if (m_hMonitorThread == NULL) {
            return -6;
        }

        // 2) start logging thread
        // ---------------------------------------------------------
        m_hLoggingThread = ::CreateThread(NULL, 0, LoggingThread, this, 0, NULL);
        if (m_hLoggingThread == NULL) {
            return -7;
        }

        // prompt for cancelation
        ::OutputDebugString(_T("Debug monitor is started."));
        ::OutputDebugString(_T("press CTRL + C to quit."));
        ::OutputDebugString(_T("press CTRL + X to clear."));
    }
    return 0;
}

void WinDbgMon::Stop()
{
    if (this->m_bRunning) {
        m_bRunning = FALSE;

        // wait for termination of threads
        HANDLE threads[] = { m_hMonitorThread, m_hLoggingThread };
        ::WaitForMultipleObjects(sizeof(threads) / sizeof(HANDLE), threads, TRUE, INFINITE);

        // shutdown
        ::UnmapViewOfFile(m_pDBBuffer);
        CloseHandle(m_hDBMonBuffer);
        CloseHandle(m_hEventBufferReady);
        CloseHandle(m_hEventDataReady);
        CloseHandle(m_hDBWinMutex);
        m_hDBWinMutex = NULL;
    }
}

void WinDbgMon::ProcessData()
{
    // wait for data ready
    if (::WaitForSingleObject(m_hEventDataReady, TIMEOUT_WIN_DEBUG) == WAIT_OBJECT_0) {
        if (OnDebugMessage != NULL) {
            OnDebugMessage(m_pDBBuffer->dwProcessId, m_pDBBuffer->data);
        }
        else {
            std::ostringstream oss;
            oss << m_pDBBuffer->dwProcessId << ": " << m_pDBBuffer->data;
            _output_queue.push(oss.str());
        }
        // signal buffer ready
        SetEvent(m_hEventBufferReady);
    }
}

DWORD WINAPI WinDbgMon::MonitorThread(void *pData)
{
    WinDbgMon *_this = (WinDbgMon *)pData;
    if (_this != NULL) {
        while (_this->m_bRunning) {
            _this->ProcessData();
        }
    }
    return 0;
}

DWORD WINAPI WinDbgMon::LoggingThread(void * pData)
{
    WinDbgMon *_this = (WinDbgMon *)pData;
    std::string s;

    while (_this->m_bRunning) {
        if (_this->_output_queue.try_pop(s)) {
            _this->OutputString(s);
        }
        else {
            ::Sleep(10);
        }
    }
    return 0;
}

void WinDbgMon::OutputString(const std::string &str)
{
    auto timestamp = []() {
        char buffer[64];
        time_t t = time(NULL);
        tm tm;
        localtime_s(&tm, &t);
        strftime(buffer, sizeof(buffer) / sizeof(char), "%Y-%m-%d %T", &tm);
        return std::string(buffer);
    };

    std::ostringstream oss;
    oss << "[" << timestamp() << "] " << str;
    if (str.length() > 0 && str.back() != '\n') {
        oss << std::endl;
    }
    std::cout << oss.str();
}
