//
// Created by kingdo on 2022/4/11.
//

#include <wukong/utils/struct.h>
const char* StorageFuncOpTypeName[Unknown] = {
    "Create",
    "Delete",
    "Get"
};

const char* PipeNameString[pipeCount] = {
    "read-write",
    "write_read",
    "request-response",
    "response-request"
};

boost::filesystem::path getFunCodePath(const std::string& funcname, FunctionType type)
{
    switch (type)
    {
    case FunctionType::Python: {
        return boost::filesystem::path("/tmp/wukong/func-code/python").append(funcname + ".py");
    }
    case FunctionType::Cpp: {
        return boost::filesystem::path("/tmp/wukong/func-code/cpp").append(funcname).append("lib.so");
    }
    case FunctionType::WebAssembly: {
        return boost::filesystem::path("/tmp/wukong/func-code/webAssembly").append(funcname).append("func");
    }
    case StorageFunc:;
    }
    WK_CHECK_WITH_EXIT(false, "Unreachable");
    return "";
}
