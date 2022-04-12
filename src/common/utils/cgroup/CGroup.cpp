//
// Created by kingdo on 2022/4/1.
//

#include <wukong/utils/cgroup/CGroup.h>

const char* CGroup::CGroup_Root_Path        = "/sys/fs/cgroup";
const char* CpuCGroup::CPU_CGroup_Root_Path = "/sys/fs/cgroup/cpu/wukong";
CpuCGroup::CpuCGroup(std::string name_)
    : name(std::move(name_))
    , shares(1024)
    , cfs_period_us(100000)
    , cfs_quota_us(-1)
{
    CGroup::set_path(boost::filesystem::path(CPU_CGroup_Root_Path).append(name));
}
WK_FUNC_RETURN_TYPE CpuCGroup::path_check(const boost::filesystem::path& path_)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(!path_.empty(), "path is empty");
    WK_FUNC_CHECK(exists(path_.parent_path()), fmt::format("{} is not exists", path_.parent_path().string()));
    WK_FUNC_CHECK(path_.string().starts_with(CPU_CGroup_Root_Path), fmt::format("`{}` is not start with `{}`", path_.string(), CPU_CGroup_Root_Path));
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE CpuCGroup::setCPUS(int cores)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    WK_FUNC_CHECK(cores > 0, "cores must is Positive");
    int add = !!(cores % WUKONG_CPU_UNIT_SIZE);
    WK_CHECK_WITH_EXIT(add == 0 || add == 1, "Wrong value of add");
    cores                              = (cores / WUKONG_CPU_UNIT_SIZE + add) * WUKONG_CPU_UNIT_SIZE;
    cfs_quota_us                       = cfs_period_us * cores / 1000;
    boost::filesystem::path quota_path = get_path().append("cpu.cfs_quota_us");
    return setValue(quota_path, cfs_quota_us);
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE CpuCGroup::setShares(int shares_)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    WK_FUNC_CHECK(shares_ > 0, "shares must is Positive");
    shares                              = shares_;
    boost::filesystem::path shares_path = get_path().append("cpu.shares");
    return setValue(shares_path, shares);
    WK_FUNC_END()
}
const char* MemoryCGroup::MEMORY_CGroup_Root_Path = "/sys/fs/cgroup/memory/wukong";
MemoryCGroup::MemoryCGroup(std::string name_)
    : name(std::move(name_))
    , hard_limit((INT64_MAX / WUKONG_PAGE_SIZE) * WUKONG_PAGE_SIZE)
{
    CGroup::set_path(boost::filesystem::path(MEMORY_CGroup_Root_Path).append(name));
}
WK_FUNC_RETURN_TYPE MemoryCGroup::setHardLimit(int mem_size)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    WK_FUNC_CHECK(mem_size > 0, "mem_size must is Positive");
    int add = !!(mem_size % WUKONG_MEMORY_UNIT_SIZE);
    WK_CHECK_WITH_EXIT(add == 0 || add == 1, "Wrong value of add");
    hard_limit                              = (mem_size / WUKONG_MEMORY_UNIT_SIZE + add) * WUKONG_MEMORY_UNIT_SIZE * 1024 * 1024;
    boost::filesystem::path hard_limit_path = get_path().append("memory.limit_in_bytes");
    return setValue(hard_limit_path, hard_limit);
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE MemoryCGroup::path_check(const boost::filesystem::path& path_)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(!path_.empty(), "path is empty");
    WK_FUNC_CHECK(exists(path_.parent_path()), fmt::format("{} is not exists", path_.parent_path().string()));
    WK_FUNC_CHECK(path_.string().starts_with(MEMORY_CGroup_Root_Path), fmt::format("`{}` is not start with `{}`", path_.string(), MEMORY_CGroup_Root_Path));
    WK_FUNC_END()
}

