//
// Created by kingdo on 2022/2/25.
//

#include <wukong/utils/timing.h>

std::unordered_map<std::string, std::atomic<unsigned long>> timerTotals;
std::unordered_map<std::string, std::atomic<int>> timerCounts;

namespace wukong::utils
{
    uint64_t getMillsTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    TimePoint Timing::now()
    {
        return std::chrono::steady_clock::now();
    }

    long Timing::timeDiffMicro(const TimePoint& t1, const TimePoint& t2)
    {
        long age = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t2).count();
        return age;
    }

    TimePoint Timing::startTimer()
    {
        return now();
    }

    void Timing::logEndTimer(const std::string& label, const TimePoint& begin)
    {
        unsigned long micros = timeDiffMicro(now(), begin);

        SPDLOG_TRACE("TIME = {:.3f}ms ({})", (double)micros / 1000, label);

        // Record microseconds total
        timerTotals[label] += micros;
        timerCounts[label]++;
    }

    void Timing::printTimerTotals()
    {
        // Switch the pairs so we can use std::sort
        std::vector<std::pair<long, std::string>> totals;
        totals.reserve(timerTotals.size());
        for (auto& p : timerTotals)
        {
            totals.emplace_back(p.second, p.first);
        }

        std::sort(totals.begin(), totals.end());

        SPDLOG_INFO("---------- TIMER TOTALS ----------");
        SPDLOG_INFO("Total (ms)  Avg (ms)   Count  Label");
        for (auto& p : totals)
        {
            double millis = double(p.first) / 1000.0;
            int count     = timerCounts[p.second];
            double avg    = millis / count;
            SPDLOG_INFO(
                "{:-11.2f} {:-10.3f} {:5}  {}", millis, avg, count, p.second.c_str());
        }
    }
}
