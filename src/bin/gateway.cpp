#include <wukong/endpoint/endpoint.h>


int main() {
    wukong::util::initLog();
    conf.print();
    wukong::endpoint::Endpoint e(8080, 4, "gateway");
    e.start();
    return 0;
}