CGroup::CGroup()
    : status(Uninitialized)
    , path()
    , parent_process_pid()
{ }
WK_FUNC_RETURN_TYPE CGroup::create()
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status != Created, "CGroup has been Created");
    WK_FUNC_CHECK(!exists(get_path()), fmt::format("Dir `{}` is exists", get_path().string()));
    int res = ::mkdir(get_path().c_str(), WUKONG_DIR_CREAT_MODE);
    WK_FUNC_CHECK(!res, fmt::format("mkdir get errors : {}", wukong::utils::errors()));
    status             = Created;
    parent_process_pid = getpid();
    SPDLOG_DEBUG("Create CGroup {} ", get_path().string());
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE CGroup::moveTo()
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    auto tasks_path  = get_path().append("tasks");
    auto current_pid = getpid();
    WK_FUNC_CHECK(parent_process_pid != current_pid, "Can't add Creator Process");
    std::ofstream outfile;
    outfile.open(tasks_path.string(), std::ios_base::app);
    WK_FUNC_CHECK(outfile.is_open(), fmt::format("open `{}` is Failed", get_path().string()));
    outfile << current_pid << std::endl;
    outfile.flush();
    outfile.close();
    SPDLOG_DEBUG("Added PID {} to {}", current_pid, tasks_path.string());
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE CGroup::remove(bool forcibly)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    auto current_pid = getpid();
    WK_FUNC_CHECK(parent_process_pid == getpid(), "Only Creator Can remove CGroup");
    std::vector<pid_t> procs;
    WK_FUNC_CHECK_RET(getProcs(procs));
    WK_FUNC_CHECK(std::find(procs.begin(), procs.end(), current_pid) == procs.end(), "GGroup Contains Self Process");
    WK_FUNC_CHECK(procs.empty() || forcibly, fmt::format("`{}` is not empty", get_path().append("cgroup.procs").string()));
    for (pid_t pid : procs)
    {
        auto ret = wukong::utils::kill(pid, SIGTERM);
        WK_CHECK_WITH_EXIT(!ret, fmt::format("Send {} to {} get an errors : {}", SIGTERM, pid, wukong::utils::errors()));
        WK_CHECK(pid == ::waitpid(pid, nullptr, 0), "SubProcess Kill Wrong!");
    }
    int res = ::remove(get_path().c_str());
    WK_FUNC_CHECK(res == 0, fmt::format("remove get errors : {}", wukong::utils::errors()));
    status = Deleted;
    SPDLOG_DEBUG("Remove CGroup {} ", get_path().string());
    WK_FUNC_END()
}

WK_FUNC_RETURN_TYPE CGroup::setValue(const boost::filesystem::path& path_, int64_t value)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    std::ofstream outfile(path_);
    WK_FUNC_CHECK(exists(path_), fmt::format("file `{}` is not exists", path_.string()));
    WK_FUNC_CHECK(outfile.is_open(), fmt::format("open `{}` is Failed", path_.string()));
    outfile << value;
    outfile.flush();
    outfile.close();
    WK_FUNC_END()
}
WK_FUNC_RETURN_TYPE CGroup::getProcs(std::vector<pid_t>& procs)
{
    WK_FUNC_START()
    WK_FUNC_CHECK(status == Created, "CGroup is not Create or has been Deleted");
    auto procs_path = get_path().append("cgroup.procs");
    std::ifstream infile(procs_path);
    WK_FUNC_CHECK(infile.is_open(), fmt::format("open `{}` is Failed", procs_path.string()));
    std::string pid_s;
    while (std::getline(infile, pid_s))
    {
        auto pid = std::strtol(pid_s.c_str(), nullptr, 10);
        WK_FUNC_CHECK(pid > 1, fmt::format("contains PID which is <=1", pid_s));
        procs.emplace_back(pid);
    }
    WK_FUNC_END()
}
void CGroup::set_path(const boost::filesystem::path& path_)
{
    WK_CHECK_FUNC_RET_WITH_EXIT(path_check(path_));
    path = path_;
}
boost::filesystem::path CGroup::get_path() const
{
    return path;
}
