import wkpython
import time


def faas_main(handle):
    output = b"Hello, World"
    # time.sleep(30)
    wkpython.core.set_output(handle, output)
    return 0
