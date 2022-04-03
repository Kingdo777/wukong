//
// Created by kingdo on 2022/4/2.
//
#include <wukong/utils/cgroup/CGroup.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>
int main()
{
    wukong::utils::initLog();
    SIGNAL_HANDLER()

    SPDLOG_DEBUG("Main Process {}", getpid());

    CpuCGroup ccg("c1");
    MemoryCGroup mcg("m1");
    WK_CHECK_FUNC_RET_WITH_ASSERT(ccg.create());
    WK_CHECK_FUNC_RET_WITH_ASSERT(mcg.create());
    CpuCGroup ccg2("c2");
    MemoryCGroup mcg2("m2");
    TIMING_START(Create_CGroup)
    WK_CHECK_FUNC_RET_WITH_ASSERT(mcg2.create());
    WK_CHECK_FUNC_RET_WITH_ASSERT(mcg2.setHardLimit(65));
    WK_CHECK_FUNC_RET_WITH_ASSERT(ccg2.create());
    WK_CHECK_FUNC_RET_WITH_ASSERT(ccg2.setShares(2048));
    WK_CHECK_FUNC_RET_WITH_ASSERT(ccg2.setCPUS(2000));
    TIMING_END(Create_CGroup)

    if (fork() == 0)
    {
        TIMING_START(MOVE_CGroup_1)
        WK_CHECK_FUNC_RET_WITH_ASSERT(ccg.moveTo());
        WK_CHECK_FUNC_RET_WITH_ASSERT(mcg.moveTo());
        TIMING_END(MOVE_CGroup_1)

        setsid();
        char* args[] = {
            nullptr
        };
        int ret = execvp(boost::dll::program_location().parent_path().append("tool-cgroup-subprocess").c_str(), args);
        WK_CHECK_WITH_ASSERT(!ret, wukong::utils::errors());
    }
    if (fork() == 0)
    {
        TIMING_START(MOVE_TO_CGroup_2)
        WK_CHECK_FUNC_RET_WITH_ASSERT(ccg2.moveTo());
        WK_CHECK_FUNC_RET_WITH_ASSERT(mcg2.moveTo());
        TIMING_END(MOVE_TO_CGroup_2)
        setsid();
        char* args[] = {
            nullptr
        };
        int ret = execvp(boost::dll::program_location().parent_path().append("tool-cgroup-subprocess").c_str(), args);
        WK_CHECK_WITH_ASSERT(!ret, wukong::utils::errors());
    }
    SIGNAL_WAIT()
    SPDLOG_INFO("Main Process Shutdown");
}