//
// Created by kingdo on 2022/3/21.
//

#ifndef WUKONG_DEFAULT_SUBPROCESS_H
#define WUKONG_DEFAULT_SUBPROCESS_H

#include <utility>
#include <wukong/utils/os.h>
#include <wukong/utils/process/subProcess.h>

const static int subprocess_write_fd = 4;
const static int subprocess_read_fd  = 3;

namespace wukong::utils
{
    class DefaultSubProcess : public SubProcess
    {
    public:
        DefaultSubProcess();

        explicit DefaultSubProcess(const Options& option)
            : SubProcess(option)
        { }

        int spawn() override;

        [[nodiscard]] int read_fd() const;

        [[nodiscard]] int write_fd() const;

    private:
        static int init_stdio(StdioContainer container, std::array<int, 2>& fds);

        void process_child_init(std::vector<std::array<int, 2>> pipes, int error_fd);

        uint64_t write_fd_index = 3;
        int read_fd_            = -1;
        uint64_t read_fd_index  = 4;
        int write_fd_           = -1;
    };
}

#endif //WUKONG_DEFAULT_SUBPROCESS_H
