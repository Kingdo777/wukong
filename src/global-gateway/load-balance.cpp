//
// Created by kingdo on 2022/3/4.
//

#include <wukong/utils/log.h>
#include <wukong/utils/radom.h>
#include <wukong/utils/redis.h>
#include <wukong/utils/timing.h>
#include <wukong/utils/string-tool.h>

#include <utility>
#include "load-balance.h"


void LoadBalanceClientHandler::registerPoller(Pistache::Polling::Epoll &poller) {
    functionCallQueue.bind(poller);
    Base::registerPoller(poller);
}

void LoadBalanceClientHandler::onReady(const Pistache::Aio::FdSet &fds) {
    for (auto fd: fds) {
        if (fd.getTag() == functionCallQueue.tag()) {
            handleFunctionCallQueue();
        }
    }
    Base::onReady(fds);
}

void LoadBalanceClientHandler::handleFunctionCallQueue() {
    for (;;) {
        auto function = functionCallQueue.popSafe();
        if (!function)
            break;
        asyncCallFunction(std::move(*function));
    }
}

void LoadBalanceClientHandler::asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry &&entry) {
    auto res = post("127.0.0.1:9080", "");
    std::string funcEntryIndex = wukong::utils::randomString(15);
    functionCallMap.insert(std::make_pair(funcEntryIndex, std::move(entry)));
    res.then(
            [=, this](Pistache::Http::Response response) {
                auto code = response.code();
                std::string result = response.body();
                responseFunctionCall(code, result, funcEntryIndex);
            },
            [=, this](std::exception_ptr exc) {
                try {
                    std::rethrow_exception(std::move(exc));
                }
                catch (const std::exception &e) {
                    auto code = Pistache::Http::Code::Internal_Server_Error;
                    std::string result = e.what();
                    responseFunctionCall(code, result, funcEntryIndex);
                }
            });
}

void LoadBalanceClientHandler::callFunction(Pistache::Http::ResponseWriter response) {
    FunctionCallEntry functionCallEntry(std::move(response));
    functionCallQueue.push(std::move(functionCallEntry));
}

void LoadBalanceClientHandler::responseFunctionCall(Pistache::Http::Code code, std::string &result,
                                                    const std::string &funcEntryIndex) {
    auto iter = functionCallMap.find(funcEntryIndex);
    if (iter == functionCallMap.end()) {
        SPDLOG_ERROR(fmt::format("functionCall not Find in functionCallMap, with funcEntryIndex `{}`",
                                 funcEntryIndex));
        return;
    }
    iter->second.response.send(code, result);
    functionCallMap.erase(iter);
}

void LoadBalance::start() {
    if (status != Uninitialized) {
        SPDLOG_ERROR("LB is not Uninitialized");
        return;
    }
    /// start LB Client
    auto opts = Pistache::Http::Client::options().
            threads(wukong::utils::Config::ClientNumThreads()).
            maxConnectionsPerHost(wukong::utils::Config::ClientMaxConnectionsPerHost());
    cs.setHandler(std::make_shared<LoadBalanceClientHandler>(&cs));
    SPDLOG_INFO("Starting LoadBalance with {} threads", wukong::utils::Config::ClientNumThreads());
    cs.start(opts);
    /// 加载 Invoker、user、Application、Function信息
    load();
    status = Running;
}

void LoadBalance::stop() {
    if (status != Running) {
        SPDLOG_ERROR("LB is not Running");
        return;
    }
    SPDLOG_INFO("Shutting down LoadBalance");
    cs.shutdown();
    status = Stopped;
}

void LoadBalance::dispatch(wukong::proto::Message &&msg, Pistache::Http::ResponseWriter response) {
    pickOneHandler()->callFunction(std::move(response));
}

std::shared_ptr<LoadBalanceClientHandler> LoadBalance::pickOneHandler() {
    return std::static_pointer_cast<LoadBalanceClientHandler>(cs.pickOneHandler());
}

