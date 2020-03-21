#include <vector>
#include <iostream>
#include <signal.h>
#include <conio.h>
#include <tchar.h>
#include "WinDbgMon.h"

using namespace std;

WinDbgMon dbgmon;

void on_user_break(int)
{
    std::cout << "User Break" << std::endl;
    dbgmon.Stop();
}

void clear()
{
    COORD topLeft = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}

int main()
{
    TCHAR title[256];
    ::GetConsoleTitle(title, sizeof(title) / sizeof(TCHAR));
    ::SetConsoleTitle(_T("Windows Debug Monitor"));

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
    if (err < 0) {
        void *msgbuf;
        int count = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&msgbuf,
            0,
            NULL
        );
        cout << "Faied to initialize: " << (TCHAR *)msgbuf << endl;
    }

    while (dbgmon.IsRunning()) {
        auto hStdin = ::GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD irec;
        DWORD num;
        ::ReadConsoleInput(hStdin, &irec, 1, &num);
        if (irec.EventType == KEY_EVENT) {
            KEY_EVENT_RECORD * ker = (KEY_EVENT_RECORD *)&irec.Event;
            if (ker->dwControlKeyState & LEFT_CTRL_PRESSED) {
                switch (toupper(ker->wVirtualKeyCode)) {
                case 'X':
                    clear();
                    break;
                }
            }
        }
    }

    ::SetConsoleTitle(title);
    return 0;
}
