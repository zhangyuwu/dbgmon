
#include <Windows.h>
#include <conio.h>
#include "console.h"

int Console::inkey()
{
    return _kbhit() ? _getch() : 0;
}

int Console::set_foreground(color_t forecolor)
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    SetConsoleTextAttribute(hStdout, csbi.wAttributes & 0xF0 | forecolor);
    return (csbi.wAttributes & 0x0F);
}

int Console::set_background(color_t backcolor)
{
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    SetConsoleTextAttribute(hStdout, csbi.wAttributes & 0x0F | (backcolor << 4));
    return (csbi.wAttributes & 0xF0) >> 4;
}

void Console::fill_console_attr(int attr)
{
    DWORD nWritten = 0;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    FillConsoleOutputAttribute(hStdout, attr, csbi.dwSize.X * csbi.dwSize.Y, { 0, 0 }, &nWritten);
}
