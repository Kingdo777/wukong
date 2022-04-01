//
// Created by kingdo on 2022/3/21.
//

#ifndef WUKONG_UV_SUB_PROCESS_H
#define WUKONG_UV_SUB_PROCESS_H

#include <uv.h>

namespace wukong::utils
{
    class UVSubProcess : public SubProcess
    {

        //        int Start() {
        ////            read_buffer_pool_ = read_buffer_pool;
        ////            handle_scope_.Init(uv_loop, absl::bind_front(&Subprocess::OnAllHandlesClosed, this));
        //            uv_process_options_t uv_options;
        //            memset(&uv_options, 0, sizeof(uv_process_options_t));
        //            uv_options.exit_cb = &Subprocess::ProcessExitCallback;
        //            uv_options.file = options.file.c_str();
        //            uv_options.args = const_cast<char **>(options.args.data());
        //            uv_options.env = const_cast<char **>(options.env.data());
        //            if (!options.cwd.empty()) {
        //                uv_options.cwd = options.cwd.c_str();
        //            }
        //            int num_pipes = stdio.size();
        //            WK_CHECK_WITH_ASSERT(num_pipes >= 3, "stdio num is at least contains stdin/out/err");
        //            std::vector<uv_stdio_container_t> uv_stdio(num_pipes);
        //            uv_pipe_handles.resize(num_pipes);
        //            pipe_closed.assign(num_pipes, false);
        //            for (int i = 0; i < num_pipes; i++) {
        //                uv_pipe_t *uv_pipe = &uv_pipe_handles[i];
        //                if (i < kNumStdPipes && std_fds_[i] != -1) {
        //                    uv_stdio[i].flags = UV_INHERIT_FD;
        //                    uv_stdio[i].data.fd = std_fds_[i];
        //                    pipe_closed[i] = true;
        //                } else {
        //                    UV_DCHECK_OK(uv_pipe_init(uv_loop, uv_pipe, 0));
        //                    uv_pipe->data = this;
        //                    handle_scope_.AddHandle(uv_pipe);
        //                    uv_stdio[i].flags = static_cast<uv_stdio_flags>(UV_CREATE_PIPE | pipe_types_[i]);
        //                    uv_stdio[i].data.stream = UV_AS_STREAM(uv_pipe);
        //                }
        //            }
        //            uv_options.stdio_count = num_pipes;
        //            uv_options.stdio = uv_stdio.data();
        //            uv_process_handle.data = this;
        //            if (uv_spawn(uv_loop, &uv_process_handle, &uv_options) != 0) {
        //                return false;
        //            }
        //            pid_ = uv_process_handle.pid;
        //            handle_scope_.AddHandle(&uv_process_handle);
        //            if (std_fds_[kStdout] == -1) {
        //                UV_DCHECK_OK(uv_read_start(UV_AS_STREAM(&uv_pipe_handles[kStdout]),
        //                                           &Subprocess::BufferAllocCallback, &Subprocess::ReadStdoutCallback));
        //            }
        //            if (std_fds_[kStderr] == -1) {
        //                UV_DCHECK_OK(uv_read_start(UV_AS_STREAM(&uv_pipe_handles[kStderr]),
        //                                           &Subprocess::BufferAllocCallback, &Subprocess::ReadStderrCallback));
        //            }
        //            for (int i = 0; i < kNumStdPipes; i++) {
        //                if (std_fds_[i] != -1) {
        //                    PCHECK(close(std_fds_[i]) == 0);
        //                }
        //            }
        //            state_ = kRunning;
        //            return true;
        //        }

    private:
        //        uv_loop_t *uv_loop;
        //        uv_process_t uv_process_handle;
        //
        //        std::vector<uv_pipe_t> uv_pipe_handles;
        //        std::vector<bool> pipe_closed;

        //        ExitCallback exit_callback;
    };
}

#endif //WUKONG_UV_SUB_PROCESS_H
