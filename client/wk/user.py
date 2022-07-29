import json
import random
import string
import requests
from invoke import task
from wk.utils.ping import test_connect
import wk.utils.auth as au


@task
def register(context, username="kingdo"):
    if au.contains(username):
        print("Register user `{}` Failed, {} has Registered!".format(username, username))
        return
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/user/register".format(host, port)
    auth = ''.join(random.choice(string.ascii_lowercase) for x in range(64))
    data = {
        "name": username,
        "auth": auth
    }
    res = requests.post(url, data=json.dumps(data))
    if res.status_code == 200 and res.text == "Ok":
        au.add_user(username, auth)
        print("Register user `{}` Success".format(username))
    else:
        print("Register user `{}` Failed with code `{}` and message `{}`".format(username, res.status_code, res.text))


@task
def delete(context, username="kingdo"):
    auth = au.auth(username)
    if auth == "":
        print("the auth of {} is empty, which is unavailable".format(username))
        return
    (success, msg, host, port) = test_connect()
    if not success:
        print("connect global gateway failed: {}".format(msg))
        return
    url = "http://{}:{}/user/delete".format(host, port)
    data = {
        "name": username,
        "auth": auth
    }
    res = requests.post(url, data=json.dumps(data))
    if res.status_code == 200 and res.text == "Ok":
        au.del_user(username)
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
        # We Shouldn't Show the Auth in the actual production env!
        print("{:<20} {:^70}".format("name", "auth"))
        for user in user_list:
            print("{:<20} {:^70}".format(user['name'], user['auth']))
    else:
        print("Get Users Failed with code `{}` and message `{}`".format(res.status_code, res.text))

# if __name__ == '__main__':
#     register("")
#     users("")
#     delete("")
