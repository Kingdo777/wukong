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
#include <utility>

class LoadBalance;

class LoadBalanceClientHandler : public wukong::client::ClientHandler {
PROTOTYPE_OF(Pistache::Aio::Handler, LoadBalanceClientHandler);

public:
    typedef wukong::client::ClientHandler Base;

    LoadBalanceClientHandler(wukong::client::ClientServer *client, LoadBalance *lb)
            : wukong::client::ClientHandler(client), loadBalance(lb) {}

    LoadBalanceClientHandler(const LoadBalanceClientHandler &handler) : ClientHandler(handler),
                                                                        loadBalance(handler.loadBalance) {}

    void onReady(const Pistache::Aio::FdSet &fds) override;

    void registerPoller(Pistache::Polling::Epoll &poller) override;

    struct FunctionCallEntry {

        explicit FunctionCallEntry(std::string host_,
                                   std::string port_,
                                   std::string uri_,
                                   bool is_async_,
                                   std::string resultKey_,
                                   std::string data_,
                                   Pistache::Http::ResponseWriter response_) :
                instanceHost(std::move(host_)),
                instancePort(std::move(port_)),
                funcname(std::move(uri_)),
                data(std::move(data_)),
                resultKey(std::move(resultKey_)),
                is_async(is_async_),
                response(std::move(response_)) {}

        /// curl -X POST -d "data" http://ip:port/uri
        std::string instanceHost;
        std::string instancePort;
        std::string funcname;
        std::string data;   /// proto::Message的json序列
        std::string resultKey;
        bool is_async;

        Pistache::Http::ResponseWriter response;
    };

    enum ResponseType {
        FunctionCall,
        StartupInstance
    };

    struct ResponseEntry {
        ResponseEntry(Pistache::Http::Code code_,
                      std::string result_,
                      std::string index_,
                      ResponseType type_) :
                code(code_),
                result(std::move(result_)),
                index(std::move(index_)),
                type(type_) {}

        Pistache::Http::Code code;
        std::string result;
        std::string index;
        ResponseType type;
    };

    struct StartupInstanceEntry {
        StartupInstanceEntry(std::string host_,
                             std::string port_,
                             std::string data_,
                             wukong::proto::Instance instance_,
                             FunctionCallEntry entry) :
                invokerHost(std::move(host_)),
                invokerPort(std::move(port_)),
                data(std::move(data_)),
                instance(std::move(instance_)),
                funcCallEntry(std::move(entry)) {}

        std::string invokerHost;
        std::string invokerPort;
        std::string data;   /// proto::Application的json序列

        wukong::proto::Instance instance;
        FunctionCallEntry funcCallEntry;
    };


    void callFunction(FunctionCallEntry entry);

    void startupInstance(StartupInstanceEntry entry);

    LoadBalance *lb() {
        return this->loadBalance;
    }

private:
    Pistache::PollableQueue<StartupInstanceEntry> startupInstanceQueue;
    std::map<std::string, StartupInstanceEntry> startupInstanceMap;
    std::mutex startupInstanceMapMutex;

    Pistache::PollableQueue<FunctionCallEntry> functionCallQueue;
    std::map<std::string, FunctionCallEntry> functionCallMap;
    std::mutex functionCallMapMutex;

    Pistache::PollableQueue<ResponseEntry> responseQueue;

    void handleFunctionCallQueue();

    void asyncCallFunction(LoadBalanceClientHandler::FunctionCallEntry &&entry);

    void handleStartupInstanceQueue();

    void asyncStartupInstance(LoadBalanceClientHandler::StartupInstanceEntry &&entry);

    void handleResponseQueue();


    void responseStartupInstance(Pistache::Http::Code code, const std::string &result,
                                 LoadBalanceClientHandler *h,
                                 const std::string &startupInstanceEntryIndex);

    void responseFunctionCall(Pistache::Http::Code code, const std::string &result,
                              LoadBalanceClientHandler *h,
                              const std::string &funcEntryIndex);

    LoadBalance *loadBalance = nullptr;
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

        bool isReconnect(const wukong::proto::Invoker &invoker);

        std::pair<bool, std::string> invokerCheck(const wukong::proto::Invoker &invoker);

        bool empty() const {
            return invokerSet.empty();
        }

        size_t size() const {
            WK_CHECK_WITH_ASSERT(invokerSet.size() == invokers.size(), "invokerSet.size() ！= invokers.size()");
            return invokerSet.size();
        }

    private:
        bool loaded = false;
    };

    struct Instance {
        /// 使用username#appname#invokerID做索引，用于存储Instance的元数据
        std::unordered_map<std::string, wukong::proto::Instance> instanceMeta;
        //  std::unordered_map<std::string, std::mutex> instanceMutex;

        void update(const std::string &key) const {
            WK_CHECK_WITH_ASSERT(contains(key), "Instance is not exist");
            if (contains(key)) {
                //  wukong::utils::UniqueLock lock(instanceMutex[key]);
            }
        }

        void add(const std::string &key, const wukong::proto::Instance &instance) {
            WK_CHECK_WITH_ASSERT(!contains(key), "Instance is exist");
            instanceMeta.emplace(key, instance);
            //  instanceMutex.emplace(key, std::mutex());
        }

        void remove(const std::string &key) {
            WK_CHECK_WITH_ASSERT(contains(key), "Instance is not exist");
            instanceMeta.erase(key);
            //  instanceMutex.erase(key);
        }

        const wukong::proto::Instance &get(const std::string &key) {
            WK_CHECK_WITH_ASSERT(contains(key), "Instance is not exist");
            return instanceMeta.at(key);
        }

        bool contains(const std::string &key) const {
            return instanceMeta.contains(key);
        }

        static std::string key(const std::string &username,
                               const std::string &appname,
                               const std::string &invokerID) {
            return fmt::format("{}#{}#{}", username, appname, invokerID);
        };

        bool empty() const {
            return instanceMeta.empty();
        }

        size_t size() const {
            return instanceMeta.size();
        }
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

        std::set<std::string> getFunction(const std::string &username = "",
                                          const std::string &appname = "",
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

    void addInstance(const std::string &key, const wukong::proto::Instance &instance) {
        wukong::utils::WriteLock lock(instances_share_mutex);
        instances.add(key, instance);
    }

private:

    void load();

    LBStatus status = Uninitialized;

    std::shared_ptr<LoadBalanceClientHandler> pickOneHandler();

    wukong::client::ClientServer cs;

    std::shared_mutex uaf_mutex;       /// 给User、App、Func三者上一把大锁
    Function functions;
    Application applications;
    User users;

    std::shared_mutex invokers_mutex;
    Invoker invokers;

    /// appIndex#InvokerID ---> Instance , Instance是和application强绑定的！
    Instance instances;
    /// 对于map自身的添加删除，应该上写锁，但是对于Instance的读写，应该上读锁，和Instance自身的互斥锁
    std::shared_mutex instances_share_mutex;

};


#endif //WUKONG_LOAD_BALANCE_H
