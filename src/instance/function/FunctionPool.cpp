//
// Created by kingdo on 22-7-12.
//

#include "FunctionPool.h"
FunctionPoolHandler::FunctionPoolHandler(FunctionPool* fp_)
    : fp(fp_)
{ }
FunctionPoolHandler::FunctionPoolHandler(const FunctionPoolHandler& handler)
    : fp(handler.fp)
{ }
void FunctionPoolHandler::onReady(const Pistache::Aio::FdSet& fds)
{
    for (auto fd : fds)
    {
        if (fd.getTag() == funcCreateReqQueue.tag())
        {
            handlerFuncCreateReq();
        }
    }
}
void FunctionPoolHandler::registerPoller(Pistache::Polling::Epoll& poller)
{
    funcCreateReqQueue.bind(poller);
}
void FunctionPoolHandler::createFunc(const FuncCreateMsg& msg)
{
    funcCreateReqQueue.push(msg);
}
std::string FunctionPoolHandler::getFuncnameFromCodePath(const boost::filesystem::path& path)
{

    if (path.parent_path().filename() == "python")
    {
        auto filename     = path.filename().string();
        size_t suffix_len = std::string { ".py" }.size();
        return filename.substr(0, filename.size() - suffix_len);
    }
    else
    {
        return path.parent_path().filename().string();
    }
}
std::string FunctionPoolHandler::makeInstUUID(const std::string& funcname, FunctionType type)
{
    switch (type)
    {
    case Python: {
        return fmt::format("{}#{}#{}", funcname, "Python", wukong::utils::randomString(8));
    }
    case Cpp: {
        return fmt::format("{}#{}#{}", funcname, "CPP", wukong::utils::randomString(8));
    }
    case WebAssembly: {
        return fmt::format("{}#{}#{}", funcname, "WebAssembly", wukong::utils::randomString(8));
    }
    case StorageFunc: {
        return fmt::format("{}#{}", STORAGE_FUNCTION_NAME, wukong::utils::randomString(8));
    }
    }
    WK_CHECK_WITH_EXIT(false, "Unreachable");
    return "";
}
void FunctionPoolHandler::handlerFuncCreateReq()
{
    for (;;)
    {
        auto msg = funcCreateReqQueue.popSafe();
        if (!msg)
            break;
        const auto& funcname = std::string { msg->funcname, msg->funcname_size };
        uint32_t workers     = msg->workers;
        WK_CHECK_WITH_EXIT(workers >= 1, "workers < 1 ?");

        auto processCGroup                  = std::make_shared<FuncInstProcessCGroup>(funcname, msg->cores, msg->memory, workers, msg->instanceType);
        auto uuid                           = makeInstUUID(funcname, msg->type);
        processCGroup->funcInst_uuid_prefix = uuid;

        for (int i = 0; i < PipeIndex::pipeCount; i++)
        {
            processCGroup->pipeArray_prefix[i] = boost::filesystem::path(NAMED_PIPE_PATH).append(uuid).append(PipeNameString[i]).string();
        }

        for (uint32_t worker_index = 0; worker_index < workers; ++worker_index)
        {
            auto process = std::make_shared<FuncInstProcess>();
            processCGroup->process_list.emplace_back(process);

            process->funcname      = funcname;
            process->funcInst_uuid = fmt::format("{}-{}", processCGroup->funcInst_uuid_prefix, worker_index);
            process->funcType      = msg->type;
            process->instType      = msg->instanceType;
            if (process->instType == WorkerFunction)
            {
                process->func_path = getFunCodePath(process->funcname, msg->type);
                WK_CHECK_WITH_EXIT(exists(process->func_path), fmt::format("{} is not exists", process->func_path.string()));
            }
            process->threads = msg->threads;
            WK_CHECK_WITH_EXIT(process->threads >= 1, "func Concurrency < 1 ?");

            /// create Named-Pipe
            for (int i = 0; i < PipeIndex::pipeCount; i++)
            {
                auto fd_path = boost::filesystem::path(processCGroup->pipeArray_prefix[i]).append(std::to_string(worker_index));
                if (!exists(fd_path.parent_path()))
                    create_directories(fd_path.parent_path());
                process->pipeArray[i] = fd_path;
                if (exists(fd_path))
                {
                    SPDLOG_WARN("May Be Wrong!!!!!!!1111");
                    continue;
                }
                int ret = ::mkfifo(fd_path.c_str(), WUKONG_FILE_CREAT_MODE);
                if (ret == -1)
                {
                    perror("Create Named-Pipe Failed");
                    WK_CHECK_WITH_EXIT(false, "Create Named-Pipe Failed");
                }
            }

            SPDLOG_DEBUG("Creating Func Instance for {} ...", process->funcname);
            process->pid = fork();
            WK_CHECK_WITH_EXIT(process->pid != -1, "fork function Instance Wrong");

            /// sub-process
            if (process->pid == 0)
            {
                if (process->instType == WorkerFunction)
                {
                    wukong::utils::initLog(fmt::format("FunctionPool/WorkerFunction/{}/{}", process->funcname, getpid()));
                    SPDLOG_INFO("-------------------worker func config---------------------");
                    wukong::utils::Config::print();

                    /// open the pipe
                    /**
                     * From the Open Group page for the open() function:
                     * O_NONBLOCK
                        When opening a FIFO with O_RDONLY or O_WRONLY set:
                        If O_NONBLOCK is set:
                            An open() for reading only will return without delay. An open()
                            for writing only will return an error if no process currently
                            has the file open for reading.
                        If O_NONBLOCK is clear:
                            An open() for reading only will block the calling thread until a
                            thread opens the file for writing. An open() for writing only
                            will block the calling thread until a thread opens the file for
                            reading.
                     * */

                    int read_fd     = open(process->pipeArray[PipeIndex::read_writePipePath].c_str(), O_RDONLY | O_NONBLOCK);
                    int write_fd    = open(process->pipeArray[PipeIndex::write_readPipePath].c_str(), O_WRONLY);
                    int response_fd = open(process->pipeArray[PipeIndex::response_requestPipePath].c_str(), O_RDONLY | O_NONBLOCK);
                    int request_fd  = open(process->pipeArray[PipeIndex::request_responsePipePath].c_str(), O_WRONLY);

                    wukong::utils::nonblock_ioctl(write_fd, 1);
                    wukong::utils::nonblock_ioctl(request_fd, 1);

                    SIGNAL_HANDLER()
                    WorkerFuncAgent agent;
                    auto opts = WorkerFuncAgent::Options::options().threads(process->threads).funcPath(process->func_path).funcType(process->funcType).fds(read_fd, write_fd, request_fd, response_fd);
                    agent.init(opts);
                    agent.set_handler(std::make_shared<AgentHandler>(&agent));
                    agent.run();
                    SIGNAL_WAIT()
                    agent.shutdown();
                }
                else if (process->instType == StorageFunction)
                {
                    wukong::utils::initLog(fmt::format("FunctionPool/StorageFunction/{}/{}", STORAGE_FUNCTION_NAME, getpid()));
                    SPDLOG_INFO("-------------------storage func config---------------------");
                    wukong::utils::Config::print();

                    int read_fd  = open(process->pipeArray[PipeIndex::read_writePipePath].c_str(), O_RDONLY | O_NONBLOCK);
                    int write_fd = open(process->pipeArray[PipeIndex::write_readPipePath].c_str(), O_WRONLY);

                    SIGNAL_HANDLER()
                    StorageFuncAgent agent(read_fd, write_fd);
                    agent.run();
                    bool running = true;
                    WRITE_2_FD_original(write_fd, &running, sizeof(running));
                    SIGNAL_WAIT()
                    agent.shutdown();
                }
                else
                {
                    WK_CHECK(false, "Unknown Instance Type");
                }
                exit(0);
            }

            /// parent-process

            /// Waiting Create
            int read_fd_parent = ::open(process->pipeArray[PipeIndex::write_readPipePath].c_str(), O_RDONLY);
            if (process->instType == WorkerFunction)
            {
                int response_fd_parent = ::open(process->pipeArray[PipeIndex::request_responsePipePath].c_str(), O_RDONLY);
                ::close(response_fd_parent);
            }
            bool success;
            READ_FROM_FD(read_fd_parent, &success);
            WK_CHECK_WITH_EXIT(success, "Create FuncInst Failed");
            ::close(read_fd_parent);

            SPDLOG_DEBUG("Create Func Instance for {} Done", process->funcname);
        }

        /// add to func-map
        fp->addInstanceCGroup(processCGroup);

        /// Send DoneMsg to LG
        fp->createFuncDone(processCGroup);
    }
}
FunctionPool::Options::Options()
    : threads_(1)
{ }
FunctionPool::Options FunctionPool::Options::options()
{
    return {};
}
FunctionPool::Options& FunctionPool::Options::threads(int val)
{
    threads_ = val;
    return *this;
}
FunctionPool::FunctionPool()
    : read_fd(wukong::utils::Config::InstanceFunctionDefaultReadFD())
    , write_fd(wukong::utils::Config::InstanceFunctionDefaultWriteFD())
{
    wukong::utils::nonblock_ioctl(write_fd, 1);
    connectLG();
}
void FunctionPool::init(FunctionPool::Options& options)
{
    poller.addFd(read_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(read_fd),
                 Pistache::Polling::Mode::Edge);
    poller.addFdOneShot(write_fd,
                        Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                        Pistache::Polling::Tag(write_fd),
                        Pistache::Polling::Mode::Edge);

    Reactor::init(options.threads_, "Work Function fp");
}
void FunctionPool::run()
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

