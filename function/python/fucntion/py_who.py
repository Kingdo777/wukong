import wkpython


def faas_main(handle):
    wkpython.core.set_output(handle, b"kingdo")
