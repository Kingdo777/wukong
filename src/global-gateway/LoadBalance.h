//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_LOADBALANCE_H
#define WUKONG_LOADBALANCE_H


class LoadBalance {
public:
    LoadBalance() = default;

    void dispatch();

    void start();

    void stop();
};


#endif //WUKONG_LOADBALANCE_H
