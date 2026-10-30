#pragma once
#include <algorithm>

namespace epidemic::gameplay::presentation {
struct StartupSequence {
    static constexpr double LogoEnd = 3.15, Duration = 3.95;
    double sweep = 0, contour = 0, logoOpacity = 0, curtain = 1;
    bool logo = true, complete = false;

    static double Smooth(double t) {
        t = std::clamp(t, 0., 1.);
        return t * t * (3 - 2 * t);
    }

    static StartupSequence At(double time, bool motion = true) {
        StartupSequence s;
        if (!motion) {
            s.logoOpacity = Smooth(time / .25) * (1 - Smooth((time - 1.1) / .35));
            s.contour = s.logoOpacity;
            s.logo = time < 1.45;
            s.curtain = 1 - Smooth((time - 1.45) / .35);
            s.complete = time >= 1.8;
            return s;
        }

        s.sweep = std::clamp((time - .35) / 2.05, 0., 1.);
        s.contour = Smooth(time / .5) * (1 - Smooth((time - 2.4) / .75));
        s.logoOpacity = Smooth(time / .35) * (1 - Smooth((time - 2.4) / .75));
        s.logo = time < LogoEnd;
        s.curtain = 1 - Smooth((time - LogoEnd) / .8);
        s.complete = time >= Duration;
        return s;
    }

};

} // namespace epidemic::gameplay::presentation
