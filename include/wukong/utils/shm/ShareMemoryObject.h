//
// Created by kingdo on 2022/4/9.
//

#ifndef WUKONG_SHARE_MEMORY_OBJECT_H
#define WUKONG_SHARE_MEMORY_OBJECT_H
#include <boost/filesystem.hpp>
#include <fcntl.h> /* For O_* constants */
#include <string>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <utility>
#include <wukong/utils/errors.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/radom.h>

#define ShareMemoryObjectUUIDRandomStringSize (32)
#define ShareMemoryObjectUUIDPrefix "wukong-"

class ShareMemoryObject
{
public:
    explicit ShareMemoryObject(off64_t length, std::string name = ShareMemoryObjectUUIDPrefix + wukong::utils::randomString(ShareMemoryObjectUUIDRandomStringSize));

    ~ShareMemoryObject();

    WK_FUNC_RETURN_TYPE create();

    WK_FUNC_RETURN_TYPE remove();

    WK_FUNC_RETURN_TYPE open(void** address, bool is_creator) const;

    static WK_FUNC_RETURN_TYPE open(const ShareMemoryObject& object, void** address, bool is_creator);

    static WK_FUNC_RETURN_TYPE open(const std::string& uuid, size_t length, void** address, bool is_creator);

    static boost::filesystem::path to_path(const std::string& uuid);

    static boost::filesystem::path to_shm_path(const std::string& uuid);

    static WK_FUNC_RETURN_TYPE uuid_check(const std::string& uuid);

    static size_t uuid_size()
    {
        return strlen(ShareMemoryObjectUUIDPrefix) + ShareMemoryObjectUUIDRandomStringSize;
    }

    [[nodiscard]] std::string uuid() const;

    [[nodiscard]] off64_t size() const;

private:
    bool created;

    const std::string name_;
    std::string shm_path;

    const off64_t length_;
};

#endif // WUKONG_SHARE_MEMORY_OBJECT_H
