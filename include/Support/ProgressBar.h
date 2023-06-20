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

#ifndef SUPPORT_PROGRESSBAR_H
#define SUPPORT_PROGRESSBAR_H

#include <string>

class ProgressBar {
public:
    enum ProgressBarStyle {
        PBS_NumberStyle,
        PBS_CharacterStyle,
        PBS_BGCStyle
    };

private:
    /// Title of the progress bar
    std::string Title;

    /// Style of the progress bar
    ProgressBarStyle Style;

    /// Windows width
    int WindowWidth;

    /// The buffer of characters, which
    /// will be shown in console as a progress bar.
    char* ProgressBuffer;

    /// How frequently to update the progress bar.
    /// For example, it Delta is 0.01, then the bar
    /// will be updated every 1 percent.
    /// @{
    float UpdateFrequency;
    float LastUpdatePercent = 0;
    /// @}

public:
    ProgressBar(const std::string &Title, ProgressBarStyle Style, float UpdateFrequency = 0.01);

    virtual ~ProgressBar();

    /// The input must be in [0, 1]
    /// 0 means it just starts.
    /// 1 means it completes.
    void showProgress(float Percent);

    void reset();

private:
    /// In case the console width is changed
    /// at runtime.
    void resize();
};

#endif // SUPPORT_PROGRESSBAR_H
