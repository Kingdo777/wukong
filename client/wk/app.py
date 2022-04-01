import json
import requests
from invoke import task
from wk.utils.ping import test_connect


@task
def create(context, username="kingdo", appname="test"):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/application/create".format(host, port)
    data = {
        "name": appname,
        "user": username
    }
    res = requests.post(url, data=json.dumps(data))
    if res.status_code == 200 and res.text == "Ok":
        print("Create application `{}#{}` Success".format(username, appname))
    else:
        print("Create application `{}#{}` Failed with code `{}` and message `{}`".format(username,
                                                                                         appname,
                                                                                         res.status_code,
                                                                                         res.text))


@task
def delete(context, username="kingdo", appname="test"):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/application/delete".format(host, port)
    data = {
        "name": appname,
        "user": username
    }
    res = requests.post(url, data=json.dumps(data))
    if res.status_code == 200 and res.text == "Ok":
        print("Delete application `{}#{}` Success".format(username, appname))
    else:
        print("Delete application `{}#{}` Failed with code `{}` and message `{}`".format(username,
                                                                                         appname,
                                                                                         res.status_code,
                                                                                         res.text))


@task
def apps(context, username="", appname=""):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/application/info".format(host, port)
    cookies = {
        "user": username,
        "application": appname,
    }
    res = requests.post(url, cookies=cookies)
    if res.status_code == 200:
        user_list = json.loads(res.text)
        print("{:<10}{:<10}".format("user", "name"))
        for application in user_list:
            print("{:<10}{:<10}".format(application['user'], application['name']))
    else:
        print("Get Users Failed with code `{}` and message `{}`".format(res.status_code, res.text))
