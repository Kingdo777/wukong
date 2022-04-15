//
// Created by kingdo on 2022/4/6.
//

#include <wukong/utils/config.h>
#include <wukong/utils/env.h>
#include <wukong/utils/errors.h>
#include <wukong/utils/macro.h>
#include <wukong/utils/os.h>
#include <wukong/utils/signal-tool.h>
#include <wukong/utils/timing.h>

#include <boost/filesystem.hpp>
#include <Python.h>

#include "faas/python/function-interface.h"

#define FAAS_MAIN_FUNC_NAME "faas_main"

void link_()
{
    interface_link();
}

int main()
{
    wukong::utils::initLog("test-python");
    boost::filesystem::path workingDir("/home/kingdo/CLionProjects/wukong/tests/python/py-source");
    std::string pythonModuleName = "find_lib.py";

    Py_InitializeEx(0);

    PyObject* numpyModule = PyImport_ImportModule("numpy");
    if (!numpyModule)
    {
        printf("\nFailed to import numpy\n");
    }
    else
    {
        printf("\nPython initialised numpy\n");
    }

    PyObject* sys  = PyImport_ImportModule("sys");
    PyObject* path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString(workingDir.c_str()));

    PyObject* module = PyImport_ImportModule(pythonModuleName.c_str());
    WK_CHECK_WITH_ERROR_HANDLE_and_RETURN_VALUR(module,
                                                fmt::format("Failed to load {}", pythonModuleName),
                                                1,
                                                [](const std::string&) { PyErr_Print(); });

    PyObject* func = PyObject_GetAttrString(module, FAAS_MAIN_FUNC_NAME);
    WK_CHECK_WITH_ERROR_HANDLE_and_RETURN_VALUR(func && PyCallable_Check(func),
                                                fmt::format("Can't find func `{}` form module `{}`", FAAS_MAIN_FUNC_NAME, pythonModuleName),
                                                1,
                                                [](const std::string&) { PyErr_Print(); });
    /// 设置参数
    PyObject* pythonFuncArgs = nullptr;
    //    PyObject* inputBytes;
    //    inputBytes     = PyBytes_FromStringAndSize("", 0);
    //    pythonFuncArgs = PyTuple_New(1);
    //    PyTuple_SetItem(pythonFuncArgs, 0, inputBytes);

    /// 执行
    PyObject* returnValue = PyObject_CallObject(func, pythonFuncArgs);

    /// 清除参数的内存空间
    //    if (pythonFuncArgs != nullptr)
    //    {
    //        Py_DECREF(pythonFuncArgs);
    //        Py_DECREF(inputBytes);
    //    }

    WK_CHECK_WITH_ERROR_HANDLE_and_RETURN_VALUR(returnValue,
                                                fmt::format("Python call `{}-{}` failed`", pythonModuleName, FAAS_MAIN_FUNC_NAME),
                                                1,
                                                [=](const std::string&) {
                                                    PyErr_Print();
                                                    Py_DECREF(func);
                                                    Py_DECREF(module);
                                                });
    long ret = PyLong_AsLong(returnValue);
    if (ret == 0)
    {
        SPDLOG_INFO("Python call succeeded");
    }
    else
    {
        SPDLOG_ERROR("Python call failed (return value = {})", ret);
    }
    Py_DECREF(returnValue);

    Py_XDECREF(func);
    Py_DECREF(module);
    Py_FinalizeEx();
    return (int)ret;
}