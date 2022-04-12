//
// Created by kingdo on 2022/4/9.
//

#include <wukong/utils/shm/ShareMemoryObject.h>
ShareMemoryObject::ShareMemoryObject(off64_t length, std::string name)
    : created(false)
    , name_(std::move(name))
    , length_(length)

{
    shm_path = "/" + name_;
}
ShareMemoryObject::~ShareMemoryObject()
{
    if (created)
        WK_CHECK_FUNC_RET(remove());
}
WK_FUNC_RETURN_TYPE ShareMemoryObject::create()
{
    WK_FUNC_START()
    WK_FUNC_CHECK(!created, "ShareMemoryObject is created");
    int fd = ::shm_open(shm_path.c_str(),
                        O_CREAT | O_EXCL | O_RDWR,
                        S_IRUSR | S_IWUSR);
    WK_FUNC_CHECK(fd != -1, wukong::utils::errors("shm_open"));
    int ret = ::ftruncate(fd, length_);
    WK_FUNC_CHECK_WITH_ERROR_HANDLE(ret != -1, wukong::utils::errors("ftruncate"), [this]() {
        ::shm_unlink(shm_path.c_str());
    });
    auto address = mmap(nullptr, length_,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    WK_FUNC_CHECK_WITH_ERROR_HANDLE(address != MAP_FAILED, wukong::utils::errors("mmap"), [this]() {
        ::shm_unlink(shm_path.c_str());
    });
    memset(address, 0, length_);
    ret     = close(fd);
    created = true;
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE ShareMemoryObject::remove()
{
    WK_FUNC_START()
    WK_FUNC_CHECK(created, "ShareMemoryObject has been not created");
    int ret = ::shm_unlink(shm_path.c_str());
    WK_FUNC_CHECK(ret != -1, wukong::utils::errors("shm_open"));
    created = false;
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE ShareMemoryObject::open(void** address, bool is_creator) const
{
    WK_FUNC_START()
    WK_FUNC_CHECK(created, "ShareMemoryObject has been not created");
    int fd = ::shm_open(shm_path.c_str(),
                        O_RDWR,
                        0);
    WK_FUNC_CHECK(fd != -1, wukong::utils::errors("shm_open"));
    if (is_creator)
    {
        *address = mmap(nullptr, length_,
                        PROT_WRITE,
                        MAP_SHARED, fd, 0);
    }
    else
    {
        *address = mmap(nullptr, length_,
                        PROT_WRITE,
                        MAP_PRIVATE, fd, 0);
    }

    WK_FUNC_CHECK(*address != MAP_FAILED, wukong::utils::errors("mmap"));
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE ShareMemoryObject::open(const ShareMemoryObject& object, void** address, bool is_creator)
{
    return open(object.uuid(), object.size(), address, is_creator);
}
WK_FUNC_RETURN_TYPE ShareMemoryObject::open(const std::string& uuid, size_t length, void** address, bool is_creator)
{
    WK_FUNC_START()
    auto path     = to_path(uuid);
    auto shm_path = to_shm_path(uuid);
    WK_FUNC_CHECK(boost::filesystem::exists(path), fmt::format("shm {} is not exists", shm_path.string()));
    int fd = ::shm_open(to_shm_path(uuid).c_str(),
                        O_RDWR,
                        0);
    WK_FUNC_CHECK(fd != -1, wukong::utils::errors("shm_open"));
    if (is_creator)
    {
        *address = mmap(nullptr, length,
                        PROT_WRITE,
                        MAP_SHARED, fd, 0);
    }
    else
    {
        *address = mmap(nullptr, length,
                        PROT_WRITE,
                        MAP_PRIVATE, fd, 0);
    }

    WK_FUNC_CHECK(*address != MAP_FAILED, wukong::utils::errors("mmap"));
    WK_FUNC_END()
}
boost::filesystem::path ShareMemoryObject::to_path(const std::string& uuid)
{
    return boost::filesystem::path("/dev/shm").append(uuid);
}
boost::filesystem::path ShareMemoryObject::to_shm_path(const std::string& uuid)
{
    return boost::filesystem::path("/").append(uuid);
}
std::string ShareMemoryObject::uuid() const
{
    return name_;
}
off64_t ShareMemoryObject::size() const
{
    return length_;
}
WK_FUNC_RETURN_TYPE ShareMemoryObject::uuid_check(const std::string& uuid)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(uuid.size() == (strlen(ShareMemoryObjectUUIDPrefix) + ShareMemoryObjectUUIDRandomStringSize),
                  fmt::format("uuid `{}` is illegal, size is unexpected", uuid));
    WK_FUNC_CHECK(uuid.starts_with(ShareMemoryObjectUUIDPrefix),
                  fmt::format("uuid `{}` is illegal, prefix is unexpected", uuid));
    WK_FUNC_END()
}
