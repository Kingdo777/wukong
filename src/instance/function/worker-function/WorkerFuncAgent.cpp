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
    , read_fd(wukong::utils::Config::InstanceFunctionDefaultReadFD())
    , max_read_buffer_size(wukong::utils::Config::InstanceFunctionReadBufferSize())
    , write_fd(wukong::utils::Config::InstanceFunctionDefaultWriteFD())
    , request_fd(wukong::utils::Config::InstanceFunctionDefaultInternalRequestFD())
    , response_fd(wukong::utils::Config::InstanceFunctionDefaultInternalResponseFD())
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

WorkerFuncAgent::WorkerFuncAgent()
    : Reactor()
    , read_fd(-1)
    , max_read_buffer_size(0)
    , write_fd(-1)
    , request_fd(-1)
    , response_fd(-1)
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
    }
}

void WorkerFuncAgent::finishExec(wukong::proto::Message msg)
{
    msg.set_finishtimestamp(wukong::utils::getMillsTimestamp());
    std::string msg_json   = wukong::proto::messageToJson(msg);
    std::string storageKey = msg.resultkey();
    auto& redis            = wukong::utils::Redis::getRedis();
    redis.set(storageKey, msg_json);
    writeQueue.push(std::move(msg));
}

void WorkerFuncAgent::internalCall(const std::string& func, const std::string& args, uint64_t request_id, Pistache::Async::Deferred<std::string> deferred)
{
    internalRequestEntry entry(func, args, request_id, std::move(deferred));
    internalRequestQueue.push(std::move(entry));
}

void WorkerFuncAgent::onRunning() const
{
    bool running = true;
    ::write(write_fd, &running, sizeof(running));
}

void WorkerFuncAgent::onReady(const Pistache::Polling::Event& event)
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
        else if (event.tag == writeQueue.tag())
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

        else if (event.tag == internalRequestQueue.tag())
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

void WorkerFuncAgent::onFailed() const
{
    bool running = false;
    ::write(write_fd, &running, sizeof(running));
}

void WorkerFuncAgent::loadFunc(WorkerFuncAgent::Options& options)
{
    read_fd              = options.read_fd;
    max_read_buffer_size = options.max_read_buffer_size;
    write_fd             = options.write_fd;
    request_fd           = options.request_fd;
    response_fd          = options.response_fd;
    FunctionInfo func_info;
    wukong::utils::nonblock_ioctl(read_fd, 0);
    wukong::utils::read_from_fd(read_fd, &func_info);
    type = func_info.type;
    WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(func_info.magic_number), "Data Wrong, Magic Check Failed!");

    wukong::utils::nonblock_ioctl(write_fd, 0);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    wukong::utils::nonblock_ioctl(request_fd, 0);
    wukong::utils::nonblock_ioctl(response_fd, 1);

    auto func_path = boost::filesystem::path(std::string { func_info.func_path, func_info.path_size });
    WK_CHECK_WITH_EXIT(exists(func_path), fmt::format("{} is not exists", func_path.string()));
    WK_CHECK_WITH_EXIT(func_info.threads >= 1, "func Concurrency < 1 ?");
    options.threads(func_info.threads);

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
        auto item = writeQueue.popSafe();
        if (!item)
            break;
        FuncResult result;
        result.magic_number = MAGIC_NUMBER_WUKONG;
        result.success      = true;
        result.request_id   = item->id();
        memcpy(result.data, item->outputdata().data(), item->outputdata().size());
        result.data_size = item->outputdata().size();
        wukong::utils::write_2_fd(write_fd, result);
    }
}

void WorkerFuncAgent::handlerIncoming()
{
    for (;;)
    {
        std::string msg_json;
        msg_json.resize(max_read_buffer_size, 0);
        // TODO 缺少封装
        auto size = ::read(read_fd, msg_json.data(), max_read_buffer_size);
        if (size == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());
                // TODO handler error;
            }
            return;
        }
        const auto& msg = wukong::proto::jsonToMessage(msg_json);
        auto handler    = pickOneHandler();
        handler->putMessage(msg);
    }
}
void WorkerFuncAgent::handlerInternalResponse()
{
    for (;;)
    {
        FuncResult result;
        auto ret = wukong::utils::read_from_fd(response_fd, &result);
        if (ret == -1)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                SPDLOG_ERROR("read fd is wrong : {}", wukong::utils::errors());
                // TODO handler error;
            }
            return;
        }
        WK_CHECK(ret == sizeof(result), "read_from_fd failed");
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
}
void WorkerFuncAgent::handlerInternalRequest()
{
    for (;;)
    {
        auto item = internalRequestQueue.popSafe();
        if (!item)
            break;
        InternalRequest request;
        request.magic_number = MAGIC_NUMBER_WUKONG;
        memcpy(request.funcname, item->funcname.data(), item->funcname.size());
        memcpy(request.args, item->args.data(), item->args.size());
        request.request_id = item->request_id;
        wukong::utils::write_2_fd(request_fd, request);
        internalRequestDeferredMap.emplace(item->request_id, std::move(item->deferred));
    }
}

void link()
{
    interface_link();
}