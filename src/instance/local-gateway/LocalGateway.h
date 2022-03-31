//
// Created by kingdo on 2022/3/28.
//

#ifndef WUKONG_LOCAL_GATEWAY_H
#define WUKONG_LOCAL_GATEWAY_H


#include "LocalGatewayEndpoint.h"
#include <wukong/utils/os.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/locks.h>
#include <wukong/utils/process/DefaultSubProcess.h>
#include <boost/dll.hpp>

#define function_index(username, appname, funcname) (fmt::format("{}#{}#{}",username,appname,funcname))

class LocalGateway {
public:
    LocalGateway();

    void start();

    void shutdown();

    bool checkUser(const std::string &username_);

    bool checkApp(const std::string &appname_);

    static bool existFunCode(const std::string &funcname);

    static boost::filesystem::path getFunCodePath(const std::string &funcname);

    wukong::proto::Function getFunction(const std::string &funcname);

    std::pair<bool, std::string> loadFuncCode(const std::string &funcname, bool update = false);

    std::pair<bool, std::string> initApp(const std::string &username_, const std::string &appname_);

    typedef std::string (*FP_Func)();

    static bool PingCode(const boost::filesystem::path &lib_path);

    std::pair<bool, std::string> createFuncProcess(const wukong::proto::Function &func);

    void callFunc(const wukong::proto::Message &msg, Pistache::Http::ResponseWriter response);

    std::set<int> getReadFDs();

    /// 作用是记录那个handler监听process
    struct ProcessInfo {
    public:

        ProcessInfo(std::shared_ptr<wukong::utils::DefaultSubProcess> p_,
                    std::shared_ptr<LocalGatewayClientHandler> h_) :
                p(std::move(p_)),
                h(std::move(h_)) {}

        std::shared_ptr<wukong::utils::DefaultSubProcess> process() {
            return p;
        }

        std::shared_ptr<LocalGatewayClientHandler> handler() {
            return h;
        }

    private:
        std::shared_ptr<wukong::utils::DefaultSubProcess> p;
        std::shared_ptr<LocalGatewayClientHandler> h;
    };

private:

    std::shared_ptr<LocalGatewayClientHandler> pickOneHandler();

    void killAllProcess();

    wukong::utils::Redis &redis = wukong::utils::Redis::getRedis();

    LocalGatewayEndpoint endpoint;
    wukong::client::ClientServer cs;


    std::string username;
    std::string appname;

    std::shared_mutex functions_mutex;
    std::unordered_map<std::string, wukong::proto::Function> functions;

    boost::filesystem::path worker_func_exec_path;
    boost::filesystem::path storage_func_exec_path;

    std::shared_mutex process_mutex;
    std::unordered_map<std::string, ProcessInfo> processes;

    std::shared_mutex read_fd_set_mutex;
    std::set<int> read_fd_set;


};

#endif //WUKONG_LOCAL_GATEWAY_H