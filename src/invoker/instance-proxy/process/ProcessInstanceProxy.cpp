//
// Created by kingdo on 2022/3/17.
//

#include "ProcessInstanceProxy.h"

std::pair<bool, std::string>
ProcessInstanceProxy::doStart(const std::string &username, const std::string &appname) {
    {
        bool success = false;
        std::string msg = "ok";
        /// 获取可执行文件的路径
//        std::string curr_path;
//        curr_path.resize(1024);
//        auto path_size = readlink("/proc/self/exe", curr_path.data(), curr_path.size());
//        WK_CHECK_WITH_ASSERT(path_size != -1, "get exec path failed");
//        exec_path = curr_path.substr(0, path_size - std::size("invoker")) + "/instance-local-gateway";
        exec_path = boost::dll::program_location().parent_path().append("instance-local-gateway");
        /// 设置options，包括环境变量等
        wukong::utils::SubProcess::Options options(exec_path.string());
        options.Env("ENDPOINT_PORT", 0).
                Env("NEED_RETURN_PORT", 1).
                Flags(0);

        process.setOptions(options);
        /// 启动子进程
        int err = process.spawn();
        if (!(0 == err)) {
            msg = fmt::format("spawn process \"{}\" failed with errno {}", exec_path.string(), err);
            return std::make_pair(success, msg);
        }
        /// 获取实例的port
        int fd = process.read_fd();
        /// TODO 这里需要将fd转化为阻塞的
        wukong::utils::nonblock_ioctl(fd, 0);
        wukong::utils::read_from_fd(fd, &instancePort);
        if (instancePort <= 0) {
            msg = fmt::format("can't get Instance Port", exec_path.string(), err);
            return std::make_pair(success, msg);
        }
        auto ping_res = this->doPing();
        if (ping_res.first) {
            success = true;
        } else {
            msg = fmt::format("spawn process \"{}\" success, but cant't ping it : {}", exec_path.string(),
                              ping_res.second);
            this->remove(true);
        }
        return std::make_pair(success, msg);
    }
}

std::pair<bool, std::string>
ProcessInstanceProxy::doInit(const std::string &username, const std::string &appname) {
    bool success = false;
    std::string msg = "ok";
    std::string uri = fmt::format("http://localhost:{}/init", instancePort);
    Pistache::Http::Cookie cookie1("username", username);
    Pistache::Http::Cookie cookie2("appname", appname);
    auto rsp = client()->
            post(uri).
            cookie(cookie1).cookie(cookie2).
            send();
    while (rsp.isPending());
    rsp.then(
            [&](Pistache::Http::Response response) {
                if (response.code() == Pistache::Http::Code::Ok && response.body() == "ok") {
                    success = true;
                } else {
                    msg = fmt::format("get wrong status code : {} , with msg : {}",
                                      response.code(),
                                      response.body());
                }
            },
            [&](const std::exception_ptr &exc) {
                try {
                    std::rethrow_exception(exc);
                }
                catch (const std::exception &e) {
                    msg = e.what();
                }
            });
    return std::make_pair(success, msg);
}

std::pair<bool, std::string> ProcessInstanceProxy::doPing() {
    bool success = false;
    std::string msg = "ok";
    std::string uri = fmt::format("http://localhost:{}/ping", instancePort);
    auto rsp = client()->get(uri).send();
    while (rsp.isPending());
    rsp.then(
            [&](Pistache::Http::Response response) {
                if (response.code() == Pistache::Http::Code::Ok && response.body() == "PONG") {
                    success = true;
                } else {
                    msg = "Status Code Wrong, " + response.body();
                }
            },
            [&](const std::exception_ptr &exc) {
                try {
                    std::rethrow_exception(exc);
                }
                catch (const std::exception &e) {
                    msg = e.what();
                }
            });
    return std::make_pair(success, msg);
}
