//
// Created by kingdo on 2022/3/21.
//

#include <wukong/utils/process/DefaultSubProcess.h>

namespace wukong::utils
{

    DefaultSubProcess::DefaultSubProcess()
    {
        /// 创建子进程读端
        write_fd_index = SubProcess::createPIPE(
            wukong::utils::SubProcess::CREATE_PIPE | wukong::utils::SubProcess::READABLE_PIPE);
        /// 创建子进程写段
        read_fd_index = SubProcess::createPIPE(
            wukong::utils::SubProcess::CREATE_PIPE | wukong::utils::SubProcess::READABLE_PIPE);
    }

    int DefaultSubProcess::spawn()
    {
        int signal_pipe[2] = { -1 };
        std::vector<std::array<int, 2>> pipes;
        ssize_t r;
        pid_t pid;
        int err;
        int exec_errorno;
        int i;
        int status;
        WK_CHECK_WITH_EXIT(!options.file.empty(), "must specify sub-process file path");
        WK_CHECK_WITH_EXIT(!(options.flags & ~(DETACHED | SET_GID | SET_UID | SET_CPUS | SET_MEMORY)),
                             "options.flags is illegal");

        err             = ENOMEM;
        int stdio_count = (int)stdio.size();
        for (i = 0; i < stdio_count; i++)
        {
            pipes.push_back({ -1, -1 });
        }

        for (i = 0; i < stdio_count; i++)
        {
            err = init_stdio(stdio.at(i), pipes.at(i));
            if (err)
            {
                SPDLOG_ERROR("init stdio Failed");
                goto error;
            }
        }

        /** signal_pipe的作用是确保，父进程在子进程执行完execve()之后再执行其他操作，以避免以下情况:
         *    if ((pid = fork()) > 0) {
         *      kill(pid, SIGTERM);
         *    }
         *    else if (pid == 0) {
         *      execve("/bin/cat", argp, envp);
         *    }
         * 即在父进程fork之后立即向子进程发送信号，此时我们子进程是否已经执行了execve(),就会产生歧义。             *
         * 为了避免这种情况，我们创建了signal_pipe，它是close-on-exec类型的，我们让父进程阻塞在这个pipe上，
         * 当子进程的执行遇到错误，或者执行了exec，那么signal_pipe会收到EOFs 或者 EPIPE errors，从而再继续执行父进程

         */
        err = make_pipe(signal_pipe, 0);
        if (err)
        {
            SPDLOG_ERROR("make signal pipe failed");
            goto error;
        }

        pid = fork();

        if (pid == -1)
        {
            err = errno;
            ::close(signal_pipe[0]);
            ::close(signal_pipe[1]);
            SPDLOG_ERROR("fork failed with pid==-1");
            goto error;
        }

        if (pid == 0)
        {
            process_child_init(pipes, signal_pipe[1]);
            abort();
        }
        state = Running;
        ::close(signal_pipe[1]);

        exec_errorno = 0;
        do
            r = read(signal_pipe[0], &exec_errorno, sizeof(exec_errorno));
        while (r == -1 && errno == EINTR);

        if (r == 0)
            ; /// 子进程正常执行
        else if ((r == -1 && errno == EPIPE) || r == sizeof(exec_errorno))
        {
            /// 子进程遇到错误
            do
                err = ::waitpid(pid, &status, 0); /* okay, got EPIPE */
            while (err == -1 && errno == EINTR);
            assert(err == pid);
            state = Exited;
            SPDLOG_ERROR("spawn process failed before EXEC");
        }
        else
            abort();

        ::close(signal_pipe[0]);

        for (i = 0; i < stdio_count; i++)
        {
            if (!(stdio.at(i).type & CREATE_PIPE) || pipes[i][0] < 0)
                continue;
            err            = ::close(pipes[i][1]);
            stdio.at(i).fd = pipes[i][0];
            if (err != 0)
            {
                SPDLOG_ERROR("close pipes failed");
                goto error;
            }
            pipes[i][1] = -1;
            nonblock_ioctl(pipes[i][0], 1);
        }

        read_fd_  = SubProcess::getPIPE_FD(read_fd_index);
        write_fd_ = SubProcess::getPIPE_FD(write_fd_index);
        this->pid = pid;

        return exec_errorno;

    error:
        for (i = 0; i < stdio_count; i++)
        {
            if (i < stdio_count)
                if (stdio.at(i).type & (INHERIT_FD))
                    continue;
            if (pipes[i][0] != -1)
                ::close(pipes[i][0]);
            if (pipes[i][1] != -1)
                ::close(pipes[i][1]);
        }
        SPDLOG_ERROR("spawn process failed before FORK");
        return err;
    }

    int DefaultSubProcess::read_fd() const
    {
        return read_fd_;
    }

    int DefaultSubProcess::write_fd() const
    {
        return write_fd_;
    }

    int DefaultSubProcess::init_stdio(SubProcess::StdioContainer container, std::array<int, 2>& fds)
    {
        int mask;
        int fd;

        mask = IGNORE | CREATE_PIPE | INHERIT_FD;

        switch (container.type & mask)
        {
        case IGNORE:
            return 0;
        case CREATE_PIPE:
            return socket_pair(fds.data());
        case INHERIT_FD:
            fd = container.fd;
            if (fd == -1)
                return EINVAL;
            fds[1] = fd;
            return 0;
        default:
            WK_CHECK_WITH_EXIT(0, "Unexpected flags");
            return EINVAL;
        }
    }

