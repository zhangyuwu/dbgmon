
#ifndef _LogDbg_H_
#define _LogDbg_H_

#include <string>
#include <concurrent_queue.h>

class LogDbg
{
private:
    concurrency::concurrent_queue<std::string> _queue;
    HANDLE _thread;
    bool _running;

private:
    static std::string timestamp();

public:
    LogDbg();
    ~LogDbg();

    void output(const std::string &str);
    static void output(const char *str);
    static void kprintf(const char * fmt, ...);
    static DWORD worker(void * logDbg);
};

#endif