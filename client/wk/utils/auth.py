import json

from .env import USER_AUTH_FILE


def contains(username):
    f = open(USER_AUTH_FILE, "r")
    try:
        users_auth = dict(json.loads(f.read()))
    except json.JSONDecodeError:
        f.close()
        print("Something Wrong when open the AUTH_FILE!")
        return False
    else:
        f.close()
        return users_auth.__contains__(username)


def add_user(username, auth_):
    if contains(username):
        print("{} has exists!".format(username))
        return False
    f = open(USER_AUTH_FILE, "r")
    try:
        users_auth = dict(json.loads(f.read()))
    except json.JSONDecodeError:
        print("Something Wrong when open the AUTH_FILE!")
        return False
    else:
        f.close()
        users_auth[username] = auth_
        f = open(USER_AUTH_FILE, "w")
        f.write(json.dumps(users_auth))
        return True


def del_user(username):
    if not contains(username):
        print("{} is not exists!".format(username))
        return False
    f = open(USER_AUTH_FILE, "r")
    try:
        users_auth = dict(json.loads(f.read()))
    except json.JSONDecodeError:
        print("Something Wrong when open the AUTH_FILE!")
        return False
    else:
        f.close()
        users_auth.__delitem__(username)
        f = open(USER_AUTH_FILE, "w")
        f.write(json.dumps(users_auth))
        return True


def auth(username):
    if not contains(username):
        print("{} is not exists!".format(username))
        return ""
    f = open(USER_AUTH_FILE, "r")
    try:
        users_auth = dict(json.loads(f.read()))
    except json.JSONDecodeError:
        print("Something Wrong when open the AUTH_FILE!")
        return ""
    else:
        f.close()
        return users_auth[username]
