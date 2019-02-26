// dbgmon.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <vector>
#include <iostream>
#include <signal.h>
#include <conio.h>
#include "WinDbgMon.h"

WinDbgMon dbgmon;

void on_user_break(int)
{
    std::cout << "User Break" << std::endl;
    dbgmon.Unintialize();
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

    dbgmon.Initialize();
    dbgmon.Wait();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
