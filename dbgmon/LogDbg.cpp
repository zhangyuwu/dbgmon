#include "LogDbg.h"
#include <windows.h>
#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <time.h>
#include <sstream>
#include <iostream>

std::string LogDbg::timestamp()
{
    char buffer[64];
    time_t t = time(NULL);
    tm tm;
    localtime_s(&tm, &t);
    strftime(buffer, sizeof(buffer) / sizeof(char), "%Y-%m-%d %T", &tm);
    return buffer;
}

void LogDbg::output(const char * str)
{
    std::ostringstream oss;
    oss << "[" << timestamp() << "] " << str;
    std::cout << oss.str();
    if (oss.str().back() != '\n') {
        std::cout << std::endl;
    }
}

void LogDbg::kprintf(const char * fmt, ...)
{
    char buffer[4096];
    va_list args;
    memset(buffer, 0, sizeof(buffer));
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
    va_end(args);
    output(buffer);
}
