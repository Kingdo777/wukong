//
// Created by kingdo on 2022/3/27.
//

#include "WorkerFuncAgent.h"

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

WorkerFuncAgent::Options::Options()
    : threads_(1)
    , workers_(1)
    , read_fd(wukong::utils::Config::InstanceFunctionDefaultReadFD())
    , write_fd(wukong::utils::Config::InstanceFunctionDefaultWriteFD())
    , request_fd(wukong::utils::Config::InstanceFunctionDefaultInternalRequestFD())
    , response_fd(wukong::utils::Config::InstanceFunctionDefaultInternalResponseFD())
    , max_read_buffer_size(wukong::utils::Config::InstanceFunctionReadBufferSize())

{ }

WorkerFuncAgent::Options WorkerFuncAgent::Options::options()
{
    return {};
}

WorkerFuncAgent::Options& WorkerFuncAgent::Options::threads(int val)
{
    threads_ = val;
    return *this;
}

WorkerFuncAgent::Options& WorkerFuncAgent::Options::workers(int val)
{
    threads_ = val;
    return *this;
}

WorkerFuncAgent::Options& WorkerFuncAgent::Options::fds(int read_fd_, int write_fd_, int request_fd_, int response_fd_)
{

    read_fd     = read_fd_;
    write_fd    = write_fd_;
    request_fd  = request_fd_;
    response_fd = response_fd_;
    return *this;
}
WorkerFuncAgent::Options& WorkerFuncAgent::Options::funcPath(const boost::filesystem::path& path)
{
    func_path = path;
    return *this;
}
WorkerFuncAgent::Options& WorkerFuncAgent::Options::funcType(FunctionType type_)
{
    func_type = type_;
    return *this;
}

WorkerFuncAgent::WorkerFuncAgent()
    : Reactor()
    , read_fd(-1)
    , write_fd(-1)
    , request_fd(-1)
    , response_fd(-1)
    , max_read_buffer_size(0)
    , type(FunctionType::Cpp)
    , lib()
{ }

void WorkerFuncAgent::init(WorkerFuncAgent::Options& options)
{
    loadFunc(options);
    poller.addFd(read_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(read_fd),
                 Pistache::Polling::Mode::Edge);
    poller.addFdOneShot(write_fd,
                        Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                        Pistache::Polling::Tag(write_fd),
                        Pistache::Polling::Mode::Edge);
    poller.addFd(response_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(response_fd),
                 Pistache::Polling::Mode::Edge);
    poller.addFdOneShot(request_fd,
                        Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                        Pistache::Polling::Tag(request_fd),
                        Pistache::Polling::Mode::Edge);
    Reactor::init(options.threads_, "Work Function Agent");
}

void WorkerFuncAgent::run()
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

void WorkerFuncAgent::shutdown()
{
    if (isLoaded())
    {
        switch (type)
        {
        case Cpp: {
            lib.close();
        }
        case Python: {
            Py_XDECREF(py_func_entry);
            Py_DECREF(py_func_module);
            Py_FinalizeEx();
            break;
        }
        case WebAssembly:
        case StorageFunc: {
            break;
        }
        }
    }

    Reactor::shutdown();
}

void WorkerFuncAgent::doExec(FaasHandle* h)
{
    switch (type)
    {
    case Cpp: {
        WK_CHECK_WITH_EXIT(func_entry != nullptr, "func_entry is null");
        func_entry(h);
        break;
    }
    case Python: {
        WK_CHECK_WITH_EXIT(py_func_entry != nullptr, "func_entry is null");
        PyObject* pythonFuncArgs = PyTuple_New(1);
        PyTuple_SetItem(pythonFuncArgs, 0, PyLong_FromVoidPtr(h));
        PyObject* returnValue = PyObject_CallObject(py_func_entry, pythonFuncArgs);
        if (PyErr_Occurred())
        {
            PyErr_Print();
        }
        if (pythonFuncArgs)
            Py_DECREF(pythonFuncArgs);
        Py_DECREF(returnValue);
        break;
    }
    case WebAssembly: {
        break;
    }
    case StorageFunc: {
        WK_CHECK_WITH_EXIT(false, "Unreachable");
    }
    }
}

void WorkerFuncAgent::finishExec(wukong::proto::Message msg)
{
    msg.set_finishtimestamp(wukong::utils::getMillsTimestamp());
    std::string msg_json   = wukong::proto::messageToJson(msg);
    std::string storageKey = msg.resultkey();
    auto& redis            = wukong::utils::Redis::getRedis();
    redis.set(storageKey, msg_json);
    toWriteResult.push(std::make_shared<wukong::proto::Message>(std::move(msg)));
    poller.rearmFd(write_fd,
                   Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                   Pistache::Polling::Tag(write_fd),
                   Pistache::Polling::Mode::Edge);
}

void WorkerFuncAgent::internalCall(const std::string& func, const std::string& args, uint64_t request_id, Pistache::Async::Deferred<std::string> deferred)
{
    toCallInternalRequest.emplace(std::make_shared<internalRequestEntry>(func, args, request_id, std::move(deferred)));
    poller.rearmFd(request_fd,
                   Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                   Pistache::Polling::Tag(request_fd),
                   Pistache::Polling::Mode::Edge);
}

void WorkerFuncAgent::onRunning() const
{
    bool running = true;
    SPDLOG_DEBUG("onRunning");
    WRITE_2_FD_original(write_fd, &running, sizeof(running));
}

