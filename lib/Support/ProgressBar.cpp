/*
 *  Canary features a fast unification-based alias analysis for C programs
 *  Copyright (C) 2021 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sys/ioctl.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include "Support/ProgressBar.h"

ProgressBar::ProgressBar(const std::string &Title, ProgressBarStyle Style, float UpdateFrequency) :
        Title(Title), Style(Style), UpdateFrequency(UpdateFrequency) {

    struct winsize WinSize{};
    ioctl(STDIN_FILENO, TIOCGWINSZ, &WinSize);
    if (WinSize.ws_col > 80) {
        WindowWidth = 80;
    } else if (WinSize.ws_col >= Title.length() + 15) {
        WindowWidth = WinSize.ws_col - Title.length() - 15;
    } else {
        WindowWidth = 0;
    }

    if (WindowWidth < 15) {
        ProgressBuffer = nullptr;
        return;
    }

    ProgressBuffer = (char *) malloc(sizeof(char) * (WindowWidth + 1));
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

    auto Val = (unsigned) (Percent * (float) WindowWidth);
    switch (this->Style) {
        case PBS_NumberStyle:
            printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                   Title.c_str(), (int) (Percent * 100));
            break;
        case PBS_CharacterStyle:
            if (ProgressBuffer) {
                memset(ProgressBuffer, '#', Val);
                printf("\033[?25l\033[37m\033[1m%s [%-s] %d%%\033[?25h\033[0m\r",
                       Title.c_str(), ProgressBuffer, (int) (Percent * 100));
            } else {
                printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                       Title.c_str(), (int) (Percent * 100));
            }
            break;
        case PBS_BGCStyle:
            if (ProgressBuffer) {
                memset(ProgressBuffer, 32, Val);
                printf("\033[?25l\033[31m\033[1m%s \033[47m %d%% %s\033[?25h\033[0m\r",
                       Title.c_str(), (int) (Percent * 100), ProgressBuffer);
            } else {
                printf("\033[?25l\033[37m\033[1m%s %d%%\033[?25h\033[0m\r",
                       Title.c_str(), (int) (Percent * 100));
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

    unsigned CurrentWindowWidth;
    struct winsize WinSize{};
    ioctl(STDIN_FILENO, TIOCGWINSZ, &WinSize);
    if (WinSize.ws_col > 80) {
        CurrentWindowWidth = 80;
    } else if (WinSize.ws_col >= Title.length() + 15) {
        CurrentWindowWidth = WinSize.ws_col - Title.length() - 15;
    } else {
        CurrentWindowWidth = 0;
    }

    if (CurrentWindowWidth < 15) {
        free(ProgressBuffer);
        ProgressBuffer = nullptr;
        return;
    }

    if (WindowWidth == CurrentWindowWidth) {
        return;
    }

    WindowWidth = CurrentWindowWidth;
    ProgressBuffer = (char *) realloc(ProgressBuffer, sizeof(char) * (WindowWidth + 1));
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
