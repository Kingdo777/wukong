import json
import requests
from os.path import exists

from wk.utils.env import CONFIG_FILE


def ping(host, port):
    url = "http://{}:{}/ping".format(host, port)
    try:
        response = requests.get(url)
    except requests.exceptions.RequestException:
        return False
    else:
        if response.status_code == 200 and response.text == "PONG":
            return True
        else:
            return False


def test_connect():
    if exists(CONFIG_FILE):
        f = open(CONFIG_FILE, "r")
        try:
            conf = json.loads(f.read())
        except json.JSONDecodeError:
            return False, "config is illegal"
        else:
            host = conf["GW_HOST"]
            port = conf["GW_PORT"]
            if ping(host, port):
                return True, "testing connect to {}:{} Success".format(host, port), host, port
            else:
                return False, "testing connect to {}:{} Failed".format(host, port)
    else:
        return False, "config is not exists"
