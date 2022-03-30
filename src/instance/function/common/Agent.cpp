//
// Created by kingdo on 2022/3/27.
//

#include "Agent.h"

Agent::Options::Options() : threads_(1),
                            read_fd(wukong::utils::Config::InstanceFunctionDefaultReadFD()),
                            max_read_buffer_size(wukong::utils::Config::InstanceFunctionReadBufferSize()),
                            write_fd(wukong::utils::Config::InstanceFunctionDefaultWriteFD()),
                            type_(C_PP) {}

Agent::Options Agent::Options::options() {
    return {};
}

Agent::Options &Agent::Options::threads(int val) {
    threads_ = val;
    return *this;
}

Agent::Options &Agent::Options::type(Agent::Type val) {
    type_ = val;
    return *this;
}

Agent::Agent() :
        reactor_(Pistache::Aio::Reactor::create()),
        handlerKey_(),
        handlerIndex_(0),
        read_fd(-1),
        max_read_buffer_size(0),
        write_fd(-1),
        type(C_PP),
        lib() {}

void Agent::init(Agent::Options &options) {
    loadFunc(options);
    reactor_->init(Pistache::Aio::AsyncContext(options.threads_));
    type = options.type_;
    poller.addFd(read_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(read_fd),
                 Pistache::Polling::Mode::Edge);
    if (!shutdownFd.isBound())
        shutdownFd.bind(poller);
    if (!writeQueue.isBound())
        writeQueue.bind(poller);
}

void Agent::set_handler(std::shared_ptr<AgentHandler> handler) {
    handler_ = std::move(handler);
    handlerKey_ = reactor_->addHandler(handler_);
}

void Agent::run() {
    if (!handler_) {
        SPDLOG_ERROR("Please Set handler First");
        onFailed();
        return;
    }
    reactor_->run();
    task = std::thread([=, this] {
        for (;;) {
            std::vector<Pistache::Polling::Event> events;
            int ready_fds = poller.poll(events);
            WK_CHECK_WITH_ASSERT(ready_fds != -1, "Pistache::Polling");
            for (const auto &event: events) {
                if (event.tag == shutdownFd.tag())
                    return;
                if (event.flags.hasFlag(Pistache::Polling::NotifyOn::Read)) {
                    auto fd = event.tag.value();
                    if (static_cast<ssize_t>(fd) == read_fd) {
                        try {
                            handlerIncoming();
                        }
                        catch (std::exception &ex) {
                            SPDLOG_ERROR("handlerIncoming error: {}", ex.what());
                        }
                    }
                    if (event.tag == writeQueue.tag()) {
                        try {
                            handlerWriteQueue();
                        }
                        catch (std::exception &ex) {
                            SPDLOG_ERROR("handlerWriteQueue error: {}", ex.what());
                        }
                    }
                }
            }
        }
    });
    onRunning();
}

void Agent::shutdown() {
    if (shutdownFd.isBound())
        shutdownFd.notify();
    task.join();
    reactor_->shutdown();
    if (func_entry)
        lib.close();
}

void Agent::doExec(FaasHandle *h) {
    WK_CHECK_WITH_ASSERT(func_entry != nullptr, "func_entry is null");
    func_entry(h);
}

void Agent::finishExec(wukong::proto::Message msg) {
    msg.set_finishtimestamp(wukong::utils::getMillsTimestamp());
    std::string msg_json = wukong::proto::messageToJson(msg);
    std::string storageKey = msg.resultkey();
    auto &redis = wukong::utils::Redis::getRedis();
    redis.set(storageKey, msg_json);
    writeQueue.push(std::move(msg));
}

void Agent::onRunning() const {
    bool running = true;
    ::write(write_fd, &running, sizeof(running));
}

void Agent::onFailed() const {
    bool running = false;
    ::write(write_fd, &running, sizeof(running));
}

void Agent::loadFunc(Agent::Options &options) {
    read_fd = options.read_fd;
    max_read_buffer_size = options.max_read_buffer_size;
    write_fd = options.write_fd;
    struct FunctionInfo {
        char lib_path[256];
        int threads;
    } func_info{{0}};
    wukong::utils::nonblock_ioctl(read_fd, 0);
    wukong::utils::read_from_fd(read_fd, &func_info);
    wukong::utils::nonblock_ioctl(read_fd, 1);
    auto lib_path = boost::filesystem::path(std::string{func_info.lib_path, 256});
    WK_CHECK_WITH_ASSERT(exists(lib_path), "Please Right Set read_from_fd");
    WK_CHECK_WITH_ASSERT(func_info.threads >= 1, "func Concurrency < 1 ?");
    options.threads(func_info.threads);
    lib.open(lib_path.c_str());
    if (lib.sym("_Z9faas_mainP10FaasHandle", (void **) (&func_entry))) {
        SPDLOG_ERROR(fmt::format("load Code Error:{}", lib.errors()));
        assert(false);
    }
}

std::shared_ptr<AgentHandler> Agent::pickOneHandler() {
    auto transports = reactor_->handlers(handlerKey_);
    auto index = handlerIndex_.fetch_add(1) % transports.size();

    return std::static_pointer_cast<AgentHandler>(transports[index]);
}

void Agent::handlerWriteQueue() {
    for (;;) {
        auto item = writeQueue.popSafe();
        if (!item)
            break;
        struct {
            bool success = false;
            uint64_t msg_id = 0;
            char data[2048] = {0};
        } result;
        result.success = true;
        result.msg_id = item->id();
        memcpy(result.data, item->outputdata().data(), item->outputdata().size());
//        SPDLOG_DEBUG("Write : {} {} {}", result.success, result.msg_id, result.data);
        wukong::utils::write_2_fd(write_fd, result);
    }
}

void Agent::handlerIncoming() {
    std::string msg_json;
    msg_json.resize(max_read_buffer_size, 0);
    // TODO 缺少封装
    auto size = ::read(read_fd, msg_json.data(), max_read_buffer_size);
    if (size <= 0) {
        SPDLOG_ERROR("read fd is wrong");
        return;
    }
    const auto &msg = wukong::proto::jsonToMessage(msg_json);
    auto handler = pickOneHandler();
    handler->putMessage(msg);
}
