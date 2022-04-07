//
// Created by kingdo on 2022/3/27.
//

#include "Agent.h"

void AgentHandler::handlerMessage()
{
    for (;;)
    {
        auto entry = messageQueue.popSafe();
        if (!entry)
            break;
        auto msg_ptr = std::make_shared<wukong::proto::Message>(entry->msg);
        FaasHandle handle(msg_ptr, agent);
        agent->doExec(&handle);
        agent->finishExec(std::move(*msg_ptr));
    }
}

void AgentHandler::putMessage(wukong::proto::Message msg)
{
    MessageEntry entry(std::move(msg));
    messageQueue.push(std::move(entry));
}

void AgentHandler::onReady(const Pistache::Aio::FdSet& fds)
{
    for (auto fd : fds)
    {
        if (fd.getTag() == messageQueue.tag())
        {
            handlerMessage();
        }
    }
}

void AgentHandler::registerPoller(Pistache::Polling::Epoll& poller)
{
    messageQueue.bind(poller);
}

Agent::Options::Options()
    : threads_(1)
    , read_fd(wukong::utils::Config::InstanceFunctionDefaultReadFD())
    , max_read_buffer_size(wukong::utils::Config::InstanceFunctionReadBufferSize())
    , write_fd(wukong::utils::Config::InstanceFunctionDefaultWriteFD())
    , request_fd(wukong::utils::Config::InstanceFunctionDefaultInternalRequestFD())
    , response_fd(wukong::utils::Config::InstanceFunctionDefaultInternalResponseFD())
    , type_(C_PP)
{ }

Agent::Options Agent::Options::options()
{
    return {};
}

Agent::Options& Agent::Options::threads(int val)
{
    threads_ = val;
    return *this;
}

Agent::Options& Agent::Options::type(Agent::Type val)
{
    type_ = val;
    return *this;
}

Agent::Agent()
    : Reactor()
    , read_fd(-1)
    , max_read_buffer_size(0)
    , write_fd(-1)
    , request_fd(-1)
    , response_fd(-1)
    , type(C_PP)
    , lib()
{ }

void Agent::init(Agent::Options& options)
{
    loadFunc(options);
    type = options.type_;
    poller.addFd(read_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(read_fd),
                 Pistache::Polling::Mode::Edge);
    poller.addFd(response_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(response_fd),
                 Pistache::Polling::Mode::Edge);
    if (!writeQueue.isBound())
        writeQueue.bind(poller);
    if (!internalRequestQueue.isBound())
        internalRequestQueue.bind(poller);
    Reactor::init(options.threads_, "Work Function Agent");
}

void Agent::run()
{
    if (!handler_)
    {
        SPDLOG_ERROR("Please Set handler First");
        onFailed();
        return;
    }
    Reactor::run();
    onRunning();
}

void Agent::shutdown()
{
    if (func_entry)
        lib.close();
    Reactor::shutdown();
}

void Agent::doExec(FaasHandle* h)
{
    WK_CHECK_WITH_ASSERT(func_entry != nullptr, "func_entry is null");
    func_entry(h);
}

void Agent::finishExec(wukong::proto::Message msg)
{
    msg.set_finishtimestamp(wukong::utils::getMillsTimestamp());
    std::string msg_json   = wukong::proto::messageToJson(msg);
    std::string storageKey = msg.resultkey();
    auto& redis            = wukong::utils::Redis::getRedis();
    redis.set(storageKey, msg_json);
    writeQueue.push(std::move(msg));
}

void Agent::onRunning() const
{
    bool running = true;
    ::write(write_fd, &running, sizeof(running));
}

