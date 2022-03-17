from os.path import dirname, exists, realpath, join, expanduser

from os import environ

HOME_DIR = expanduser("~")

CONFIG_FILE = join(HOME_DIR, ".wk", "config")

PROJ_ROOT = environ.get("WUKONG_PROJ_ROOT", "{}/CLionProjects/wukong".format(HOME_DIR))

_BUILD_DIR = environ.get("WUKONG_BUILD_DIR", "cmake-build-debug/out")
WUKONG_BUILD_DIR = join(PROJ_ROOT, _BUILD_DIR)