/// 由于注册Invoker并不是一个经常发生的过程，因此交由endpoint的线程完成，就不需要给LB的Client线程去实现了。
void LoadBalance::handleInvokerRegister(const std::string &host,
                                        const std::string &invokerJson,
                                        Pistache::Http::ResponseWriter response) {
    wukong::proto::Invoker invoker = wukong::proto::jsonToInvoker(invokerJson);
    invoker.set_registertime(wukong::utils::getMillsTimestamp());
    invoker.set_ip(host);

    auto check_result = invokers.invokerCheck(invoker);

    if (check_result.first) {
        invokers.registerInvoker(invoker);
        response.send(Pistache::Http::Code::Ok, wukong::proto::messageToJson(invoker));
        SPDLOG_DEBUG("Invoker Register Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("Invoker Register Done. Failed: {}", check_result.second);
    }
}

void LoadBalance::handleFuncRegister(wukong::proto::Function &function,
                                     const std::string &code,
                                     Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto check_result = functions.registerFuncCheck(function, code);
    if (check_result.first) {
        functions.registerFunction(function, code);
        response.send(Pistache::Http::Code::Ok, check_result.second);
        SPDLOG_DEBUG("Function Register Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("Function Register Done. Failed: {}", check_result.second);
    }
}

void LoadBalance::handleFuncDelete(const std::string &username,
                                   const std::string &appname,
                                   const std::string &funcname,
                                   Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto check_result = functions.deleteFuncCheck(username, appname, funcname);
    if (check_result.first) {
        /// 这里不能用引用，因为我们要删除function，如果用了引用，那么function被删除之后，就会内存泄露
        wukong::proto::Function function = functions.functions[function_index(username, appname, funcname)];
        functions.deleteFunction(function);
        response.send(Pistache::Http::Code::Ok, check_result.second);
        SPDLOG_DEBUG("Function Delete Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("Function Delete Done. Failed: {}", check_result.second);
    }
}

void LoadBalance::handleUserRegister(const wukong::proto::User &user, Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto check_result = users.registerUserCheck(user);
    if (check_result.first) {
        users.registerUser(user);
        response.send(Pistache::Http::Code::Ok, check_result.second);
        SPDLOG_DEBUG("User Register Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("User Register Done. Failed: {}", check_result.second);
    }
}

void LoadBalance::handleUserDelete(const wukong::proto::User &user, Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto check_result = users.deleteUserCheck(user);
    if (check_result.first) {
        users.deleteUser(user);
        response.send(Pistache::Http::Code::Ok, check_result.second);
        SPDLOG_DEBUG("User Delete Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("User Delete Done. Failed: {}", check_result.second);
    }
}

void
LoadBalance::handleAppCreate(const wukong::proto::Application &application, Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto check_result = applications.checkCreateApplication(application);
    if (check_result.first) {
        applications.createApplication(application);
        response.send(Pistache::Http::Code::Ok, check_result.second);
        SPDLOG_DEBUG("User Register Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("User Register Done. Failed: {}", check_result.second);
    }
}

void
LoadBalance::handleAppDelete(const wukong::proto::Application &application, Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto check_result = applications.checkDeleteApplication(application);
    if (check_result.first) {
        applications.deleteApplication(application);
        response.send(Pistache::Http::Code::Ok, check_result.second);
        SPDLOG_DEBUG("User Delete Done. Success");
    } else {
        response.send(Pistache::Http::Code::Bad_Request, check_result.second);
        SPDLOG_DEBUG("User Delete Done. Failed: {}", check_result.second);
    }
}

void LoadBalance::load() {
    wukong::utils::UniqueLock lock(uaf_mutex);
//    wukong::utils::UniqueLock user(users.mutex);
//    wukong::utils::UniqueLock app(applications.mutex);
//    wukong::utils::UniqueLock func(functions.mutex);
    invokers.loadInvokers(cs);
    users.loadUser();
    applications.loadApplications(users.userSet);
    functions.loadFunctions(applications.applicationSet);
}

void LoadBalance::handleAppInfo(const std::string &username, const std::string &appname,
                                Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    if (!username.empty() && !users.userSet.contains(username)) {
        response.send(Pistache::Http::Code::Bad_Request, fmt::format("{} is not exists", username));
        return;
    }
    auto appInfo = applications.getApplicationInfo(username);
    response.send(Pistache::Http::Code::Ok, appInfo);
}

void LoadBalance::handleUserInfo(const std::string &username, Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    auto usersInfo = users.getUserInfo();
    response.send(Pistache::Http::Code::Ok, usersInfo);
}

void LoadBalance::handleFuncInfo(const std::string &username,
                                 const std::string &appname,
                                 const std::string &funcname,
                                 Pistache::Http::ResponseWriter response) {
    wukong::utils::UniqueLock lock(uaf_mutex);
    response.send(Pistache::Http::Code::Ok, functions.getFunctionInfo(username, appname, funcname));
}

///###########################Invoker###################################
std::pair<bool, std::string> LoadBalance::Invoker::invokerCheck(const wukong::proto::Invoker &invoker) {

    if (invoker.invokerid().empty()) {
        return std::make_pair(false, "invokerID is empty !");
    } else if (invoker.port() <= 0) {
        return std::make_pair(false, fmt::format("port {} is illegal", invoker.port()));
    } else if (invoker.cpu() < 1000) {
        /// CPU 不能少于1个core
        return std::make_pair(false, fmt::format("cpu {} is to small， at least 1 core", invoker.cpu()));
    } else if (invoker.memory() < 2 * 1024) {
        /// CPU 不能少于1个core
        return std::make_pair(false, fmt::format("memory {} is to small， at least 2GB", invoker.memory()));
    } else if (invokerSet.contains(invoker.invokerid())) {
        return std::make_pair(false, fmt::format("InvokerID {} already exists！", invoker.invokerid()));
    }
    for (const auto &existingInvoker: invokers) {
        if (invoker.ip() == existingInvoker.second.ip() && invoker.port() == existingInvoker.second.port())
            return std::make_pair(false, fmt::format("{} has the same ip:port with existing invoker {}！",
                                                     invoker.invokerid(),
                                                     existingInvoker.second.invokerid()));
    }
    return std::make_pair(true, "OK");
}

std::string LoadBalance::Invoker::getInvokersInfo() const {
//    rapidjson::Document invokersInfo;
//    invokersInfo.SetObject();
//    rapidjson::Document::AllocatorType &a = invokersInfo.GetAllocator();
//    for (const auto &invokerID: invokerSet) {
//        wukong::proto::Invoker invoker = invokers[invokerID];
//        auto invokerJsonString = messageToJson(invoker);
//        SPDLOG_DEBUG(invokerJsonString);
//        rapidjson::MemoryStream ms(invokerJsonString.c_str(), invokerJsonString.size());
//        rapidjson::Document d;
//        d.ParseStream(ms);
//        rapidjson::Value invokerIDValue;
//        invokerIDValue.SetString(invokerID.c_str(), a);
//        invokersInfo.AddMember(invokerIDValue, d.GetObject(), a);
//    }
//    rapidjson::StringBuffer sb;
//    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
//    invokersInfo.Accept(writer);
//    return sb.GetString();

    std::string invokersInfoJson = "[";

    for (const auto &invokerID: invokerSet) {
        wukong::proto::Invoker invoker = invokers.at(invokerID);
        auto invokerJsonString = messageToJson(invoker);
        SPDLOG_DEBUG(invokerJsonString);
        invokersInfoJson += fmt::format("{},", invokerJsonString);
    }
    if (invokersInfoJson.ends_with(','))
        invokersInfoJson = invokersInfoJson.substr(0, invokersInfoJson.length() - 1);
    invokersInfoJson += "]";
    return invokersInfoJson;
}

void LoadBalance::Invoker::loadInvokers(wukong::client::ClientServer &client) {
    if (loaded)
        return;
    loaded = true;
    auto &redis = wukong::utils::Redis::getRedis();
    invokerSet = redis.smembers(SET_INVOKER_ID_REDIS_KEY);
    std::vector<std::string> illegalInvokerID;
    for (const auto &invokerID: invokerSet) {
        auto invokerHash = redis.hgetall(invokerID);
        bool pong = false;
        int tryPing = 3;
        if (!invokerHash.empty()) {
            for (int try_time = 0; try_time < tryPing; ++try_time) {
                if (try_time)
                    SPDLOG_WARN("Try ping {} {} times", invokerID, try_time);
                auto rsp = client.
                        get(invokerHash["IP"] + ":" + invokerHash["port"] + "/ping").
                        timeout(std::chrono::seconds(5)).
                        send();
                while (rsp.isPending());
                rsp.then(
                        [&](Pistache::Http::Response response) {
                            if (response.code() == Pistache::Http::Code::Ok && response.body() == "PONG") {
                                pong = true;
                                invokers.insert(
                                        std::make_pair(invokerID, wukong::proto::hashToInvoker(invokerHash)));
                            }
                        },
                        [&](const std::exception_ptr &exc) {

                        });
                if (pong)
                    break;
            }
        }
        if (!pong) {
            /// 删除redis中的hash
            redis.del(invokerID);
            /// 删除redis中invokerSet中指定的invokerID
            redis.srem(SET_INVOKER_ID_REDIS_KEY, invokerID);
            illegalInvokerID.push_back(invokerID);
        }
    }
    /// 删除LB缓存的invokerSet中的invokerID
    for (const auto &invokerID: illegalInvokerID) {
        invokerSet.erase(invokerID);
    }

    std::string invokerInfo = "[";
    for (const auto &invoker: invokerSet) {
        invokerInfo += (invoker + ",");
    }
    if (invokerInfo.ends_with(','))
        invokerInfo = invokerInfo.substr(0, invokerInfo.length() - 1);
    invokerInfo += "]";
    SPDLOG_DEBUG("Available invokers detected: {}", invokerInfo);
}

void LoadBalance::Invoker::registerInvoker(const wukong::proto::Invoker &invoker) {
    auto &redis = wukong::utils::Redis::getRedis();
    invokerSet.insert(invoker.invokerid());
    invokers.insert(std::make_pair(invoker.invokerid(), invoker));
    redis.sadd(SET_INVOKER_ID_REDIS_KEY, invoker.invokerid());
    redis.hmset(invoker.invokerid(), wukong::proto::messageToHash(invoker));
}

///###########################User###################################
void LoadBalance::User::loadUser() {
    auto &redis = wukong::utils::Redis::getRedis();
    if (loaded)
        return;
    userSet = redis.smembers(SET_USERS_REDIS_KEY);
    for (const auto &username: userSet) {
        auto userHash = redis.hgetall(USER_REDIS_KEY(username));
        users.emplace(username, wukong::proto::hashToUser(userHash));
        application.addUser(username);
    }
    SPDLOG_DEBUG(fmt::format("load Users : {}", getUserInfo()));
    loaded = true;
}

std::pair<bool, std::string> LoadBalance::User::registerUserCheck(const wukong::proto::User &user) const {
    bool success = false;
    std::string msg = "Ok";
    auto userName = user.username();
    if (userName.empty())
        msg = "username is empty";
    else if (userSet.contains(userName))
        msg = fmt::format("{} is exists", userName);
    else if (userName.find('#') != std::string::npos)
        msg = fmt::format("{} is has illegal symbol \"#\"", userName);
    else
        success = true;
    return std::make_pair(success, msg);
}

std::pair<bool, std::string> LoadBalance::User::deleteUserCheck(const wukong::proto::User &user) const {
    bool success = false;
    std::string msg = "Ok";
    auto userName = user.username();
    if (!userSet.contains(userName))
        msg = fmt::format("{} is not exists", userName);
    else if (!application.empty(userName))
        msg = "Please remove all application before delete user";
    else
        success = true;
    return std::make_pair(success, msg);
}

void LoadBalance::User::deleteUser(const wukong::proto::User &user) {
    const auto &username = user.username();
    auto &redis = wukong::utils::Redis::getRedis();
    application.deleteUser(username);
    userSet.erase(username);
    users.erase(username);
    redis.srem(SET_USERS_REDIS_KEY, username);
    redis.del(USER_REDIS_KEY(username));
}

std::string LoadBalance::User::getUserInfo() const {
    std::string userInfo = "[";
    for (const auto &userName: userSet) {
        const wukong::proto::User &user = users.at(userName);
        auto invokerJsonString = messageToJson(user);
        userInfo += fmt::format("{},", invokerJsonString);
    }
    if (userInfo.ends_with(','))
        userInfo = userInfo.substr(0, userInfo.length() - 1);
    userInfo += "]";
    return userInfo;
}

void LoadBalance::User::registerUser(const wukong::proto::User &user) {
    auto userName = user.username();
    userSet.insert(userName);
    users.emplace(userName, user);
    auto &redis = wukong::utils::Redis::getRedis();
    redis.sadd(SET_USERS_REDIS_KEY, userName);
    redis.hmset(USER_REDIS_KEY(userName), wukong::proto::messageToHash(user));
    application.addUser(userName);
}

///###########################Application###################################
void LoadBalance::Application::loadApplications(const std::set<std::string> &usersSet) {
    auto &redis = wukong::utils::Redis::getRedis();
    if (loaded)
        return;
    for (const auto &username: usersSet) {
        auto appSetRedis = redis.smembers(SET_APPLICATION_REDIS_KEY(username));
        auto appSet = std::set<std::string>();
        for (const auto &appname: appSetRedis) {
            auto user_app = fmt::format("{}#{}", username, appname);
            auto appHast = redis.hgetall(APPLICATION_REDIS_KEY(username, appname));
            applications.emplace(user_app, wukong::proto::hashToApplication(appHast));
            appSet.insert(user_app);
            function.addApplication(username, appname);
        }
        applicationSet[username] = appSet;
    }
    SPDLOG_DEBUG(fmt::format("load Applications : {}", getApplicationInfo()));
    loaded = true;
}

std::pair<bool, std::string>
LoadBalance::Application::checkCreateApplication(const wukong::proto::Application &application) const {
    bool success = false;
    std::string msg = "Ok";
    const auto &userName = application.user();
    const auto &appName = application.appname();
    if (userName.empty() || appName.empty())
        msg = "username or appname is empty";
    else if (!applicationSet.contains(userName))
        msg = fmt::format("{} is not exists", userName);
    else if (applicationSet.contains(userName) &&
             applicationSet.at(userName).contains(fmt::format("{}#{}", userName, appName)))
        msg = fmt::format("{}-{} is exists", userName, appName);
    else if (userName.find('#') != std::string::npos || appName.find('#') != std::string::npos)
        msg = fmt::format("{} or {} is has illegal symbol \"#\"", userName, appName);
    else
        success = true;
    return std::make_pair(success, msg);
}

std::pair<bool, std::string>
LoadBalance::Application::checkDeleteApplication(const wukong::proto::Application &application) const {
    bool success = false;
    std::string msg = "Ok";

    const auto &username = application.user();
    const auto &appname = application.appname();
    if (username.empty() || appname.empty())
        msg = "username or appname is empty";
    else if (!applicationSet.contains(username) ||
             !applicationSet.at(username).contains(fmt::format("{}#{}", username, appname)))
        msg = fmt::format("{}:{} is not exists", username, appname);
    else if (!function.empty(username, appname))
        msg = "Please remove all function before delete application";
    else
        success = true;
    return std::make_pair(success, msg);
}

void LoadBalance::Application::deleteApplication(const wukong::proto::Application &application) {
    const auto &username = application.user();
    const auto &appname = application.appname();
    auto appIndex = fmt::format("{}#{}", username, appname);
    applicationSet.at(username).erase(appIndex);
    applications.erase(appIndex);
    auto &redis = wukong::utils::Redis::getRedis();
    redis.srem(SET_APPLICATION_REDIS_KEY(username), appname);
    redis.del(APPLICATION_REDIS_KEY(username, appname));
    function.deleteApplication(username, appname);
}

std::string LoadBalance::Application::getApplicationInfo(const std::string &user) const {
    std::string appInfo = "[";
//    if (user.empty()) {
//        for (const auto &item: applicationSet) {
//            auto username = item.first;
//            auto appIndexes = item.second;
//            for (const auto &appIndex: appIndexes) {
//                wukong::proto::Application application = applications[appIndex];
//                auto appJsonString = messageToJson(application);
//                appInfo += fmt::format("{},", appJsonString);
//            }
//        }
//    } else if (applicationSet.contains(user)) {
//        auto appIndexes = applicationSet[user];
//        for (const auto &appIndex: appIndexes) {
//            wukong::proto::Application application = applications[appIndex];
//            auto appJsonString = messageToJson(application);
//            appInfo += fmt::format("{},", appJsonString);
//        }
//    }
    auto appIndexes = getApplication(user);
    for (const auto &appIndex: appIndexes) {
        wukong::proto::Application application = applications.at(appIndex);
        auto appJsonString = messageToJson(application);
        appInfo += fmt::format("{},", appJsonString);
    }
    if (appInfo.ends_with(','))
        appInfo = appInfo.substr(0, appInfo.length() - 1);
    appInfo += "]";
    return appInfo;
}

std::set<std::string> LoadBalance::Application::getApplication(const std::string &username) const {
    std::set<std::string> a;
    if (username.empty())
        for (const auto &item: applicationSet)
            std::set_union(a.begin(), a.end(),
                           item.second.begin(), item.second.end(),
                           std::inserter(a, a.begin()));
    else if (applicationSet.contains(username))
        a = applicationSet.at(username);
    return a;
}

void LoadBalance::Application::deleteUser(const std::string &username) {
    if (applicationSet.contains(username) && !applicationSet[username].empty())
        SPDLOG_ERROR("applicationSet is not empty");
    assert(!(applicationSet.contains(username) && !applicationSet[username].empty()));
    applicationSet.erase(username);
    function.deleteUser(username);
}

bool LoadBalance::Application::empty(const std::string &username) const {
    return getApplication(username).empty();
}

void LoadBalance::Application::createApplication(const wukong::proto::Application &application) {
    const auto &userName = application.user();
    const auto &appName = application.appname();
    if (!applicationSet.contains(userName))
        applicationSet.emplace(userName, std::set<std::string>());
    applicationSet[userName].insert(fmt::format("{}#{}", userName, appName));
    applications.emplace(fmt::format("{}#{}", userName, appName), application);
    auto &redis = wukong::utils::Redis::getRedis();
    redis.sadd(SET_APPLICATION_REDIS_KEY(userName), appName);
    redis.hmset(APPLICATION_REDIS_KEY(userName, appName), wukong::proto::messageToHash(application));
    function.addApplication(userName, appName);
}

void LoadBalance::Application::addUser(const std::string &username) {
    if (applicationSet.contains(username)) {
        SPDLOG_ERROR(fmt::format("{} is exists in applications.applicationSet", username));
        assert(false);
    }
    applicationSet.emplace(username, std::set<std::string>());
    function.addUser(username);
}

///###########################Function###################################
void
LoadBalance::Function::loadFunctions(const std::unordered_map<std::string, std::set<std::string>> &applicationSet) {
    auto &redis = wukong::utils::Redis::getRedis();
    if (loaded)
        return;
    for (const auto &item: applicationSet) {
        const auto &username = item.first;
        for (const auto &user_app: item.second) {
            const auto &appname = user_app.substr(username.size() + 1);
            auto funSet = redis.smembers(SET_FUNCTION_REDIS_KEY(username, appname));
            std::set<std::string> s;
            for (const auto &funcname: funSet) {
                auto funcHast = redis.hgetall(FUNCTION_REDIS_KEY(username, appname, funcname));
                functions.emplace(function_index(username, appname, funcname),
                                  wukong::proto::hashToFunction(funcHast));
                s.emplace(function_index(username, appname, funcname));
            }
            functionSet[username][appname] = s;
        }
    }
    SPDLOG_DEBUG(fmt::format("load Functions : {}", getFunctionInfo()));
    loaded = true;
}

bool LoadBalance::Function::pingCode(const std::string &code) {
    wukong::utils::Lib lib;
    if (lib.open(code)) {
        SPDLOG_ERROR(fmt::format("pingCode Error:{}", lib.errors()));
        return false;
    }
    FP_Func faas_ping;
    if (lib.sym("_Z9faas_pingB5cxx11v", (void **) &faas_ping)) {
        SPDLOG_ERROR(fmt::format("pingCode Error:{}", lib.errors()));
        return false;
    }
    bool pong = faas_ping() == "pong";
    lib.close();
    return pong;
}

std::pair<bool, std::string>
LoadBalance::Function::registerFuncCheck(const wukong::proto::Function &function, const std::string &code) const {
    bool success = false;
    std::string msg = "Ok";

    const auto &userName = function.user();
    const auto &appName = function.application();
    const auto &funcName = function.functionname();
    if (userName.empty() || appName.empty() || funcName.empty())
        msg = "username or appname or funcname is empty";
    else if (userName.find('#') != std::string::npos || appName.find('#') != std::string::npos)
        msg = fmt::format("{} or {} or {} is has illegal symbol \"#\"", userName, appName, funcName);
    else if (!functionSet.contains(userName))
        msg = fmt::format("{} is not exists", userName);
    else if (!functionSet.at(userName).contains(appName))
        msg = fmt::format("{}-{} is not exists", userName, appName);
    else if ((functionSet.contains(userName) &&
              functionSet.at(userName).contains(appName) &&
              functionSet.at(userName).at(appName).contains(function_index(userName, appName, funcName))) ||
             functions.contains(function_index(userName, appName, funcName)))
        msg = fmt::format("{}-{}-{} is exists", userName, appName, funcName);

    else {
        // 对code进行ping验证，即将其加载之后，调佣其faas_ping()函数，需要得到"pong"的返回值
        if (!pingCode(code)) {
            msg = "can't ping to code,Please check your code";
        } else
            success = true;
    }
    return std::make_pair(success, msg);
}

void LoadBalance::Function::registerFunction(wukong::proto::Function &function, const std::string &code) {
    const auto &userName = function.user();
    const auto &appName = function.application();
    const auto &funcName = function.functionname();

    function.set_storagekey(FUNCTION_CODE_STORAGE_KEY(userName, appName, funcName));
    functionSet[userName][appName].emplace(function_index(userName, appName, funcName));
    functions.emplace(function_index(userName, appName, funcName), function);

    auto &redis = wukong::utils::Redis::getRedis();
    redis.sadd(SET_FUNCTION_REDIS_KEY(userName, appName), funcName);
    redis.hmset(FUNCTION_REDIS_KEY(userName, appName, funcName), wukong::proto::messageToHash(function));
    redis.set(function.storagekey(), code);
}

std::pair<bool, std::string>
LoadBalance::Function::deleteFuncCheck(const std::string &userName, const std::string &appName,
                                       const std::string &funcName) {
    bool success = false;
    std::string msg = "Ok";

    if (userName.empty() || appName.empty() || funcName.empty())
        msg = "username or appname or funcname is empty";
    else if (userName.find('#') != std::string::npos || appName.find('#') != std::string::npos)
        msg = fmt::format("{} or {} or {} is has illegal symbol \"#\"", userName, appName, funcName);
    else if (!((functionSet.contains(userName) &&
                functionSet.at(userName).contains(appName) &&
                functionSet[userName][appName].contains(function_index(userName, appName, funcName))) ||
               functions.contains(function_index(userName, appName, funcName))))
        msg = fmt::format("{}-{}-{} is not exists", userName, appName, funcName);
    else {
        success = true;
    }
    return std::make_pair(success, msg);
}

void LoadBalance::Function::deleteFunction(const wukong::proto::Function &function) {
    const auto &userName = function.user();
    const auto &appName = function.application();
    const auto &funcName = function.functionname();

    const auto &storageKey = function.storagekey();

    functionSet[userName][appName].erase(function_index(userName, appName, funcName));
    functions.erase(function_index(userName, appName, funcName));

    auto &redis = wukong::utils::Redis::getRedis();
    redis.del(storageKey);
    redis.del(FUNCTION_REDIS_KEY(userName, appName, funcName));
    redis.srem(SET_FUNCTION_REDIS_KEY(userName, appName), funcName);

}

std::string LoadBalance::Function::getFunctionInfo(const std::string &username, const std::string &appname,
                                                   const std::string &funcname) const {
    std::string funcInfo = "[";
    auto funcIndexes = getFunction(username, appname, funcname);
    for (const auto &funcIndex: funcIndexes) {
        wukong::proto::Function function = functions.at(funcIndex);
        auto funcJsonString = messageToJson(function);
        funcInfo += fmt::format("{},", funcJsonString);
    }
    if (funcInfo.ends_with(','))
        funcInfo = funcInfo.substr(0, funcInfo.length() - 1);
    funcInfo += "]";
    return funcInfo;
}

std::set<std::string> LoadBalance::Function::getFunction(const std::string &username,
                                                         const std::string &appname,
                                                         const std::string &funcname) const {
    std::set<std::string> f;
    /// 直接调用返回全部
    if (appname.empty() && username.empty()) {
        for (const auto &user: functionSet) {
            for (const auto &app: user.second) {
                std::set_union(f.begin(), f.end(),
                               app.second.begin(), app.second.end(),
                               std::inserter(f, f.begin()));
            }
        }
    } else if (!appname.empty() && !username.empty()) {
        /// 全指定
        if (funcname.empty()) {
            if (functionSet.contains(username) && functionSet.at(username).contains(appname))
                f = functionSet.at(username).at(appname);
        } else if (functionSet.at(username).at(appname).contains(function_index(username, appname, funcname))) {
            f.emplace(function_index(username, appname, funcname));
        }
    } else if (appname.empty() && !username.empty()) {
        /// 仅仅指定用户
        if (functionSet.contains(username)) {
            for (const auto &app: functionSet.at(username)) {
                std::set_union(f.begin(), f.end(),
                               app.second.begin(), app.second.end(),
                               std::inserter(f, f.begin()));
            }
        }

    } else {
        /// 仅仅指定Application
        for (const auto &user: functionSet) {
            for (const auto &app: user.second) {
                if (app.first == appname) {
                    std::set_union(f.begin(), f.end(),
                                   app.second.begin(), app.second.end(),
                                   std::inserter(f, f.begin()));
                }
            }
        }
    }
    return f;
}

bool LoadBalance::Function::deleteApplicationCheck(const std::string &username, const std::string &appname) const {
    return !(functionSet.contains(username) &&
             functionSet.at(username).contains(appname) &&
             !functionSet.at(username).at(appname).empty());
}

void LoadBalance::Function::deleteApplication(const std::string &username, const std::string &appname) {
    bool check = deleteApplicationCheck(username, appname);
    if (!check) {
        SPDLOG_ERROR(fmt::format("Application {}#{} is not empty", username, appname));
        assert(false);
    }
    functionSet[username].erase(appname);
}

void LoadBalance::Function::addApplication(const std::string &username, const std::string &appname) {
    if (!functionSet.contains(username)) {
        SPDLOG_ERROR(fmt::format("user {} is not exists in functions.functionSet", username));
        assert(false);
    }
    if (functionSet.at(username).contains(appname)) {
        SPDLOG_ERROR(fmt::format("app {}#{} is exists in functions.functionSet", username, appname));
        assert(false);
    }
    functionSet[username].emplace(appname, std::set<std::string>());
}

bool LoadBalance::Function::deleteUserCheck(const std::string &username) const {
    return !(functionSet.contains(username) && !functionSet.at(username).empty());
}

void LoadBalance::Function::deleteUser(const std::string &username) {
    bool check = deleteUserCheck(username);
    if (!check) {
        SPDLOG_ERROR(fmt::format("User {} is not empty", username));
        assert(false);
    }
    functionSet.erase(username);
}

void LoadBalance::Function::addUser(const std::string &username) {
    if (functionSet.contains(username)) {
        SPDLOG_ERROR(fmt::format("user {} is exists in functions.functionSet", username));
        assert(false);
    }
    functionSet.emplace(username, std::unordered_map<std::string, std::set<std::string>>());
}

bool LoadBalance::Function::empty(const std::string &username, const std::string &appname) const {
    return getFunction(username, appname).empty();
}