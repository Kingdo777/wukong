//
// Created by kingdo on 2022/3/28.
//

#ifndef WUKONG_LOCAL_GATEWAY_H
#define WUKONG_LOCAL_GATEWAY_H

#include "LocalGatewayEndpoint.h"
#include <boost/dll.hpp>
#include <queue>
#include <utility>
#include <wukong/proto/proto.h>
#include <wukong/utils/errors.h>
#include <wukong/utils/locks.h>
#include <wukong/utils/os.h>
#include <wukong/utils/process/DefaultSubProcess.h>
#include <wukong/utils/reactor/Reactor.h>
#include <wukong/utils/struct.h>

#define function_index(username, appname, funcname) (fmt::format("{}#{}#{}", username_, appname_, funcname))

class LocalGatewayHandler;
class LocalGateway;

enum RequestType {
    ExternalRequest_Type,
    InternalRequest_Type,
    InternalStorageRequest_Type
};

class RequestEntry
{
public:
    RequestEntry(wukong::proto::Message msg_, RequestType type_);

    RequestType type;
    wukong::proto::Message msg;
};

class InternalRequestEntry : public RequestEntry
{
public:
    InternalRequestEntry(wukong::proto::Message msg_, int internalResponseFD_);
    int internalResponseFD = -1;
};

class ExternalRequestEntry : public RequestEntry
{
public:
    ExternalRequestEntry(wukong::proto::Message msg_, Pistache::Http::ResponseWriter response_);
    Pistache::Http::ResponseWriter response;
};

struct FunctionInstances;
struct FunctionInstanceGroup;

/// corresponding to each Worker/Storage Function, mainly for available slots,
/// and the handler thread associated with it
struct FunctionInstanceInfo
{
    FunctionInstanceInfo(std::shared_ptr<LocalGatewayHandler> handler,
                         std::string funcInst_uuid);

    void dispatch(std::shared_ptr<RequestEntry> entry, std::shared_ptr<FunctionInstanceInfo> inst, int64_t need_slots_or_freeSize = 1);

    void sendRequest(const wukong::proto::Message& msg) const;

    /** 这里的逻辑比较难理解：
     * read_write_fd 和write_read_fd:
     * 对于子进程而言，前者用于读请求，后者用于写结果；
     * 然而对于父进程，前者用于写请求，后者用于读结果，因此子进程监听read_write_fd ， 而父进程监听write_read_fd。
     * 因此read_write_fd 对于子进程是read_fd，对于父进程是write_fd; write_read_fd对于子进程是write_fd，对于父进程是read_fd。
     *
     * request_response_fd和response_request_fd:
     * 对于子进程而言，前者用于发送内部请求，而后者用于接收内部请求的结果；
     * 对于父进程又刚好相反前者用于返回内部请求结果，而后者用于接收内部请求。
     * 但是，与前面不同的是，request_response_fd对于父子进程而言，都是request_fd, response_request_fd在父子进程中都是response_fd。
     * 子进程中，request是动词，response则是名词，因此监听的是response_fd，即response_request_fd；
     * 父进程中，request则是名词，而response是动词，因此监听的是request_fd, 即request_response_fd
     * */

    [[nodiscard]] int getReadFD() const;

    [[nodiscard]] int getWriteFD() const;

    [[nodiscard]] int getRequestFD() const;

    [[nodiscard]] int getResponseFD() const;

    std::shared_ptr<LocalGatewayHandler> handler;
    std::string funcInst_uuid;

    int fds[4] = { -1, -1, -1, -1 };

    std::shared_ptr<FunctionInstanceGroup> instGroup;
};

struct FunctionInstanceGroup
{
    FunctionInstanceGroup(uint32_t groupSize, std::shared_ptr<FunctionInstances> instList)
        : groupSize(groupSize)
        , instList(std::move(instList))
    { }

    std::string funcInst_uuid_prefix;

