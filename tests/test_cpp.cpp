//
// Created by kingdo on 2022/3/16.
//

#include <boost/filesystem.hpp>
#include <list>
#include <wukong/utils/macro.h>

using namespace std;

boost::filesystem::path getFunCodePath(const std::string& funcname, bool is_python)
{
    if (is_python)
        return boost::filesystem::path("/tmp/wukong/func-code/python").append(funcname + ".py");
    return boost::filesystem::path("/tmp/wukong/func-code/cpp").append(funcname).append("lib.so");
}

std::string getFuncnameFromCodePath(const boost::filesystem::path& path)
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

int main()
{
    SPDLOG_INFO(getFuncnameFromCodePath(getFunCodePath("cpp", false)));
    SPDLOG_INFO(getFuncnameFromCodePath(getFunCodePath("python", true)));

    SPDLOG_INFO(getFuncnameFromCodePath(getFunCodePath("cppfhwuifhiwh", false)));
    SPDLOG_INFO(getFuncnameFromCodePath(getFunCodePath("pythonwifhiowehfioweh", true)));

    //    boost::filesystem::path p("/home/kingdo/CLionProjects/wukong/src/instance/function/worker-function/CMakeLists.txt");
    //    SPDLOG_INFO(p.parent_path().string());
    //    SPDLOG_INFO(p.filename().string());

    //    char s_[] = "123456";
    //    string s { s_, size(s_) };
    //    SPDLOG_INFO("{:p}", s.data());
    //    string s1 = "654321";
    //    s         = s1;
    //    SPDLOG_INFO("{:p}", s.data());

    //    wchar_t s[] = L"中国";
    //    printf("%ls\n",s);

    //    char s[] = {
    //        'q', 'w', 'e', 0, 1, 'q', 'w', 'e'
    //    };
    //
    //    string str { s, size(s) };
    //    SPDLOG_INFO("{} {} {}", s, str.size(),str.length());

    //    string uuid = "wukong-3fC15xRgrwk3IcrxCCfZT66CX9cH97yB";
    //    int a       = uuid.size();
    //    int b       = strlen("wukong-");
    //    if ((a == 32 + b) && uuid.starts_with("wukong-"))
    //        printf("OK");

    //    std::list<string> list;
    //    for (int i = 0; i < 100; ++i)
    //    {
    //        list.emplace_back("s" + to_string(i));
    //    }
    //
    //    auto i33 = std::find(list.begin(), list.end(), "s33");
    //    auto i77 = std::find(list.begin(), list.end(), "s77");
    //    SPDLOG_INFO("{} {}", *i33, *i77);
    //
    //    list.erase(std::find(list.begin(), list.end(), "s55"));
    //    SPDLOG_INFO("{} {}", *i33, *i77);

    //    vector<std::array<int, 2>> pipes;
    //    pipes.push_back({ 1, 2 });
}