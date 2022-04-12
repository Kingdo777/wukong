//
// Created by kingdo on 2022/3/16.
//

#include <list>
#include <wukong/utils/macro.h>

using namespace std;

int main()
{
    char s[] = {
        'q', 'w', 'e', 0, 1, 'q', 'w', 'e'
    };

    string str { s, size(s) };
    SPDLOG_INFO("{} {} {}", s, str.size(),str.length());

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