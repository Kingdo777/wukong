//
// Created by kingdo on 2022/3/16.
//

#include <list>
#include <wukong/utils/macro.h>
#include <boost/filesystem.hpp>

using namespace std;

int main()
{

    boost::filesystem::path p("/home/kingdo/CLionProjects/wukong/src/instance/function/worker-function/CMakeLists.txt");
    SPDLOG_INFO(p.parent_path().string());
    SPDLOG_INFO(p.filename().string());

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