//
// Created by kingdo on 22-7-19.
//

#include "wukong/utils/process/Process.h"
namespace wukong::utils
{
    pid_t Process::getPid() const
    {
        return pid;
    }
    int Process::send(int signum) const
    {
        return wukong::utils::kill(pid, signum);
    }
    bool Process::kill()
    {
        int ret = send(SIGTERM);
        if (ret)
        {
            SPDLOG_ERROR("Send {} to {} get an errno : {}", SIGTERM, pid, ret);
            return false;
        }
        WK_CHECK(pid == ::waitpid(pid, nullptr, 0), "Process Kill Wrong!");
        return true;
    }
}