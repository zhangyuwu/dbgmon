#ifndef _WinDbgMon_H_
#define _WinDbgMon_H_

#include <windows.h>
#include <functional>
#include "LogDbg.h"

class WinDbgMon
{
public:
    typedef std::function<void(int, const char *str)> Callback;

private:
    enum {
        TIMEOUT_WIN_DEBUG = 100,
    };

    struct dbwin_buffer_t
    {
        DWORD dwProcessId;
        char  data[4096 - sizeof(DWORD)];
    };

private:
    HANDLE m_hDBWinMutex = NULL;
    HANDLE m_hDBMonBuffer = NULL;
    HANDLE m_hEventBufferReady = NULL;
    HANDLE m_hEventDataReady = NULL;

    HANDLE m_hMonitorThread = NULL;
    BOOL m_bWinDebugMonStopped = true;
    struct dbwin_buffer_t *m_pDBBuffer = NULL;
    Callback OnDebugMessage;

    LogDbg m_logDbg;

private:
    DWORD ProcessData();
    static DWORD WINAPI MonitorThread(void *pData);

public:
    WinDbgMon(Callback onDebugMessage = NULL);
    ~WinDbgMon();
    DWORD Start();
    void Stop();
    void Wait();
};

#endif
