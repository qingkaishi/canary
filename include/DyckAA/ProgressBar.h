/*
 * It is used to show progress in console.
 *
 *  Created on: Dec 05, 2016
 *      Author: Qingkai
 */

#ifndef DYCKAA_PROGRESSBAR_H
#define DYCKAA_PROGRESSBAR_H

#include <string>

namespace DyckAA {

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
    ProgressBar(std::string Title, ProgressBarStyle Style, float UpdateFrequency = 0.01);

    virtual ~ProgressBar();

    /// The input must be in [0, 1]
    /// 0 means it just starts.
    /// 1 means it completes.
    void showProgress(float Percent);

    void reset() {
        LastUpdatePercent = 0;
    }

private:
    /// In case the console width is changed
    /// at runtime.
    void resize();
};

}

#endif /* DYCKAA_PROGRESSBAR_H */
