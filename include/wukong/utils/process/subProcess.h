//
// Created by kingdo on 2022/3/18.
// 以下代码，参照了nightcore(https://github.com/ut-osa/nightcore.git)
//

#ifndef WUKONG_SUBPROCESS_H
#define WUKONG_SUBPROCESS_H

#include <string>
#include <vector>
#include <wukong/utils/log.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/os.h>
#include <wukong/utils/signal-tool.h>

#define WK_CHECK_STATE(s1)                                              \
    do                                                                  \
    {                                                                   \
        WK_CHECK((state == (s1)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_2(s1, s2)                                                         \
    do                                                                                   \
    {                                                                                    \
        WK_CHECK((state == (s1) || state == (s2)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_3(s1, s2, s3)                                                                      \
    do                                                                                                    \
    {                                                                                                     \
        WK_CHECK((state == (s1) || state == (s2) || state == (s3)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_WITH_ASSERT(s1)                                              \
    do                                                                              \
    {                                                                             \
        WK_CHECK_WITH_EXIT((state == (s1)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_WITH_ASSERT_2(s1, s2)                                                         \
    do                                                                                               \
    {                                                                                              \
        WK_CHECK_WITH_EXIT((state == (s1) || state == (s2)), "SubProcess Status is Not Expected"); \
    } while (false)
#define WK_CHECK_STATE_WITH_ASSERT_3(s1, s2, s3)                                                                      \
    do                                                                                                                \
    {                                                                                                               \
        WK_CHECK_WITH_EXIT((state == (s1) || state == (s2) || state == (s3)), "SubProcess Status is Not Expected"); \
    } while (false)

namespace wukong::utils
{
    typedef void (*ProcessExitCb)(int64_t exit_status, int term_signal);

    class SubProcess
    {
    public:
        struct ProcessResource
        {
            uint64_t cpus; /// 指定使用cpu的core数，每个core划分为1000份
            uint64_t memory; /// 指定使用memory的多少，单位为MB
        };

        enum OptionFlags {
            SET_UID  = (1 << 0),
            SET_GID  = (1 << 1),
            DETACHED = (1 << 3),
            /// 特有flags，libuv不包含
            SET_CPUS   = (1 << 7),
            SET_MEMORY = (1 << 8),
        };

        enum StdioType {
            IGNORE      = 0x00,
            CREATE_PIPE = 0x01,
            INHERIT_FD  = 0x02,

            READABLE_PIPE = 0x10,
            WRITABLE_PIPE = 0x20,

        };

        struct Options
        {
            friend class SubProcess;

        public:
            Options() = default;

            explicit Options(const std::string& cmd);

            Options& File(const std::string& file);

            Options& Args(const std::string& s);

            Options& Workdir(const std::string& dir);

            Options& Env(const std::string& key, const std::string& value);

            Options& Env(const std::string& key, int64_t value);

            Options& Flags(unsigned int flags_);

            [[nodiscard]] bool hasFlags(unsigned int flags_) const;

            Options& CPU(uint64_t cpu);

            Options& Memory(uint64_t memory_);

            Options& UID(uid_t uid_);

            Options& GID(gid_t gid_);

            std::string file; /// 可执行文件的路径, 是必选的参数
            std::vector<std::string> args; /// 命令行的参数，默认第一个参数同file
            std::vector<std::string> env; /// 环境变量
            std::string cwd; /// 工作目录

            unsigned int flags = 0; /// 控制子进程相关行为的Flags

            /// 用于配置进程的CPU/Memory资源
            ProcessResource resource { 0 };

            /// 用于配置Group ID和User ID，仅当flags中对应的标志位生效时才可用
            uid_t uid = 0;
            gid_t gid = 0;
        };

        struct StdioContainer
        {
            int fd;
            uint64_t type;
        };

    public:
        /// 进程的标准输入、输出、错误输出的fd在stdFds中的默认位置
        /// 在默认情况下其fd分别是宏：stdin,stdin,stdin
        enum StandardPipe {
            Stdin       = 0,
            Stdout      = 1,
            Stderr      = 2,
            NumStdPipes = 3
        };

        SubProcess();

        explicit SubProcess(Options option);

        ~SubProcess();

        void setOptions(const Options& option)
        {
            options = option;
        }

        void setStandardFile(StandardPipe pipe, std::string_view file_path);

        pid_t getPid();

        [[nodiscard]] int send(int signum) const;

        bool kill();

        uint64_t createPIPE(uint64_t type)
        {
            WK_CHECK_WITH_EXIT(stdio.size() >= 3, "count of stdio illegal");
            stdio.push_back({ -1, type });
            return stdio.size() - 1;
        }

        [[nodiscard]] int getPIPE_FD(uint64_t index) const
        {
            WK_CHECK_WITH_EXIT(stdio.size() > index, "index is out of range");
            WK_CHECK_STATE(Running);
            return stdio[index].fd;
        }

        [[nodiscard]] bool isRunning() const
        {
            return state == Running;
        }

        virtual int spawn() = 0;

    protected:
        enum State {
            Created,
            Running,
            Paused,
            Exited
        };
        State state = Created;

        Options options;
        int pid            = -1;
        int64_t exitStatus = 0;

        std::vector<StdioContainer> stdio;
    };

}

#endif // WUKONG_SUBPROCESS_H
