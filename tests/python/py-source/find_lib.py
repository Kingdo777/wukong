import ctypes

lib = ctypes.CDLL(None)


def faas_main():
    print(lib.faas_getInputSize_py)
    print(lib.faas_getInput_py)
    print(lib.faas_setOutput_py)
    return 0
