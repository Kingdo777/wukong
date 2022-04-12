//
// Created by kingdo on 2022/4/9.
//

#ifndef WUKONG_STORAGE_FUNC_AGENT_H
#define WUKONG_STORAGE_FUNC_AGENT_H

#include <pistache/mailbox.h>
#include <pistache/reactor.h>
#include <wukong/utils/config.h>
#include <wukong/utils/os.h>
#include <wukong/utils/struct.h>
#include <wukong/utils/reactor/Reactor.h>
#include <wukong/utils/shm/ShareMemoryObject.h>
#include <wukong/proto/proto.h>

class StorageFuncAgent
{
public:
    StorageFuncAgent();

    void run();

    void shutdown();

private:
    void backResponse(bool success, const std::string& msg, uint64_t request_id) const;
    void handleCreate(size_t length, uint64_t request_id);

    void handleDelete(const std::string& name, uint64_t request_id);
    void onReady(const Pistache::Polling::Event& event);

    enum Status {
        Created,
        Running,
        Shutdown
    };
    Status status;

    int read_fd  = -1;
    int write_fd = -1;

    std::thread task;

    Pistache::Polling::Epoll poller;
    Pistache::NotifyFd shutdownFd;

    std::unordered_map<std::string, std::shared_ptr<ShareMemoryObject>> shareMemoryObjectMap;
};

#endif // WUKONG_STORAGE_FUNC_AGENT_H
