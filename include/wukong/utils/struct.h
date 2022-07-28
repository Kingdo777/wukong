//
// Created by kingdo on 2022/4/11.
//

#ifndef WUKONG_STRUCT_H
#define WUKONG_STRUCT_H

#include "macro.h"

enum FunctionInstanceType {
    WorkerFunction,
    StorageFunction,
    InstanceTypeCount
};

enum FunctionType {
    Cpp,
    Python,
    WebAssembly,
    StorageFunc
};

typedef struct GreetingMsg
{
    GreetingMsg() = default;
    explicit GreetingMsg(std::string& msg_)
    {
        strcpy(msg, msg_.c_str());
    }

    explicit GreetingMsg(const char* msg_)
    {
        strcpy(msg, msg_);
    }

    [[nodiscard]] bool equal(const char* want) const
    {
        return std::string(msg) == std::string(want);
    }

    [[nodiscard]] bool equal(const std::string& want) const
    {
        return std::string(msg) == want;
    }
    std::string to_string()
    {
        return std::string { msg };
    }

private:
    char msg[32] {};
} GreetingMsg;

typedef struct FuncCreateMsg
{
    FuncCreateMsg() = default;
    FuncCreateMsg(const std::string& funcname_,
                  FunctionType type_,
                  FunctionInstanceType instanceType_,
                  uint32_t workers_ = 1,
                  uint32_t threads_ = 1,
                  uint32_t cores    = 1000,
                  uint32_t memory   = 64)
        : type(type_)
        , instanceType(instanceType_)
        , workers(workers_)
        , threads(threads_)
        , cores(cores)
        , memory(memory)
    {
        funcname_size = funcname_.size();
        if (funcname_size >= WUKONG_FUNC_NAME_SIZE)
        {
            SPDLOG_ERROR("func path is too long (size is {}, which is bigger than WUKONG_FUNC_PATH_SIZE({}))", funcname_size, WUKONG_FUNC_NAME_SIZE - 1);
            funcname_size = WUKONG_FUNC_NAME_SIZE - 1;
        }
        strncpy(funcname, funcname_.data(), funcname_size);
    }
    magic_t magic_number              = 0;
    FunctionType type                 = FunctionType::Cpp;
    FunctionInstanceType instanceType = FunctionInstanceType::WorkerFunction;
    char funcname[WUKONG_FUNC_NAME_SIZE] {};

    size_t funcname_size = 0;
    uint32_t workers     = 1;
    uint32_t threads     = 1;
    uint32_t cores       = 1000;
    uint32_t memory      = 64;

} FuncCreateMsg;

typedef struct FuncCreateDoneMsg
{
    FuncCreateDoneMsg()  = default;
    magic_t magic_number = 0;

    char funcname[WUKONG_FUNC_NAME_SIZE] {};
    size_t funcname_size = 0;

    char PipeArray[4][WUKONG_NAMED_PIPE_SIZE] {};
    size_t PipeSizeArray[4] {};

    char funcInst_uuid[WUKONG_UUID_SIZE] {};
    size_t uuidSize = 0;

    FunctionInstanceType instType = InstanceTypeCount;

} FuncCreateDoneMsg;

/// #1_#2PipePath
/// for sub-process, #1 is meaningful
/// for parent-process, #2 is meaningful
enum PipeIndex {
    read_writePipePath,
    write_readPipePath,
    request_responsePipePath,
    response_requestPipePath,
    pipeCount
};

extern const char* PipeNameString[pipeCount];

typedef struct FuncResult
{
public:
    FuncResult() = default;
    FuncResult(bool success_, const std::string& data_, uint64_t request_id_)
        : magic_number(MAGIC_NUMBER_WUKONG)
        , success(success_)
        , data {}
        , data_size(data_.size())
        , request_id(request_id_)
    {
        if (data_size > WUKONG_MESSAGE_SIZE)
        {
            success = false;
            strcpy(data, "data is to large!");
        }
        else
        {
            strcpy(data, data_.data());
        }
    }
    magic_t magic_number;
    bool success;
    char data[WUKONG_MESSAGE_SIZE];
    size_t data_size;
    uint64_t request_id;
} FuncResult;

typedef struct InternalRequest
{
    magic_t magic_number           = 0;
    char funcname[256]             = { 0 };
    char args[WUKONG_MESSAGE_SIZE] = { 0 };
    uint64_t request_id            = 0;
} InternalRequest;

enum StorageFuncOpType {
    Create = 0,
    Delete = 1,
    Get    = 2,
    Unknown
};
extern const char* StorageFuncOpTypeName[Unknown];

boost::filesystem::path getFunCodePath(const std::string& funcname, FunctionType type);

#endif // WUKONG_STRUCT_H