void FunctionPool::shutdown()
{
    system(fmt::format("rm -rf {}", NAMED_PIPE_PATH).c_str());
    Reactor::shutdown();
}

void FunctionPool::connectLG() const
{
    GreetingMsg msg;
    wukong::utils::nonblock_ioctl(read_fd, 0);
    READ_FROM_FD(read_fd, &msg);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    WK_CHECK_WITH_EXIT(msg.equal("HELLO FP"), fmt::format("Connect LocalGateway Failed, Receive `{}`", msg.to_string()));
    SPDLOG_DEBUG("Connect LocalGateway Success");
}
void FunctionPool::createFuncDone(const std::shared_ptr<FuncInstProcessCGroup>& processCGroup)
{
    FuncCreateDoneMsg msg;
    msg.magic_number = MAGIC_NUMBER_WUKONG;
    for (int i = 0; i < PipeIndex::pipeCount; i++)
    {
        strcpy(msg.PipeArray[i], processCGroup->pipeArray_prefix[i].c_str());
        msg.PipeSizeArray[i] = processCGroup->pipeArray_prefix[i].size();
    }

    strcpy(msg.funcInst_uuid, processCGroup->funcInst_uuid_prefix.c_str());
    msg.uuidSize = processCGroup->funcInst_uuid_prefix.size();

    strcpy(msg.funcname, processCGroup->funcname.c_str());
    msg.funcname_size = processCGroup->funcname.size();

    msg.instType = processCGroup->instType;

    funcCreateDoneMsgQueue.push(msg);
    poller.rearmFd(write_fd,
                   Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                   Pistache::Polling::Tag(write_fd),
                   Pistache::Polling::Mode::Edge);
}
void FunctionPool::addInstanceCGroup(const std::shared_ptr<FuncInstProcessCGroup>& processCGroup)
{
    wukong::utils::UniqueLock lock(funcInstMapMutex);
    funcInstMap.emplace(processCGroup->funcInst_uuid_prefix, processCGroup);
}
std::shared_ptr<FunctionPoolHandler> FunctionPool::pickOneHandler()
{
    return std::static_pointer_cast<FunctionPoolHandler>(pickHandler());
}
void FunctionPool::onRunning() const
{
    bool running = true;
    WRITE_2_FD_original(write_fd, &running, sizeof(running));
}
void FunctionPool::onFailed() const
{
    bool running = false;
    WRITE_2_FD_original(write_fd, &running, sizeof(running));
}
void FunctionPool::onReady(const Pistache::Polling::Event& event)
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
    }
    if (event.flags.hasFlag(Pistache::Polling::NotifyOn::Write))
    {
        if (fd == write_fd)
        {
            try
            {
                handlerFuncCreateDoneQueue();
            }
            catch (std::exception& ex)
            {
                SPDLOG_ERROR("handlerFuncCreateDoneQueue error: {}", ex.what());
            }
        }
    }
}
void FunctionPool::handlerIncoming()
{
    for (;;)
    {
        FuncCreateMsg msg;
        // TODO 缺少封装
        READ_FROM_FD_goto(read_fd, &msg);
        WK_CHECK_WITH_EXIT(MAGIC_NUMBER_CHECK(msg.magic_number), "Data Wrong, Magic Check Failed!");
        auto handler = pickOneHandler();
        handler->createFunc(msg);
    }
read_fd_EAGAIN:;
}
void FunctionPool::handlerFuncCreateDoneQueue()
{
    for (;;)
    {
        if (funcCreateDoneMsgQueue.empty())
            break;
        auto msg = funcCreateDoneMsgQueue.front();
        WRITE_2_FD_goto(write_fd, msg);
        funcCreateDoneMsgQueue.pop();
        continue;
    write_fd_EAGAIN:
        poller.rearmFd(write_fd,
                       Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Write),
                       Pistache::Polling::Tag(write_fd),
                       Pistache::Polling::Mode::Edge);
        break;
    }
}
