//
// Created by kingdo on 2022/3/4.
//

#ifndef WUKONG_LOAD_BALANCE_H
#define WUKONG_LOAD_BALANCE_H

#include <wukong/endpoint/endpoint.h>
#include <wukong/client/client-server.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/json.h>
#include <wukong/utils/redis.h>
#include <wukong/utils/dl.h>
#include <wukong/utils/locks.h>

#include <map>

class LoadBalanceClientHandler : public wukong::client::ClientHandler {
PROTOTYPE_OF(Pistache::Aio::Handler, LoadBalanceClientHandler);

public:
    typedef wukong::client::ClientHandler Base;

    explicit LoadBalanceClientHandler(wukong::client::ClientServer *client) : wukong::client::ClientHandler(client) {}

    LoadBalanceClientHandler(const LoadBalanceClientHandler &handler) : ClientHandler(handler), functionCallQueue() {}

    void onReady(const Pistache::Aio::FdSet &fds) override;

    void registerPoller(Pistache::Polling::Epoll &poller) override;

    struct FunctionCallEntry {

        explicit FunctionCallEntry(Pistache::Http::ResponseWriter response_) : response(std::move(response_)) {}

        Pistache::Http::ResponseWriter response;
    };

    void callFunction(Pistache::Http::ResponseWriter response);

private:

    void handleFunctionCallQueue();

    void asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry &&entry);

private:
    Pistache::PollableQueue<FunctionCallEntry> functionCallQueue;
    std::map<std::string, FunctionCallEntry> functionCallMap;

    void responseFunctionCall(Pistache::Http::Code code, std::string &result, const std::string &funcEntryIndex);
};

class LoadBalance {
public:

    enum LBStatus {
        Uninitialized,
        Running,
        Stopped
    };

    LoadBalance() :
            functions(),
            applications(functions),
            users(applications) {}

    void handleInvokerRegister(const std::string &host,
                               const std::string &invokerJson,
                               Pistache::Http::ResponseWriter response);

    void handleInvokerInfo(Pistache::Http::ResponseWriter response) const {
        auto invokersInfo = invokers.getInvokersInfo();
        response.send(Pistache::Http::Code::Ok, invokersInfo);
    }

    void handleFuncRegister(wukong::proto::Function &function,
                            const std::string &code,
                            Pistache::Http::ResponseWriter response);

    void handleFuncDelete(const std::string &username,
                          const std::string &appname,
                          const std::string &funcname,
                          Pistache::Http::ResponseWriter response);

    void handleFuncInfo(const std::string &username,
                        const std::string &appname,
                        const std::string &funcname,
                        Pistache::Http::ResponseWriter response);

    void handleUserRegister(const wukong::proto::User &user, Pistache::Http::ResponseWriter response);

    void handleUserDelete(const wukong::proto::User &user, Pistache::Http::ResponseWriter response);

    void handleUserInfo(const std::string &username,
                        Pistache::Http::ResponseWriter response);

    void handleAppCreate(const wukong::proto::Application &application, Pistache::Http::ResponseWriter response);

    void handleAppDelete(const wukong::proto::Application &application, Pistache::Http::ResponseWriter response);

    void handleAppInfo(const std::string &username,
                       const std::string &appname,
                       Pistache::Http::ResponseWriter response);

    void dispatch(wukong::proto::Message &&msg, Pistache::Http::ResponseWriter response);

    void start();

    void stop();

    struct Invoker {
        /// 用于存储invokerID
        std::set<std::string> invokerSet;
        /// 使用invokerID做索引，用于存储invoker的元数据
        std::unordered_map<std::string, wukong::proto::Invoker> invokers;

        void loadInvokers(wukong::client::ClientServer &client);

        void registerInvoker(const wukong::proto::Invoker &invoker);

        std::string getInvokersInfo() const;

        std::pair<bool, std::string> invokerCheck(const wukong::proto::Invoker &invoker);

    private:
        bool loaded = false;
    };

#define function_index(username, appname, funcname) (fmt::format("{}#{}#{}",username,appname,funcname))

    struct Function {
    public:
        Function() = default;

        ///user:function:user#application#function
        std::unordered_map<std::string, std::unordered_map<std::string, std::set<std::string>>> functionSet;
        std::unordered_map<std::string, wukong::proto::Function> functions;

        void loadFunctions(const std::unordered_map<std::string, std::set<std::string>> &applicationSet);

        typedef std::string (*FP_Func)();

        std::pair<bool, std::string>
        registerFuncCheck(const wukong::proto::Function &function, const std::string &code) const;

        void registerFunction(wukong::proto::Function &function,
                              const std::string &code);

        std::pair<bool, std::string>
        deleteFuncCheck(const std::string &userName,
                        const std::string &appName,
                        const std::string &funcName);

        void deleteFunction(const wukong::proto::Function &function);

        std::string getFunctionInfo(const std::string &username = "",
                                    const std::string &appname = "",
                                    const std::string &funcname = "") const;

        std::set<std::string> getFunction(const std::string &username,
                                          const std::string &appname,
                                          const std::string &funcname = "") const;

        bool deleteApplicationCheck(const std::string &username, const std::string &appname) const;

        void deleteApplication(const std::string &username, const std::string &appname);

        void addApplication(const std::string &username, const std::string &appname);

        void deleteUser(const std::string &username);

        void addUser(const std::string &username);

        bool empty(const std::string &username = "", const std::string &appname = "") const;

    private:
        bool loaded = false;

        static bool pingCode(const std::string &code);

        bool deleteUserCheck(const std::string &username) const;
    };

    struct Application {
    public:
        explicit Application(Function &f) : function(f) {}

        /// user : user#application
        std::unordered_map<std::string, std::set<std::string>> applicationSet;
        /// user#application ： wukong::proto::Application
        std::unordered_map<std::string, wukong::proto::Application> applications;

        void loadApplications(const std::set<std::string> &usersSet);

        std::pair<bool, std::string> checkCreateApplication(const wukong::proto::Application &application) const;

        void createApplication(const wukong::proto::Application &application);

        std::pair<bool, std::string> checkDeleteApplication(const wukong::proto::Application &application) const;

        void deleteApplication(const wukong::proto::Application &application);

        std::string getApplicationInfo(const std::string &user = "") const;

        std::set<std::string> getApplication(const std::string &username = "") const;

        void deleteUser(const std::string &username);

        void addUser(const std::string &username);

        bool empty(const std::string &username = "") const;

    private:
        Function &function;

        bool loaded = false;
    };

    struct User {
    public:
        explicit User(Application &a) : application(a) {}

        std::set<std::string> userSet;
        std::unordered_map<std::string, wukong::proto::User> users;

        void loadUser();

        std::pair<bool, std::string> registerUserCheck(const wukong::proto::User &user) const;

        std::pair<bool, std::string> deleteUserCheck(const wukong::proto::User &user) const;

        void registerUser(const wukong::proto::User &user);


        void deleteUser(const wukong::proto::User &user);

        std::string getUserInfo() const;

    private:
        Application &application;

        bool loaded = false;
    };

private:

    void load();

    LBStatus status = Uninitialized;

    std::shared_ptr<LoadBalanceClientHandler> pickOneHandler();

    wukong::client::ClientServer cs;

    std::mutex uaf_mutex;       /// 给User、App、Func三者上一把大锁
    Function functions;
    Application applications;
    User users;

    Invoker invokers;
};


#endif //WUKONG_LOAD_BALANCE_H
