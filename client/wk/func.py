import json
import requests
from invoke import task
from os.path import exists, join
from wk.utils.ping import test_connect
from wk.utils.env import WUKONG_BUILD_DIR


@task
def register(context, file="", username='kingdo', appname='test', funcname='hello', func_type='c/cpp',
             concurrency=1,
             memory=1024,
             cpu=1000):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    if file != "" and exists(file):
        conf = json.loads(open(file, "r").read())
        username, appname, funcname, concurrency, memory, cpu, func_type = \
            conf['user'], conf['test'], conf['hello'], conf['concurrency'], conf['memory'], conf['cpu'], conf['type']
    function_so_path = join(WUKONG_BUILD_DIR, "lib", "libfunc_{}.so".format(funcname))
    if not exists(function_so_path):
        print("{} is not exists".format(function_so_path))
        return
    elif username == '':
        print("user is empty")
        return
    elif appname == '':
        print("application is empty")
        return
    elif funcname == '':
        print("function is empty")
        return
    elif concurrency < 1 or concurrency > 1000:
        print("concurrency < 1 or concurrency > 1000")
        return
    elif memory < 64 or memory > 1024:
        print("memory < 64 or memory > 1024")
        return
    elif cpu < 100 or cpu > 4000:
        print("cpu < 100 or cpu > 4000")
        return
    elif func_type != 'c/cpp' and func_type != 'python':
        print("type must is c/cpp or python")
        return
    else:
        url = "http://{}:{}/function/register".format(host, port)
        cookies = {
            "user": username,
            "application": appname,
            "function": funcname,
            "concurrency": str(concurrency),
            "memory": str(memory),
            "cpu": str(cpu),
            "type": str(func_type)
        }
        res = requests.post(url, data=open(function_so_path, "rb"), cookies=cookies)
        if res.status_code == 200 and res.text == "Ok":
            print("Register Function `{}#{}#{}` Success".format(username, appname, funcname))
        else:
            print("Register Function `{}#{}#{}` Failed with code `{}` and message `{}`".format(username,
                                                                                               appname,
                                                                                               funcname,
                                                                                               res.status_code,
                                                                                               res.text))


@task
def delete(context, username='kingdo', appname='test', funcname='hello'):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/function/delete".format(host, port)
    cookies = {
        "user": username,
        "application": appname,
        "function": funcname,
    }
    res = requests.post(url, cookies=cookies)
    if res.status_code == 200 and res.text == "Ok":
        print("Delete Function `{}#{}#{}` Success".format(username, appname, funcname))
    else:
        print("Delete Function  `{}#{}#{}` Failed with code `{}` and message `{}`".format(username,
                                                                                          appname,
                                                                                          funcname,
                                                                                          res.status_code,
                                                                                          res.text))


@task
def funcs(context, username='', appname='', funcname=''):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/function/info".format(host, port)
    cookies = {
        "user": username,
        "application": appname,
        "function": funcname,
    }
    res = requests.post(url, cookies=cookies)
    if res.status_code == 200:
        func_list = json.loads(res.text)
        print("{:^10}{:^10}{:^15}{:^15}{:^10}{:^15}{:^25}".format(
            "name",
            "user",
            "application",
            "concurrency",
            "memory",
            "cpu",
            "storage-key"))
        for func in func_list:
            print("{:^10}{:^10}{:^15}{:^15}{:^10}{:^15}{:^25}".format(
                func['name'],
                func['user'],
                func['application'],
                func['concurrency'],
                "{}MB".format(func['memory']),
                "{:.1f} core".format(float(func['cpu']) / 1000.0),
                func['storageKey'],
            ))
    else:
        print("Create application `{}#{}#{}` Failed with code `{}` and message `{}`".format(username,
                                                                                            appname,
                                                                                            funcname,
                                                                                            res.status_code,
                                                                                            res.text))
