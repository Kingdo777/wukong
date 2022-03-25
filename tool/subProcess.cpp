//
// Created by kingdo on 2022/3/21.
//

#include <wukong/utils/process/DefaultSubProcess.h>
#include <wukong/utils/log.h>

int main() {
    wukong::utils::initLog();
    wukong::utils::SubProcess::Options options("pwd");
    options.Workdir("/home/kingdo");
    wukong::utils::DefaultSubProcess process(options);
    if (!process.spawn()) {
        printf("%d\n", process.getPid());
        wait(nullptr);
    }
}