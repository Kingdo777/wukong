import ctypes

_host_interface = ctypes.CDLL(None)


def get_input_len(handle):
    return _host_interface.faas_getInputSize_py(ctypes.c_void_p(handle))


def get_input(handle):
    input_len = int(_host_interface.faas_getInputSize_py(ctypes.c_void_p(handle)))
    buff = ctypes.create_string_buffer(input_len)
    _host_interface.faas_getInput_py(ctypes.c_void_p(handle), buff, input_len)
    return buff.value


def set_output(handle, output):
    _host_interface.faas_setOutput_py(ctypes.c_void_p(handle), output, len(output))
