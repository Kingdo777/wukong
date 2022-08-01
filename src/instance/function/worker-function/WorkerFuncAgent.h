//
// Created by kingdo on 2022/3/27.
//

#ifndef WUKONG_WORKER_FUNC_AGENT_H
#define WUKONG_WORKER_FUNC_AGENT_H

#include <Python.h>
#include <boost/filesystem.hpp>
#include <faas/cpp/function-interface.h>
#include <faas/python/function-interface.h>
#include <pistache/async.h>
#include <pistache/http.h>
#include <pistache/reactor.h>
#include <queue>
#include <utility>
#include <wukong/proto/proto.h>
#include <wukong/utils/config.h>
#include <wukong/utils/dl.h>
#include <wukong/utils/errors.h>
#include <wukong/utils/log.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/os.h>
#include <wukong/utils/reactor/Reactor.h>
#include <wukong/utils/redis.h>
#include <wukong/utils/struct.h>

typedef void (*Faas_Main)(FaasHandle*);

class WorkerFuncAgent;

class AgentHandler : public Pistache::Aio::Handler
{
public:
    PROTOTYPE_OF(Pistache::Aio::Handler, AgentHandler)

    explicit AgentHandler(WorkerFuncAgent* agent_)
        : agent(agent_) {};

    AgentHandler(const AgentHandler& handler)
        : agent(handler.agent)
    { }

    void onReady(const Pistache::Aio::FdSet& fds) override;

    void registerPoller(Pistache::Polling::Epoll& poller) override;

    struct MessageEntry
    {
        explicit MessageEntry(wukong::proto::Message msg_)
            : msg(std::move(msg_))
        { }
        wukong::proto::Message msg;
    };

    void putMessage(wukong::proto::Message msg);

private:
    void handlerMessage();

    WorkerFuncAgent* agent;
    Pistache::PollableQueue<MessageEntry> messageQueue;
};

class WorkerFuncAgent : public Reactor
{
public:
    struct Options
    {
        friend class WorkerFuncAgent;

        Options();

        static Options options();

        Options& threads(uint32_t val);

        Options& fds(int read_fd_, int write_fd_, int request_fd_, int response_fd_);

        Options& funcPath(const boost::filesystem::path& path);

        Options& funcType(FunctionType type_);

    private:
        uint32_t threads_;

        int read_fd;
        int write_fd;
        int request_fd;
        int response_fd;

        uint64_t max_read_buffer_size;

        boost::filesystem::path func_path;

        FunctionType func_type = FunctionType::Cpp;
    };

    explicit WorkerFuncAgent(boost::filesystem::path func_path, FunctionType type)
        : Reactor()
        , read_fd(-1)
        , write_fd(-1)
        , request_fd(-1)
        , response_fd(-1)
        , max_read_buffer_size(0)
        , type(type)
        , func_path(std::move(func_path))
        , lib()
    { }

    WorkerFuncAgent(const WorkerFuncAgent& agent)
        : Reactor()
        , read_fd(-1)
        , write_fd(-1)
        , request_fd(-1)
        , response_fd(-1)
        , max_read_buffer_size(0)
        , type(agent.type)
        , func_path(agent.func_path)
        , lib(agent.lib)
        , func_entry(agent.func_entry)
        , py_func_entry(agent.py_func_entry)
        , py_func_module(agent.py_func_module)
    { }

    WorkerFuncAgent();

    void init(Options& options);

    void run() override;

    void loadFunc();

    void shutdown() override;

    void doExec(FaasHandle* h);

    void finishExec(wukong::proto::Message msg);

    void internalCall(const std::string& func, const std::string& args, uint64_t request_id, Pistache::Async::Deferred<std::string> deferred);

    struct internalRequestEntry
    {
        internalRequestEntry(std::string func, std::string args_, uint64_t request_id_, Pistache::Async::Deferred<std::string> deferred_)
            : funcname(std::move(func))
            , args(std::move(args_))
            , request_id(request_id_)
            , deferred(std::move(deferred_))
        { }
        std::string funcname;
        std::string args;
        uint64_t request_id;
        Pistache::Async::Deferred<std::string> deferred;
    };

private:
    void onRunning() const;

    void onReady(const Pistache::Polling::Event& event) override;

    void onFailed() const;

    bool isLoaded() const
    {
        return loaded;
    }

    bool isPython() const
    {
        return type == FunctionType::Python;
    }

    std::shared_ptr<AgentHandler> pickOneHandler();

    void handlerInternalRequest();

    void handlerWriteQueue();

    void handlerInternalResponse();

    void handlerIncoming();

    int read_fd;
    int write_fd;
    int request_fd;
    int response_fd;
    uint64_t max_read_buffer_size;

    std::mutex toWriteResult_mutex;
    std::queue<std::shared_ptr<wukong::proto::Message>> toWriteResult;

    std::mutex toCallInternalRequest_mutex;
    std::queue<std::shared_ptr<internalRequestEntry>> toCallInternalRequest;

    FunctionType type;
    boost::filesystem::path func_path;

    /// 不用上锁，因为是单线程
    std::unordered_map<uint64_t, Pistache::Async::Deferred<std::string>> internalRequestDeferredMap;

    bool loaded = false;

    wukong::utils::Lib lib;
    Faas_Main func_entry = nullptr;

    PyObject* py_func_entry  = nullptr;
    PyObject* py_func_module = nullptr;
};

[[maybe_unused]] void link();

#endif // WUKONG_WORKER_FUNC_AGENT_H