void Agent::onReady(const Pistache::Polling::Event& event)
{
    if (event.flags.hasFlag(Pistache::Polling::NotifyOn::Read))
    {
        auto fd = event.tag.value();
        if (static_cast<ssize_t>(fd) == read_fd)
        {
            try
            {
                handlerIncoming();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerIncoming error: {}", ex.what());
            }
        }
        if (static_cast<ssize_t>(fd) == response_fd)
        {
            try
            {
                handlerInternalResponse();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerIncoming error: {}", ex.what());
            }
        }
        if (event.tag == writeQueue.tag())
        {
            try
            {
                handlerWriteQueue();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerWriteQueue error: {}", ex.what());
            }
        }

        if (event.tag == internalRequestQueue.tag())
        {
            try
            {
                handlerInternalRequest();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerWriteQueue error: {}", ex.what());
            }
        }
    }
}

void Agent::onFailed() const
{
    bool running = false;
    ::write(write_fd, &running, sizeof(running));
}

void Agent::loadFunc(Agent::Options& options)
{
    read_fd              = options.read_fd;
    max_read_buffer_size = options.max_read_buffer_size;
    write_fd             = options.write_fd;
    request_fd           = options.request_fd;
    response_fd          = options.response_fd;
    FunctionInfo func_info;
    wukong::utils::nonblock_ioctl(read_fd, 0);
    wukong::utils::read_from_fd(read_fd, &func_info);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    auto lib_path = boost::filesystem::path(std::string { func_info.lib_path, 256 });
    WK_CHECK_WITH_ASSERT(exists(lib_path), "Please Right Set read_from_fd");
    WK_CHECK_WITH_ASSERT(func_info.threads >= 1, "func Concurrency < 1 ?");
    options.threads(func_info.threads);
    lib.open(lib_path.c_str());
    if (lib.sym("_Z9faas_mainP10FaasHandle", (void**)(&func_entry)))
    {
        SPDLOG_ERROR(fmt::format("load Code Error:{}", lib.errors()));
        assert(false);
    }
}

std::shared_ptr<AgentHandler> Agent::pickOneHandler()
{
    return std::static_pointer_cast<AgentHandler>(pickHandler());
}

void Agent::handlerWriteQueue()
{
    for (;;)
    {
        auto item = writeQueue.popSafe();
        if (!item)
            break;
        FuncResult result;
        result.success = true;
        result.msg_id  = item->id();
        memcpy(result.data, item->outputdata().data(), item->outputdata().size());
        //        SPDLOG_DEBUG("Write : {} {} {}", result.success, result.msg_id, result.data);
        wukong::utils::write_2_fd(write_fd, result);
    }
}

void Agent::handlerIncoming()
{
    std::string msg_json;
    msg_json.resize(max_read_buffer_size, 0);
    // TODO 缺少封装
    auto size = ::read(read_fd, msg_json.data(), max_read_buffer_size);
    if (size <= 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());
            // TODO handler error;
            return;
        }
        else
        {
            SPDLOG_WARN("get EAGAIN or EWOULDBLOCK");
            return;
        }
    }
    const auto& msg = wukong::proto::jsonToMessage(msg_json);
    auto handler    = pickOneHandler();
    handler->putMessage(msg);
}
void Agent::handlerInternalResponse()
{
    InternalResponse result;
    wukong::utils::read_from_fd(response_fd, &result);
    uint64_t request_id = result.request_id;
    if (!internalRequestDeferredMap.contains(request_id))
    {
        SPDLOG_ERROR("internalRequestDeferredMap don't contains requestID {}", request_id);
        return;
    }
    if (result.success)
    {
        internalRequestDeferredMap.at(request_id).resolve(std::string(result.data));
    }
    else
    {
        internalRequestDeferredMap.at(request_id).reject(std::string(result.data));
    }
    internalRequestDeferredMap.erase(request_id);
}
void Agent::handlerInternalRequest()
{
    for (;;)
    {
        auto item = internalRequestQueue.popSafe();
        if (!item)
            break;
        InternalRequest request;
        memcpy(request.funcname, item->funcname.data(), item->funcname.size());
        memcpy(request.args, item->args.data(), item->args.size());
        request.request_id = item->request_id;
        wukong::utils::write_2_fd(request_fd, request);
        internalRequestDeferredMap.emplace(item->request_id, std::move(item->deferred));
    }
}
