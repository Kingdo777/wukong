//
// Created by kingdo on 2022/2/25.
//

#ifndef WUKONG_TIMING_H
#define WUKONG_TIMING_H

#include <wukong/utils/log.h>
#include <chrono>
#include <string>

#ifdef ENABLE_TRACE
#define TIMING_START(name)                                                       \
        const wukong::utils::TimePoint name = wukong::utils::Timing::startTimer();
#define TIMING_END(name) \
        wukong::utils::Timing::logEndTimer(#name, name);
#define TIMING_SUMMARY \
        wukong::utils::Timing::printTimerTotals();
#else
#define TIMING_START(name)
#define TIMING_END(name)
#define TIMING_SUMMARY
#endif

namespace wukong::utils {
    typedef std::chrono::steady_clock::time_point TimePoint;

    uint64_t getMillsTimestamp();

    class Timing {
    public:
        Timing() = default;

        static TimePoint startTimer();

        static void logEndTimer(const std::string &label,
                                const wukong::utils::TimePoint &begin);

        static void printTimerTotals();


    private:
        static TimePoint now();

        static long timeDiffMicro(const TimePoint &t1, const TimePoint &t2);
    };
}

#endif //WUKONG_TIMING_H
