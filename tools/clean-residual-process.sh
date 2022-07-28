ps -aux | grep 'instance-function-pool' | awk '{{print $2}}' | xargs kill -9
ps -aux | grep 'test-localgateway' | awk '{{print $2}}' | xargs kill -9
ps -aux | grep '/home/kingdo/CLionProjects/wukong/cmake-build-debug/out/bin' | awk '{{print $2}}' | xargs kill -9
