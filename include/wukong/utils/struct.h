//
// Created by kingdo on 2022/4/11.
//

#ifndef WUKONG_STRUCT_H
#define WUKONG_STRUCT_H

#include "macro.h"

typedef struct FunctionInfo
{
    char lib_path[256] = { 0 };
    int threads        = 0;
} FunctionInfo;

typedef struct FuncResult
{
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
    Unknown
};
extern const char* StorageFuncOpTypeName[Unknown];

#endif // WUKONG_STRUCT_H
