// dbgmon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <vector>
#include <iostream>
#include <signal.h>
#include <conio.h>
#include "WinDbgMon.h"

using namespace std;

WinDbgMon dbgmon;

void on_user_break(int)
{
    std::cout << "User Break" << std::endl;
    dbgmon.Stop();
}

int main()
{
    std::vector<int> signals = {
        SIGINT,
        SIGILL,
        SIGFPE,
        SIGSEGV,
        SIGTERM,
        SIGBREAK,
        SIGABRT
    };

    for (auto &signum : signals) {
        signal(signum, on_user_break);
    }

    auto err = dbgmon.Start();
    if (err != 0) {
        void *msgbuf;
        int count = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&msgbuf,
            0,
            NULL
        );
        cout << "Faied to initialize: " << (TCHAR *)msgbuf << endl;
    }
    else {
        cout << "Debug monitor is started, press CTRL+C to quit." << endl;
        dbgmon.Wait();
    }
    return 0;
}
