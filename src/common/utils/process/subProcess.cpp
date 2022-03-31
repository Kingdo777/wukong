//
// Created by kingdo on 2022/3/18.
//

#include <wukong/utils/process/subProcess.h>
#include <utility>
#include <csignal>

namespace wukong::utils {

    SubProcess::Options::Options(const std::string &cmd) :
            file("/bin/bash"),
            cwd("/"),
            flags(0),
            resource({}),
            uid(-1),
            gid(-1) {
        args.emplace_back(file);
        args.emplace_back("-c");
        args.emplace_back(cmd);
    }

    SubProcess::Options &SubProcess::Options::File(const std::string &file_) {
        file = file_;
        args.emplace_back(file_);
        return *this;
    }

    SubProcess::Options &SubProcess::Options::Args(const std::string &s) {
        args.pop_back();
        args.emplace_back(s);
        return *this;
    }

    SubProcess::Options &SubProcess::Options::Workdir(const std::string &dir) {
        cwd = dir;
        return *this;
    }

    SubProcess::Options &SubProcess::Options::Env(const std::string &key, const std::string &value) {
        env.emplace_back(fmt::format("{}={}", key, value).c_str());
        return *this;
    }

    SubProcess::Options &SubProcess::Options::Env(const std::string &key, int64_t value) {
        env.emplace_back(fmt::format("{}={}", key, value).c_str());
        return *this;
    }

    SubProcess::Options &SubProcess::Options::Flags(unsigned int flags_) {
        flags = flags_;
        return *this;
    }

    bool SubProcess::Options::hasFlags(unsigned int flags_) const {
        return flags & flags_;
    }

    SubProcess::Options &SubProcess::Options::CPU(uint64_t cpu) {
        resource.cpus = cpu;
        return *this;
    }

    SubProcess::Options &SubProcess::Options::Memory(uint64_t memory_) {
        resource.memory = memory_;
        return *this;
    }

    SubProcess::Options &SubProcess::Options::UID(uid_t uid_) {
        uid = uid_;
        return *this;
    }

    SubProcess::Options &SubProcess::Options::GID(gid_t gid_) {
        gid = gid_;
        return *this;
    }

    SubProcess::SubProcess() {
        for (int i = 0; i < NumStdPipes; i++)
            stdio.push_back({i, INHERIT_FD});
    }

    SubProcess::SubProcess(Options option) :
            state(Created),
            options(std::move(option)),
            pid(-1),
            exitStatus(0) {
        for (int i = 0; i < NumStdPipes; i++)
            stdio.push_back({i, INHERIT_FD});
    }

    SubProcess::~SubProcess() {
        WK_CHECK_STATE_2(Created, Exited);
    }

    void SubProcess::setStandardFile(SubProcess::StandardPipe pipe, std::string_view file_path) {
        WK_CHECK_STATE_WITH_ASSERT(Created);
        WK_CHECK_WITH_ASSERT(stdio[pipe].fd == pipe, "this stdPip has been rightly set");
        int fd;
        StdioType type;
        if (pipe == Stdin) {
            fd = open(std::string(file_path).c_str(), O_RDONLY);
            WK_CHECK_WITH_ASSERT(fd != -1, fmt::format("can't open the file, {}", file_path));
            type = READABLE_PIPE;
        } else {
            fd = creat(std::string(file_path).c_str(), WUKONG_FILE_CREAT_MODE);
            WK_CHECK_WITH_ASSERT(fd != -1, fmt::format("can't open the file, {}", file_path));
            type = WRITABLE_PIPE;
        }
        stdio.push_back({fd, type});
    }

    pid_t SubProcess::getPid() {
        WK_CHECK_STATE(Running);
        return pid;
    }

    int SubProcess::send(int signum) const {
        WK_CHECK_STATE_WITH_ASSERT(Running);
        if (::kill(pid, signum)) {
            SPDLOG_ERROR("kill {} to {} Failed", signum, pid);
            return errno;
        } else
            return 0;
    }

    bool SubProcess::kill() {
        int ret = send(SIGTERM);
        if (ret) {
            SPDLOG_ERROR("Send {} to {} get an errno : {}", SIGKILL, pid, ret);
            return false;
        }
        WK_CHECK(pid == ::waitpid(pid, nullptr, 0), "SubProcess Kill Wrong!");
        state = Exited;
        return true;
    }


}