void WorkerFuncAgent::onReady(const Pistache::Polling::Event& event)
{
    int fd = (int)event.tag.value();
    if (event.flags.hasFlag(Pistache::Polling::NotifyOn::Read))
    {
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
        else if (static_cast<ssize_t>(fd) == response_fd)
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
    }
    if (event.flags.hasFlag(Pistache::Polling::NotifyOn::Write))
    {
        if (fd == write_fd)
        {
            try
            {
                handlerWriteQueue();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerFuncCreateDoneQueue error: {}", ex.what());
            }
        }

        else if (fd == request_fd)
        {
            try
            {
                handlerInternalRequest();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerFuncCreateDoneQueue error: {}", ex.what());
            }
        }
    }
}

void WorkerFuncAgent::onFailed() const
{
    bool running = false;
    WRITE_2_FD_original(write_fd, &running, sizeof(running));
}

void WorkerFuncAgent::loadFunc(WorkerFuncAgent::Options& options)
{
    read_fd              = options.read_fd;
    write_fd             = options.write_fd;
    request_fd           = options.request_fd;
    response_fd          = options.response_fd;
    max_read_buffer_size = options.max_read_buffer_size;

    type      = options.func_type;
    func_path = options.func_path;

    wukong::utils::nonblock_ioctl(write_fd, 1);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    wukong::utils::nonblock_ioctl(request_fd, 1);
    wukong::utils::nonblock_ioctl(response_fd, 1);

    switch (type)
    {
    case Cpp: {
        lib.open(func_path.c_str());
        if (lib.sym("_Z9faas_mainP10FaasHandle", (void**)(&func_entry)))
        {
            SPDLOG_ERROR(fmt::format("load Code Error:{}", lib.errors()));
            assert(false);
        }
        break;
    }
    case Python: {
        Py_InitializeEx(0);
        const boost::filesystem::path& workingDir(func_path);
        PyObject* sys  = PyImport_ImportModule("sys");
        PyObject* path = PyObject_GetAttrString(sys, "path");
        PyList_Append(path, PyUnicode_FromString(workingDir.parent_path().c_str()));
        auto file        = workingDir.filename().string();
        auto module_name = file.substr(0, file.size() - strlen(".py"));
        py_func_module   = PyImport_ImportModule(module_name.c_str());
        if (PyErr_Occurred())
        {
            PyErr_Print();
        }
        WK_CHECK_WITH_EXIT(py_func_module, fmt::format("Failed to load module `{}`", module_name));
        py_func_entry = PyObject_GetAttrString(py_func_module, "faas_main");
        if (PyErr_Occurred())
        {
            PyErr_Print();
        }
        WK_CHECK_WITH_EXIT(py_func_module, fmt::format("Failed to load Function `faas_main` from module `{}`", module_name));
        break;
    }
    case WebAssembly:
    case StorageFunc: {
        break;
    }
    }

    loaded = true;
}

std::shared_ptr<AgentHandler> WorkerFuncAgent::pickOneHandler()
{
    return std::static_pointer_cast<AgentHandler>(pickHandler());
}

void WorkerFuncAgent::handlerWriteQueue()
{
    for (;;)
    {
        if (toWriteResult.empty())
            break;
        auto msg = toWriteResult.front();
        FuncResult result;
        result.magic_number = MAGIC_NUMBER_WUKONG;
        result.success      = true;
        result.request_id   = msg->id();
        memcpy(result.data, msg->outputdata().data(), msg->outputdata().size());
        result.data_size = msg->outputdata().size();
        WRITE_2_FD_goto(write_fd, result);
        toWriteResult.pop();
    }
write_fd_EAGAIN:
    poller.rearmFd(write_fd,
                   Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                   Pistache::Polling::Tag(write_fd),
                   Pistache::Polling::Mode::Edge);
}

void WorkerFuncAgent::handlerIncoming()
{
    for (;;)
    {
        std::string msg_json;
        msg_json.resize(max_read_buffer_size, 0);
        // TODO 缺少封装
        READ_FROM_FD_original_goto(read_fd, msg_json.data(), max_read_buffer_size);
        const auto& msg = wukong::proto::jsonToMessage(msg_json);
        auto handler    = pickOneHandler();
        handler->putMessage(msg);
    }
read_fd_EAGAIN:;
}
void WorkerFuncAgent::handlerInternalResponse()
{
    for (;;)
    {
        FuncResult result;
        READ_FROM_FD_goto(response_fd, &result);
        uint64_t request_id = result.request_id;
        if (!internalRequestDeferredMap.contains(request_id))
        {
            SPDLOG_ERROR("internalRequestDeferredMap don't contains requestID {}", request_id);
            return;
        }
        if (result.success)
        {
            internalRequestDeferredMap.at(request_id).resolve(std::string(result.data, result.data_size));
        }
        else
        {
            internalRequestDeferredMap.at(request_id).reject(std::string(result.data, result.data_size));
        }
        internalRequestDeferredMap.erase(request_id);
    }
read_fd_EAGAIN:;
}
void WorkerFuncAgent::handlerInternalRequest()
{
    for (;;)
    {
        if (toCallInternalRequest.empty())
            break;
        auto item = toCallInternalRequest.front();
        InternalRequest request;
        request.magic_number = MAGIC_NUMBER_WUKONG;
        memcpy(request.funcname, item->funcname.data(), item->funcname.size());
        memcpy(request.args, item->args.data(), item->args.size());
        request.request_id = item->request_id;
        WRITE_2_FD_goto(request_fd, request);
        toCallInternalRequest.pop();
        internalRequestDeferredMap.emplace(item->request_id, std::move(item->deferred));
    }
write_fd_EAGAIN:
    poller.rearmFd(request_fd,
                   Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                   Pistache::Polling::Tag(request_fd),
                   Pistache::Polling::Mode::Edge);
}

void link()
{
    interface_link();
}