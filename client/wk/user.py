import json
import requests
from invoke import task
from wk.utils.ping import test_connect


@task
def register(context, username="kingdo"):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/user/register".format(host, port)
    data = {
        "name": username
    }
    res = requests.post(url, data=json.dumps(data))
    if res.status_code == 200 and res.text == "Ok":
        print("Register user `{}` Success".format(username))
    else:
        print("Register user `{}` Failed with code `{}` and message `{}`".format(username, res.status_code, res.text))


@task
def delete(context, username="kingdo"):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/user/delete".format(host, port)
    data = {
        "name": username
    }
    res = requests.post(url, data=json.dumps(data))
    if res.status_code == 200 and res.text == "Ok":
        print("Delete user `{}` Success".format(username))
    else:
        print("Delete user `{}` Failed with code `{}` and message `{}`".format(username, res.status_code, res.text))


@task
def users(context, username=""):
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/user/info".format(host, port)
    cookies = {
        "user": username,
    }
    res = requests.post(url, cookies=cookies)
    if res.status_code == 200:
        user_list = json.loads(res.text)
        print("{:<20}".format("name"))
        for user in user_list:
            print("{:<20}".format(user['name']))
    else:
        print("Get Users Failed with code `{}` and message `{}`".format(res.status_code, res.text))
