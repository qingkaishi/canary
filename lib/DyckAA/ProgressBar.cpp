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
    WindowWidth = WinSize.ws_col > 80 ? 80 : WinSize.ws_col - Title.length() - 15;
#elif _WIN32
    HANDLE StdOutHandler = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    GetConsoleScreenBufferInfo(StdOutHandler, ConsoleInfo);
    WindowWidth = ConsoleInfo.dwSize.x > 80 ? 80 : ConsoleInfo.dwSize.x - Title.length() - 15;
#endif

    if (WindowWidth < 15) {
        ProgressBuffer = nullptr;
        return;
    }

    ProgressBuffer = (char*) malloc(sizeof(char) * (WindowWidth + 1));
    if (Style == PBS_BGCStyle) {
        memset(ProgressBuffer, 0x00, WindowWidth + 1);
    } else {
        memset(ProgressBuffer, 32, WindowWidth);
        memset(ProgressBuffer + WindowWidth, 0x00, 1);
    }
}

void ProgressBar::showProgress(float Percent) {
    if (Percent < 0)
        Percent = 0;
    else if (Percent > 1)
        Percent = 1;

    if (Percent > 0 && Percent < 1) {
        if (Percent - LastUpdatePercent < UpdateFrequency) {
            return;
        }
    }

    LastUpdatePercent = Percent;

    // In case the window width is changed at runtime.
    resize();

    float Val = Percent * WindowWidth;
    switch (this->Style) {
    case PBS_NumberStyle:
        printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                Title.c_str(), (int)(Percent * 100));
        break;
    case PBS_CharacterStyle:
        if (ProgressBuffer) {
            memset(ProgressBuffer, '#', Val);
            printf("\033[?25l\033[37m\033[1m%s [%-s] %d%%\033[?25h\033[0m\r",
                    Title.c_str(), ProgressBuffer, (int)(Percent * 100));
        } else {
            printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                    Title.c_str(), (int)(Percent * 100));
        }
        break;
    case PBS_BGCStyle:
        if (ProgressBuffer) {
            memset(ProgressBuffer, 32, Val);
            printf("\033[?25l\033[31m\033[1m%s \033[47m %d%% %s\033[?25h\033[0m\r",
                    Title.c_str(), (int)(Percent * 100), ProgressBuffer);
        } else {
            printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                    Title.c_str(), (int)(Percent * 100));
        }
        break;
    default:
        assert(false && "Unknown style!");
    }
    fflush(stdout);
}

void ProgressBar::resize() {
    // clear the line.
    printf("\r\033[K");

    int CurrentWindowWidth = 0;

#ifdef __linux__
    struct winsize WinSize;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &WinSize);
    CurrentWindowWidth = WinSize.ws_col > 80 ? 80 : WinSize.ws_col  - Title.length() - 15;
#elif _WIN32
    HANDLE StdOutHandler = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    GetConsoleScreenBufferInfo(StdOutHandler, ConsoleInfo);
    CurrentWindowWidth = ConsoleInfo.dwSize.x > 80 ? 80 : ConsoleInfo.dwSize.x - Title.length() - 15;
#endif

    if (CurrentWindowWidth < 15) {
        free(ProgressBuffer);
        ProgressBuffer = nullptr;
        return;
    }

    if (WindowWidth == CurrentWindowWidth) {
        return;
    }

    WindowWidth = CurrentWindowWidth;
    ProgressBuffer = (char*) realloc(ProgressBuffer, sizeof(char) * (WindowWidth + 1));
    if (Style == PBS_BGCStyle) {
        memset(ProgressBuffer, 0x00, WindowWidth + 1);
    } else {
        memset(ProgressBuffer, 32, WindowWidth);
        memset(ProgressBuffer + WindowWidth, 0x00, 1);
    }
}

void ProgressBar::reset() {
    LastUpdatePercent = 0;
    if (ProgressBuffer) {
        if (Style == PBS_BGCStyle) {
            memset(ProgressBuffer, 0x00, WindowWidth + 1);
        } else {
            memset(ProgressBuffer, 32, WindowWidth);
            memset(ProgressBuffer + WindowWidth, 0x00, 1);
        }
    }
}

ProgressBar::~ProgressBar() {
    if (ProgressBuffer) {
        free(ProgressBuffer);
        ProgressBuffer = nullptr;
    }
}
}
