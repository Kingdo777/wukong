//
// Created by kingdo on 2022/3/17.
//

#include "InstanceProxy.h"

bool InstanceProxy::checkStatus(std::initializer_list<Status> desiredStates) {
    if (std::find(desiredStates.begin(), desiredStates.end(), status) == desiredStates.end()) {
        std::string desiredStatesList;
        for (auto s: desiredStates) {
            desiredStatesList += (StatusName[s] + ", ");
        }
        SPDLOG_ERROR("current status is {}, But [  {} ] is Needed", StatusName[status], desiredStatesList);
        return false;
    }
    return true;
}
