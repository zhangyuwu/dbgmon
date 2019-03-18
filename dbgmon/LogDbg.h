
#ifndef _LogDbg_H_
#define _LogDbg_H_

#include <string>

class LogDbg
{
private:
    static std::string timestamp();

public:
    static void output(const char *str);
    static void kprintf(const char * fmt, ...);
};

#endif