#ifndef _WinDbgMon_H_
#define _WinDbgMon_H_

#include <windows.h>
#include <functional>
#include <concurrent_queue.h>

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
    HANDLE m_hLoggingThread = NULL;
    BOOL m_bWinDebugMonStopped = true;
    struct dbwin_buffer_t *m_pDBBuffer = NULL;
    Callback OnDebugMessage;
    concurrency::concurrent_queue<std::string> _output_queue;

private:
    DWORD ProcessData();
    static DWORD WINAPI MonitorThread(void *pData);
    static DWORD WINAPI LoggingThread(void *pData);
    void OutputString(const std::string &str);

public:
    WinDbgMon(Callback onDebugMessage = NULL);
    ~WinDbgMon();
    DWORD Start();
    void Stop();
    void Wait();
};

#endif
