/*
 * It is used to show progress in console.
 *
 *  Created on: Dec 05, 2016
 *      Author: Qingkai
 */

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <assert.h>
#include <string.h>
#include "DyckAA/ProgressBar.h"

namespace DyckAA {

ProgressBar::ProgressBar(std::string Title, ProgressBarStyle Style, float UpdateFrequency) :
            Title(Title), Style(Style), UpdateFrequency(UpdateFrequency) {
#ifdef __linux__
    struct winsize WinSize;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &WinSize);
    WindowWidth = (WinSize.ws_col - 15 - Title.length()) * 0.95;
#elif _WIN32
    HANDLE StdOutHandler = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    GetConsoleScreenBufferInfo(StdOutHandler, ConsoleInfo);
    WindowWidth = (ConsoleInfo.dwSize.x- 15 - Title.length()) * 0.95;
#endif

    if (WindowWidth < 0) {
        WindowWidth = 10;
    }

    ProgerssBuffer = new char[WindowWidth + 1];
    if (Style == PBS_BGCStyle) {
        memset(ProgerssBuffer, 0x00, WindowWidth + 1);
    } else {
        memset(ProgerssBuffer, 32, WindowWidth);
        memset(ProgerssBuffer + WindowWidth, 0x00, 1);
    }
}

void ProgressBar::showProgress(float Percent) {
    if (Percent != 0 && Percent != 1) {
        if (Percent - LastPercent < UpdateFrequency) {
            return;
        }
    }

    LastPercent = Percent;

    float Val = Percent * WindowWidth;
    switch (this->Style) {
    case PBS_NumberStyle:
        printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                Title.c_str(), (int)(Val / WindowWidth * 100));
        fflush(stdout);
        break;
    case PBS_CharacterStyle:
        memset(ProgerssBuffer, '#', Val);
        printf("\033[?25l\033[37m\033[1m%s [%-s] %d%%\033[?25h\033[0m\r",
                Title.c_str(), ProgerssBuffer, (int)(Val / WindowWidth * 100));
        fflush(stdout);
        break;
    case PBS_BGCStyle:
        memset(ProgerssBuffer, 32, Val);
        printf("\033[?25l\033[31m\033[1m%s \033[47m %d%% %s\033[?25h\033[0m\r",
                Title.c_str(), (int)(Val / WindowWidth * 100), ProgerssBuffer);
        fflush(stdout);
        break;
    default:
        assert(false && "Unknown style!");
    }
}

ProgressBar::~ProgressBar() {
    if (ProgerssBuffer) {
        delete[] ProgerssBuffer;
        ProgerssBuffer = nullptr;
    }
}
}
