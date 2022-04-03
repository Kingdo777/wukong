//
// Created by kingdo on 2022/4/1.
//

#ifndef WUKONG_C_GROUP_H
#define WUKONG_C_GROUP_H

#include "wukong/utils/log.h"
#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <wukong/utils/errors.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/os.h>

class CGroup
{
public:
    static const char* CGroup_Root_Path;

    explicit CGroup();

    ~CGroup()
    {
        WK_CHECK_FUNC_RET(remove(true));
    }

    WK_FUNC_RETURN_TYPE create();

    WK_FUNC_RETURN_TYPE moveTo();

    WK_FUNC_RETURN_TYPE remove(bool forcibly = false);

    WK_FUNC_RETURN_TYPE getProcs(std::vector<pid_t>& procs);

protected:
    virtual WK_FUNC_RETURN_TYPE path_check(const boost::filesystem::path& path_) = 0;

    void set_path(const boost::filesystem::path& path_);

    WK_FUNC_RETURN_TYPE setValue(const boost::filesystem::path& path_, int64_t value);

    [[nodiscard]] boost::filesystem::path get_path() const;

    enum CGroupStatus {
        Uninitialized,
        Created,
        Deleted
    };
    CGroupStatus status;

    boost::filesystem::path path;

    pid_t parent_process_pid;
};

class MemoryCGroup : public CGroup
{
public:
    static const char* MEMORY_CGroup_Root_Path;
    explicit MemoryCGroup(std::string name_);

    WK_FUNC_RETURN_TYPE setHardLimit(int mem_size);

private:
    WK_FUNC_RETURN_TYPE path_check(const boost::filesystem::path& path_) override;

    std::string name;

    int64_t hard_limit;
};

class CpuCGroup : public CGroup
{
public:
    static const char* CPU_CGroup_Root_Path;
    explicit CpuCGroup(std::string name_);

    WK_FUNC_RETURN_TYPE setShares(int shares_);
    WK_FUNC_RETURN_TYPE setCPUS(int cores);

private:
    WK_FUNC_RETURN_TYPE path_check(const boost::filesystem::path& path_) override;

    std::string name;

    int64_t shares;
    int64_t cfs_period_us;
    int64_t cfs_quota_us;
};

#endif // WUKONG_C_GROUP_H