    std::vector<std::shared_ptr<FunctionInstanceInfo>> group;
    uint32_t groupSize;
    uint32_t index = 0;

    std::shared_ptr<FunctionInstanceInfo> loadBalanceOneInst();

    /// for WorkerFunc, it means how many available concurrent slots left
    /// for StorageFunc, it means how many available space for storage data
    int64_t slots_or_freeSize = 0;

    std::shared_ptr<FunctionInstances> instList;
};

struct FunctionInstances
{
    std::list<std::shared_ptr<FunctionInstanceGroup>> list;
    FunctionInstanceType type = WorkerFunction;

    int64_t need_Slots_or_allFreeSize     = 0;
    int64_t actual_Slots_or_allFreeSize   = 0;
    int64_t creating_Slots_or_allFreeSize = 0;

    [[nodiscard]] bool needCreateInst() const
    {
        return creating_Slots_or_allFreeSize < need_Slots_or_allFreeSize;
    }
};

class ResponseEntry
{
public:
    ResponseEntry(std::shared_ptr<FunctionInstanceInfo> inst, std::shared_ptr<RequestEntry> request);

    std::shared_ptr<FunctionInstanceInfo> inst;

    std::shared_ptr<RequestEntry> request;
};

class StorageFuncEphemeralDataRecord
{
public:
    explicit StorageFuncEphemeralDataRecord(bool need_lock = true);

    bool contains(const std::string& SF_dada_uuid);

    std::shared_ptr<FunctionInstanceInfo> getInst(const std::string& SF_dada_uuid);

    void getStorageShm(const std::string& SF_dada_uuid, std::string& result);

    void deleteShmDone(const std::shared_ptr<FunctionInstanceInfo>& inst, size_t length);

    void createShmDone(const std::shared_ptr<FunctionInstanceInfo>& inst, const std::string& SF_dada_uuid, size_t length);

private:
    struct StorageFuncEphemeralData
    {
        StorageFuncEphemeralData(std::string SF_dada_uuid, size_t size, std::shared_ptr<FunctionInstanceInfo> inst);

        std::string SF_dada_uuid;
        size_t data_size;
        std::shared_ptr<FunctionInstanceInfo> inst;
    };

    bool need_lock = true;
    std::mutex mutex;

    std::unordered_map<std::string, std::shared_ptr<StorageFuncEphemeralData>> SF_DadaUUID2Inst_map;
};

class LocalGatewayHandler : public Pistache::Aio::Handler
{
    PROTOTYPE_OF(Pistache::Aio::Handler, LocalGatewayHandler);

public:
    struct FDsRecord
    {
        explicit FDsRecord(bool need_lock = true);

        void add(int read_fd, int request_fd, int response_fd);

        bool hasReadFD(int read_fd);

        bool hasRequestFD(int request_fd);

        int getResponseFD(int request_fd);

    private:
        std::set<int> readFD_set;
        std::set<int> requestFD_set;
        std::unordered_map<int, int> requestFD_2_responseFD_map;

        bool need_lock;
        std::shared_mutex mutex;
    };

public:
    explicit LocalGatewayHandler(LocalGateway* lg_);

    LocalGatewayHandler(const LocalGatewayHandler& handler);

    void onReady(const Pistache::Aio::FdSet& fds) override;

    void registerPoller(Pistache::Polling::Epoll& poller) override;

    void putRequest(std::shared_ptr<RequestEntry> requestEntry, std::shared_ptr<FunctionInstanceInfo> inst);

    /// called by LG, to bing Inst and handle
    void addInst(const std::string& funcInst_uuid, const std::shared_ptr<FunctionInstanceInfo>& inst);

private:
    /// that req is dispatched by LG, handler needs send it to the Inst associated with it
    void sendReq_fromLG_toFuncInst();
    /// receive internal-Rqe, construct the Message, and send to LG
    void receiveInternalReq_ConsMsg_submit2LG(int request_fd);

