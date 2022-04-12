//
// Created by kingdo on 2022/4/9.
//

#include "StorageFuncAgent.h"
StorageFuncAgent::StorageFuncAgent()
    : status(Created)
    , read_fd(wukong::utils::Config::InstanceFunctionDefaultReadFD())
    , write_fd(wukong::utils::Config::InstanceFunctionDefaultWriteFD())
{
    wukong::utils::nonblock_ioctl(write_fd, 0);
    wukong::utils::nonblock_ioctl(read_fd, 1);
}
void StorageFuncAgent::run()
{
    if (status == Running)
    {
        SPDLOG_WARN("StorageFuncAgent is already Running ");
        return;
    }
    if (!shutdownFd.isBound())
        shutdownFd.bind(poller);

    poller.addFd(read_fd,
                 Pistache::Flags<Pistache::Polling::NotifyOn>(Pistache::Polling::NotifyOn::Read),
                 Pistache::Polling::Tag(read_fd),
                 Pistache::Polling::Mode::Edge);

    task = std::thread([=, this] {
        for (;;)
        {
            std::vector<Pistache::Polling::Event> events;
            int ready_fds = poller.poll(events);
            WK_CHECK_WITH_EXIT(ready_fds != -1, "Pistache::Polling");
            for (const auto& event : events)
            {
                if (event.tag == shutdownFd.tag())
                    return;
                onReady(event);
            }
        }
    });
}
void StorageFuncAgent::shutdown()
{
    if (shutdownFd.isBound())
        shutdownFd.notify();
    if (status != Running)
    {
        SPDLOG_WARN("StorageFuncAgent is not Running ");
        return;
    }
    task.join();
    status = Shutdown;
}
void StorageFuncAgent::backResponse(bool success, const std::string& msg, uint64_t request_id) const
{
    FuncResult result;
    result.magic_number = MAGIC_NUMBER_WUKONG;
    memcpy(result.data, msg.data(), msg.size());
    result.data_size  = msg.size();
    result.request_id = request_id;
    result.success    = success;
    wukong::utils::write_2_fd(write_fd, result);
}
void StorageFuncAgent::handleCreate(size_t length, uint64_t request_id)
{
    auto obj_prt = std::make_shared<ShareMemoryObject>(length);
    auto ret     = obj_prt->create();
    WK_CHECK_WITH_ERROR_HANDLE_and_RETURN(ret.first,
                                          fmt::format("can't create shm with length `{}` : {}", length, ret.second),
                                          ([&, this](const std::string& msg) { backResponse(false, msg, request_id); }));
    shareMemoryObjectMap.emplace(obj_prt->uuid(), obj_prt);
    wukong::utils::Json json;
    json.set("uuid", obj_prt->uuid());
    json.setUInt64("length", length);
    backResponse(true, json.serialize(), request_id);
}
void StorageFuncAgent::handleDelete(const std::string& uuid, uint64_t request_id)
{
    WK_CHECK_WITH_ERROR_HANDLE_and_RETURN(shareMemoryObjectMap.contains(uuid),
                                          fmt::format("can't find shm named `{}`", uuid),
                                          ([&, this](const std::string& msg) { backResponse(false, msg, request_id); }));
    auto length = shareMemoryObjectMap.at(uuid)->size();
    auto ret    = shareMemoryObjectMap.at(uuid)->remove();
    WK_CHECK_WITH_ERROR_HANDLE_and_RETURN(ret.first,
                                          fmt::format("can't delete shm named `{}` : {}", uuid, ret.second),
                                          ([&, this](const std::string& msg) { backResponse(false, msg, request_id); }));
    shareMemoryObjectMap.erase(uuid);
    wukong::utils::Json json;
    json.set("uuid", uuid);
    json.setUInt64("length", length);
    backResponse(true, json.serialize(), request_id);
}
void StorageFuncAgent::onReady(const Pistache::Polling::Event& event)
{
    for (;;)
    {
        std::string msg_json;
        static auto max_read_buffer_size = wukong::utils::Config::InstanceFunctionReadBufferSize();
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
        const auto& msg        = wukong::proto::jsonToMessage(msg_json);
        std::string funcname   = msg.function();
        uint64_t request_id    = msg.id();
        StorageFuncOpType type = StorageFuncOpType::Unknown;
        WK_CHECK_WITH_ERROR_HANDLE_and_RETURN(funcname.starts_with(STORAGE_FUNCTION_NAME),
                                              fmt::format("funcname {} is not start with {}", funcname, STORAGE_FUNCTION_NAME),
                                              ([&, this](const std::string& msg) { backResponse(false, msg, request_id); }));
        if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Create]))
        {
            type = StorageFuncOpType::Create;
        }
        else if (funcname == fmt::format("{}/{}", STORAGE_FUNCTION_NAME, StorageFuncOpTypeName[StorageFuncOpType::Delete]))
        {
            type = StorageFuncOpType::Delete;
        }

        switch (type)
        {
        case Create: {
            handleCreate(strtoul(msg.inputdata().c_str(), nullptr, 10), request_id);
            break;
        }
        case Delete: {
            handleDelete(msg.inputdata(), request_id);
            break;
        }
        default:
            WK_CHECK_WITH_ERROR_HANDLE_and_RETURN(false,
                                                  fmt::format("Data Wrong : Unknown OP type : {}", type),
                                                  ([&, this](const std::string& msg) { backResponse(false, msg, request_id); }));
        }
    }
}
