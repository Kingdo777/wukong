//
// Created by kingdo on 2022/4/11.
//

#ifndef WUKONG_STRUCT_H
#define WUKONG_STRUCT_H

#include "macro.h"

enum FunctionType {
    Cpp,
    Python
};

typedef struct FunctionInfo
{
    magic_t magic_number;
    FunctionType type;
    char func_path[512];
    size_t path_size;
    int threads;
} FunctionInfo;

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

#endif // WUKONG_STRUCT_H