    /// receive Response from Inst, and back the result to user/function
    void receiveResponse_classifyHandle(int read_fd);
    /// classify the result and Handle it(back to user/function and submit to LG)
    void handleExternalResult(const FuncResult& result);
    void handleInternalWorkerResult(const FuncResult& result);
    void handleInternalStorageResult(const FuncResult& result);

    struct LGHandlerRequestEntry
    {
        LGHandlerRequestEntry(std::shared_ptr<RequestEntry> request, std::shared_ptr<FunctionInstanceInfo> inst)
            : request(std::move(request))
            , inst(std::move(inst)) {};
        std::shared_ptr<RequestEntry> request;
        std::shared_ptr<FunctionInstanceInfo> inst;
    };

    Pistache::PollableQueue<LGHandlerRequestEntry> requestQueue;

    FDsRecord fdsRecord;

    std::unordered_map<uint64_t, std::shared_ptr<ResponseEntry>> responseRecord;

    LocalGateway* lg;
};

class LocalGateway : public Reactor
{
public:
    struct Options
    {
        friend class LocalGateway;

        Options();

        static Options options();

        Options& threads(int val);

    private:
        int threads_;
    };

public:
    LocalGateway();

    void init(Options& options);

    void run() override;

    void shutdown() override;

    void initFuncPool();

    bool checkUser(const std::string& username_);

    bool checkApp(const std::string& appname_);

    static bool existFunCode(const std::string& funcname, FunctionType type);

    wukong::proto::Function getFunction(const std::string& funcname);
    void addFunction(const std::string& funcname, const wukong::proto::Function& func);

    std::pair<bool, std::string> loadFuncCode(const std::string& funcname, FunctionType type, bool update = false);

    std::pair<bool, std::string> initApp(const std::string& username_, const std::string& appname_);

    typedef std::string (*FP_Func)();

    static bool PingCode(const boost::filesystem::path& lib_path);

    void externalCall(const wukong::proto::Message& msg, Pistache::Http::ResponseWriter response);
    void internalCall(const wukong::proto::Message& msg, int responseFD);
    void putResult(const ResponseEntry& entry);

    std::string username() const;

    std::string appname() const;

    StorageFuncEphemeralDataRecord storageFuncEphemeralDataRecord;

private:
    std::shared_ptr<LocalGatewayHandler> pickOneHandler();

    void onReady(const Pistache::Polling::Event& event) override;

    void handlerRequest();

    void handlerWorkerRequest(const std::shared_ptr<RequestEntry>& entry);

    void handlerStorageRequest(const std::shared_ptr<InternalRequestEntry>& entry);

    void toWaitQueue_and_CreateNewInst(const std::shared_ptr<RequestEntry>& entry, const std::string& funcname);

    /// for send the create-req to FP to create instance.
    void CreateFuncInst(const std::string& funcname);

    void instCreateDone();

    bool checkInstList_and_dispatchReq(const std::shared_ptr<RequestEntry>& entry, const std::string& funcname, int64_t need_Slots_or_FreeSize = 1);

    void handlerResult();

    void killAllProcess();

    wukong::utils::Redis& redis = wukong::utils::Redis::getRedis();

    LocalGatewayEndpoint endpoint;

    std::string username_;
    std::string appname_;

    Pistache::PollableQueue<std::shared_ptr<RequestEntry>> readyQueue;
    std::unordered_map<std::string, std::queue<std::shared_ptr<RequestEntry>>> waitQueue;

    Pistache::PollableQueue<ResponseEntry> resultQueue;

    std::shared_mutex functions_shared_mutex;
    std::unordered_map<std::string, wukong::proto::Function> functions;

    boost::filesystem::path func_pool_exec_path;

    std::unordered_map<std::string, std::shared_ptr<FunctionInstances>> funcInstanceList_map;

    wukong::utils::DefaultSubProcess func_pool_process;
    int func_pool_read_fd  = -1;
    int func_pool_write_fd = -1;
};

#endif // WUKONG_LOCAL_GATEWAY_H