    void DefaultSubProcess::process_child_init(std::vector<std::array<int, 2>> pipes, int error_fd)
    {
        sigset_t set;
        int close_fd;
        int use_fd;
        int err;
        int fd;
        int n;

        if (options.flags & DETACHED)
            setsid();

        if (options.flags & SET_MEMORY)
        {
        }
        if (options.flags & SET_CPUS)
        {
        }

        int pipes_count = (int)pipes.size();

        /* First duplicate low numbered fds, since it's not safe to duplicate them,
         * they could get replaced. Example: swapping stdout and stderr; without
         * this fd 2 (stderr) would be duplicated into fd 1, thus making both
         * stdout and stderr go to the same fd, which was not the intention. */
        for (fd = 0; fd < pipes_count; fd++)
        {
            use_fd = pipes[fd][1];
            if (use_fd < 0 || use_fd >= fd)
                continue;
            pipes[fd][1] = fcntl(use_fd, F_DUPFD, pipes_count);
            if (pipes[fd][1] == -1)
            {
                write_2_fd(error_fd, errno);
                _exit(127);
            }
        }

        for (fd = 0; fd < pipes_count; fd++)
        {
            close_fd = pipes[fd][0];
            use_fd   = pipes[fd][1];

            if (use_fd < 0)
            {
                if (fd >= 3)
                    continue;
                else
                {
                    /* redirect stdin, stdout and stderr to /dev/nullptr even if UV_IGNORE is
                     * set
                     */
                    use_fd   = open("/dev/nullptr", fd == 0 ? O_RDONLY : O_RDWR);
                    close_fd = use_fd;

                    if (use_fd < 0)
                    {
                        write_2_fd(error_fd, errno);
                        _exit(127);
                    }
                }
            }

            if (fd == use_fd)
                cloexec_fcntl(use_fd, 0);
            else
                    fd = ::dup2(use_fd, fd);

            if (fd == -1)
            {
                write_2_fd(error_fd, errno);
                _exit(127);
            }

            if (fd <= 2)
                nonblock_fcntl(fd, 0);

            if (close_fd >= pipes_count)
                ::close(close_fd);
        }

        for (fd = 0; fd < pipes_count; fd++)
        {
            use_fd = pipes[fd][1];

            if (use_fd >= pipes_count)
                ::close(use_fd);
        }

        if (!options.cwd.empty() && chdir(options.cwd.c_str()))
        {
            write_2_fd(error_fd, errno);
            SPDLOG_ERROR("change Work Dir failed");
            _exit(127);
        }

        if (options.flags & (SET_UID | SET_GID))
        {
            /* When dropping privileges from root, the `setgroups` call will
             * remove any extraneous groups. If we don't call this, then
             * even though our uid has dropped, we may still have groups
             * that enable us to do super-user things. This will fail if we
             * aren't root, so don't bother checking the return value, this
             * is just done as an optimistic privilege dropping function.
             */
            setgroups(0, nullptr);
        }

        if ((options.flags & SET_GID) && setgid(options.gid))
        {
            write_2_fd(error_fd, errno);
            _exit(127);
        }

        if ((options.flags & SET_UID) && setuid(options.uid))
        {
            write_2_fd(error_fd, errno);
            _exit(127);
        }

        std::vector<char*> env;
        if (!options.env.empty())
        {
            char** env_ptr = environ;
            while (*env_ptr != nullptr)
            {
                env.emplace_back(*env_ptr);
                env_ptr++;
            }
            for (const auto& item : options.env)
            {
                env.emplace_back(const_cast<char*>(item.c_str()));
            }
            env.emplace_back(nullptr);
            environ = env.data();
        }

        /* Reset signal disposition.  Use a hard-coded limit because NSIG
         * is not fixed on Linux: it's either 32, 34 or 64, depending on
         * whether RT signals are enabled.  We are not allowed to touch
         * RT signal handlers, glibc uses them internally.
         */
        for (n = 1; n < 32; n += 1)
        {
            if (n == SIGKILL || n == SIGSTOP)
                continue; /* Can't be changed. */

#if defined(__HAIKU__)
            if (n == SIGKILLTHR)
                continue; /* Can't be changed. */
#endif

            if (SIG_ERR != signal(n, SIG_DFL))
                continue;

            write_2_fd(error_fd, errno);
            _exit(127);
        }

        /* Reset signal mask. */
        sigemptyset(&set);
        err = pthread_sigmask(SIG_SETMASK, &set, nullptr);

        if (err != 0)
        {
            write_2_fd(error_fd, err);
            _exit(127);
        }

        std::vector<char*> args;
        for (const auto& item : options.args)
        {
            args.emplace_back(const_cast<char*>(item.c_str()));
        }
        args.emplace_back(nullptr);

        execvp(options.file.c_str(), args.data());
        write_2_fd(error_fd, errno);
        _exit(127);
    }
}