#pragma once

class Console
{
public:
    typedef enum
    {
        Black = 0,
        DarkBlue = 1,
        DarkGreen = 2,
        DarkCyan = 3,
        DarkRed = 4,
        DarkMagenta = 5,
        DarkYellow = 6,
        Gray = 7,
        DarkGray = 8,
        Blue = 9,
        Green = 10,
        Cyan = 11,
        Red = 12,
        Magenta = 13,
        Yellow = 14,
        White = 15,
    } color_t;

    static int inkey();
    static int set_foreground(color_t forecolor);
    static int set_background(color_t backcolor);
    static void fill_console_attr(int attr);
};
