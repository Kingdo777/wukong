import json
from os import mkdir
from os.path import exists, realpath, dirname
from time import localtime, strftime

import requests
from invoke import task
from wk.utils.ping import ping, test_connect
from wk.utils.env import CONFIG_FILE

GW_IP = "127.0.0.1"
GW_Port = "8080"


@task(help={'host': 'global gateway endpoint IP address, default : 127.0.0.1',
            'port': 'global gateway endpoint, default : 8080'})
def connect(context, host="127.0.0.1", port=8080):
    """
    Connect to Global Gateway.
    """
    if ping(host, port):
        if exists(CONFIG_FILE):
            f = open(CONFIG_FILE, "r")
            conf = json.loads(f.read())
            if not (conf["GW_HOST"] == host and conf["GW_PORT"] == port):
                f = open(CONFIG_FILE, "w")
                f.write(json.dumps({"GW_HOST": host, "GW_PORT": port}))
            f.close()
        else:
            conf_dir = dirname(realpath(CONFIG_FILE))
            if not exists(conf_dir):
                mkdir(conf_dir)
            f = open(CONFIG_FILE, "w")
            f.write(json.dumps({"GW_HOST": host, "GW_PORT": port}))
            f.close()
        print("Connect Success")
    else:
        print("Connect Failed, can't ping {}:{}".format(host, port))


@task()
def invokers(context):
    """
    Get Invokers Info.
    """
    conn = test_connect()
    if not conn[0]:
        print("connect global gateway failed: {}".format(conn[1]))
        return
    host = conn[2]
    port = conn[3]
    url = "http://{}:{}/get_invokers_info".format(host, port)
    try:
        response = requests.get(url)
    except requests.exceptions.RequestException:
        print("access {} failed", url)
        return
    else:
        if not response.status_code == 200:
            print("get wrong status_code : {}", response.status_code)
            return
        invokers_list = sorted(json.loads(response.text), key=lambda invoker: invoker['registerTime'])
        print("{:<20}{:<15}{:<8}{:<8}{:<8}{:<20}".
              format("invokerID",
                     "IP",
                     "port",
                     "memory",
                     "cpu",
                     "registerTime"
                     ))
        for invoker in invokers_list:
            print("{:<20}{:<15}{:<8}{:<8}{:<8}{:<20}".format(
                invoker['invokerID'],
                invoker['IP'],
                invoker['port'],
                invoker['memory'],
                invoker['cpu'],
                strftime("%Y-%m-%d %H:%M:%S", localtime(invoker['registerTime'] / 1000))
            ))
