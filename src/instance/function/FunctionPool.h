//
// Created by kingdo on 22-7-12.
//

#ifndef WUKONG_FUNCTION_POOL_H
#define WUKONG_FUNCTION_POOL_H

#include "storage-function/StorageFuncAgent.h"
#include "worker-function/WorkerFuncAgent.h"
#include <fmt/format.h>
#include <pistache/mailbox.h>
#include <utility>
#include <vector>
#include <wukong/utils/cgroup/CGroup.h>
#include <wukong/utils/config.h>
#include <wukong/utils/errors.h>
#include <wukong/utils/os.h>
#include <wukong/utils/process/Process.h>
#include <wukong/utils/radom.h>
#include <wukong/utils/reactor/Reactor.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/struct.h>

class FunctionPool;

class FuncInstProcess : public wukong::utils::Process
{
public:
    std::string funcInst_uuid;

    FunctionInstanceType instType;

    FunctionType funcType;

    std::string funcname;
    boost::filesystem::path func_path;

    uint32_t threads = 1;

    std::array<boost::filesystem::path, pipeCount> pipeArray;
};

struct FuncInstProcessCGroup
{
    FuncInstProcessCGroup(std::string funcname_, uint32_t cores, uint32_t memory, uint32_t workers, FunctionInstanceType instanceType)
        : funcname(std::move(funcname_))
        , cgroup_name(fmt::format("{}-{}", funcname, wukong::utils::randomString(8)))
        , memCGroup(cgroup_name)
        , cpuCGroup(cgroup_name)
        , workers_count(workers)
        , instType(instanceType)
    {
        memCGroup.setHardLimit(memory);
        cpuCGroup.setCPUS(cores);
    }
    std::string funcname;

    std::string funcInst_uuid_prefix;
    std::array<std::string, pipeCount> pipeArray_prefix;

    std::string cgroup_name;
    wukong::utils::MemoryCGroup memCGroup;
    wukong::utils::CpuCGroup cpuCGroup;

    std::vector<std::shared_ptr<FuncInstProcess>> process_list;
    uint32_t workers_count = 0;

    FunctionInstanceType instType;
};

class FunctionPoolHandler : public Pistache::Aio::Handler
{
public:
    PROTOTYPE_OF(Pistache::Aio::Handler, FunctionPoolHandler)

    explicit FunctionPoolHandler(FunctionPool* fp_);

    FunctionPoolHandler(const FunctionPoolHandler& handler);

    void onReady(const Pistache::Aio::FdSet& fds) override;

    void registerPoller(Pistache::Polling::Epoll& poller) override;

    void createFunc(const FuncCreateMsg& msg);
    static std::string getFuncnameFromCodePath(const boost::filesystem::path& path);

private:
    static std::string makeInstUUID(const std::string& funcname, FunctionType type);
    void handlerFuncCreateReq();

    FunctionPool* fp;

    Pistache::PollableQueue<FuncCreateMsg> funcCreateReqQueue;
};

class FunctionPool : public Reactor
{
public:
    struct Options
    {
        friend class FunctionPool;

        Options();

        static Options options();

        Options& threads(int val);

    private:
        int threads_;
    };

    FunctionPool();

    void init(Options& options);

    void run() override;

    void shutdown() override;

    void connectLG() const;

    void createFuncDone(const std::shared_ptr<FuncInstProcessCGroup>& processCGroup);

    void addInstanceCGroup(const std::shared_ptr<FuncInstProcessCGroup>& processCGroup);

private:
    std::shared_ptr<FunctionPoolHandler> pickOneHandler();

    void onRunning() const;
    void onFailed() const;
    void onReady(const Pistache::Polling::Event& event) override;

    void handlerIncoming();

    void handlerFuncCreateDoneQueue();

private:
    int read_fd;
    int write_fd;

    std::queue<FuncCreateDoneMsg> funcCreateDoneMsgQueue;

    std::mutex funcInstMapMutex;
    std::unordered_map<std::string, std::shared_ptr<FuncInstProcessCGroup>> funcInstMap;
};

#endif // WUKONG_FUNCTION_POOL_H
