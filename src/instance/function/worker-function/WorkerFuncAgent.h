//
// Created by kingdo on 2022/3/27.
//

#ifndef WUKONG_WORKERFUNCAGENT_H
#define WUKONG_WORKERFUNCAGENT_H

#include <boost/filesystem.hpp>
#include <faas/cpp/function-interface.h>
#include <faas/python/function-interface.h>
#include <pistache/async.h>
#include <pistache/http.h>
#include <pistache/reactor.h>
#include <python3.8/Python.h>
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

        Options& threads(int val);

    private:
        int threads_;
        int read_fd;
        uint64_t max_read_buffer_size;
        int write_fd;
        int request_fd;
        int response_fd;
    };

    WorkerFuncAgent();

    void init(Options& options);

    void run() override;

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

    void loadFunc(Options& options);

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
    uint64_t max_read_buffer_size;
    int write_fd;
    Pistache::PollableQueue<wukong::proto::Message> writeQueue;
    Pistache::PollableQueue<internalRequestEntry> internalRequestQueue;

    /// 不用上锁，因为是单线程
    std::unordered_map<uint64_t, Pistache::Async::Deferred<std::string>> internalRequestDeferredMap;

    int request_fd;
    int response_fd;

    FunctionType type;

    bool loaded = false;

    wukong::utils::Lib lib;
    Faas_Main func_entry = nullptr;

    PyObject* py_func_entry  = nullptr;
    PyObject* py_func_module = nullptr;
};

void link();

#endif // WUKONG_WORKERFUNCAGENT_H
