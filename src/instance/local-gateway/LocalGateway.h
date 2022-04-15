//
// Created by kingdo on 2022/3/28.
//

#ifndef WUKONG_LOCAL_GATEWAY_H
#define WUKONG_LOCAL_GATEWAY_H

#include "LocalGatewayEndpoint.h"
#include <boost/dll.hpp>
#include <wukong/proto/proto.h>
#include <wukong/utils/locks.h>
#include <wukong/utils/os.h>
#include <wukong/utils/process/DefaultSubProcess.h>
#include <wukong/utils/struct.h>

#define function_index(username, appname, funcname) (fmt::format("{}#{}#{}", username_, appname_, funcname))

class LocalGateway
{
public:
    LocalGateway();

    void start();

    void shutdown();

    bool checkUser(const std::string& username_);

    bool checkApp(const std::string& appname_);

    static bool existFunCode(const std::string& funcname, bool is_python = false);

    static boost::filesystem::path getFunCodePath(const std::string& funcname, bool is_python = false);

    wukong::proto::Function getFunction(const std::string& funcname);

    std::pair<bool, std::string> loadFuncCode(const std::string& funcname, bool is_python = false, bool update = false);

    std::pair<bool, std::string> initApp(const std::string& username_, const std::string& appname_);

    typedef std::string (*FP_Func)();

    static bool PingCode(const boost::filesystem::path& lib_path);

    void externalCall(const wukong::proto::Message& msg, Pistache::Http::ResponseWriter response);

    std::set<int> getReadFDs();
    std::set<int> geInternalRequestFDs();

    /// 作用是记录那个handler监听process
    struct Process
    {
        Process(std::shared_ptr<wukong::utils::DefaultSubProcess> process_,
                LocalGatewayClientHandler* handler_,
                int request_fd,
                int response_fd,
                int slots_)
            : sub_process(std::move(process_))
            , handler(handler_)
            , slots(slots_)
            , internal_request_fd(request_fd)
            , internal_response_fd(response_fd)
        { }

        Process(std::shared_ptr<wukong::utils::DefaultSubProcess> process_,
                LocalGatewayClientHandler* handler_,
                uint64_t free_size_)
            : sub_process(std::move(process_))
            , handler(handler_)
            , free_size(free_size_)
        { }

        std::shared_ptr<wukong::utils::DefaultSubProcess> sub_process;
        LocalGatewayClientHandler* handler;

        /// for worker-func
        std::atomic_int slots    = 0;
        int internal_request_fd  = -1;
        int internal_response_fd = -1;

        /// for storage-func
        uint64_t free_size = 0;
    };

    struct WorkerProcesses
    {
        std::list<std::shared_ptr<Process>> process_vector;
        std::atomic_int func_slots = 0;
        std::shared_mutex func_processes_shared_mutex;
    };

    WK_FUNC_RETURN_TYPE createWorkerFuncProcess(const wukong::proto::Function& func, Process** process, LocalGatewayClientHandler* handler);

    WK_FUNC_RETURN_TYPE takeWorkerFuncProcess(const std::string& funcname, Process** process, LocalGatewayClientHandler* handler);

    WK_FUNC_RETURN_TYPE backWorkerFuncProcess(const std::string& funcname, Process* process);

    WK_FUNC_RETURN_TYPE createStorageFuncProcess(size_t length, Process** process, LocalGatewayClientHandler* handler);

    WK_FUNC_RETURN_TYPE takeStorageFuncProcess(StorageFuncOpType type, const wukong::proto::Message& message, Process** process, LocalGatewayClientHandler* handler);
    WK_FUNC_RETURN_TYPE createShmDone(Process* process, const std::string& uuid, size_t length);
    WK_FUNC_RETURN_TYPE deleteShmDone(Process* process, size_t length);

    WK_FUNC_RETURN_TYPE getStorageShm(const std::string& uuid, std::string& result);

    std::string username() const
    {
        return username_;
    }

    std::string appname() const
    {
        return appname_;
    }

    int getInternalResponseFD(int internalRequestFD)
    {
        wukong::utils::ReadLock lock(internal_request_fd_2_response_fd_map_mutex);
        WK_CHECK_WITH_EXIT(internal_request_fd_2_response_fd_map.contains(internalRequestFD), "Dont find internalRequestFD");
        return internal_request_fd_2_response_fd_map.at(internalRequestFD);
    }

private:
    std::shared_ptr<LocalGatewayClientHandler> pickOneHandler();

    void killAllProcess();

    wukong::utils::Redis& redis = wukong::utils::Redis::getRedis();

    LocalGatewayEndpoint endpoint;
    wukong::client::ClientServer cs;

    std::string username_;
    std::string appname_;

    std::shared_mutex functions_mutex;
    std::unordered_map<std::string, wukong::proto::Function> functions;

    boost::filesystem::path worker_func_exec_path;
    boost::filesystem::path storage_func_exec_path;

    std::shared_mutex worker_processes_map_mutex;
    std::unordered_map<std::string, std::shared_ptr<WorkerProcesses>> worker_processes_map;

    std::mutex storageProcessesMutex;
    std::list<std::shared_ptr<Process>> storageProcessList;
    /// uuid -> <length, storageProcessList_index>
    std::unordered_map<std::string, std::pair<size_t, Process*>> uuid2StorageProcessMap;

    std::shared_mutex read_fd_set_mutex;
    std::set<int> read_fd_set;

    std::shared_mutex internal_request_fd_set_mutex;
    std::set<int> internal_request_fd_set;

    std::shared_mutex internal_request_fd_2_response_fd_map_mutex;
    std::unordered_map<int, int> internal_request_fd_2_response_fd_map;
};

#endif // WUKONG_LOCAL_GATEWAY_H