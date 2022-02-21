include(FindGit)
find_package(Git REQUIRED)
include(FetchContent)

#------------- 使用FetchContent构建各个第三方库 ------------------------
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY  https://github.com/gabime/spdlog.git
        GIT_TAG         v1.9.2
)

FetchContent_Declare(
        pistache
        GIT_REPOSITORY  https://github.com/Kingdo777/pistache
)

FetchContent_MakeAvailable(spdlog pistache)


#------------- 将所有的第三方包, 全部打包成一个依赖库 ------------------------
# 创建一个INTERFACE库，一个INTERFACE库不会直接创建编译目标文件, 与下面的INTERFACE并不是同一概念
add_library(common_dependencies INTERFACE)
# INTERFACE 表示, wukong::common_dependencies本身不会链接后面一系列的库及其头文件,
# 而链接wukong::common_dependencies的库或者可执行程序会链接这些库及其头文件,
# INTERFACE与PRIVATE的效果刚好相反. 而PUBLIC则是两者都会链接库及其头文件.
# 详见https://blog.csdn.net/weixin_43862847/article/details/119762230
target_link_libraries(common_dependencies INTERFACE
        spdlog
        pistache_shared
)
# 起一个别名, 因为当前CMAKE不支持直接生成wukong::格式的lib
add_library(wukong::common_dependencies ALIAS common_dependencies)