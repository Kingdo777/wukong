#include <wukong/endpoint/endpoint.h>
#include <wukong/utils/timing.h>

int main() {
    wukong::utils::initLog();
    wukong::utils::Config::print();
    const auto handler = std::make_shared<wukong::endpoint::WukongHandler>();
    wukong::endpoint::Endpoint e("gateway", handler);
    e.start();
    wukong::utils::Timing::printTimerTotals();
    return 0;
}

