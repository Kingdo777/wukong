import ctypes

_host_interface = ctypes.CDLL(None)

# 返回值会自动的转化为python支持的类型，但是必须是要指定
# 如C语言函数func()返回的是 uint64，如果在py中不指定起返回值，那么将会默认返回的是ctypes.c_int，这样就会导致数据失真
# 因此需要将其返回值指定为ctypes.c_int，但是这里并不是说起实际返回的类型是ctypes.c_uint64，而是会将其转为python对应的类型
# 如c_size_t，c_int，c_int，c_void_p等都会返回python的int； c_char_p 会返回字节串；


# 返回值是size_t
faas_uuid_size = _host_interface.faas_uuid_size_py
faas_uuid_size.restype = ctypes.c_size_t
faas_uuid_size.argtypes = []

faas_result_size = _host_interface.faas_result_size_py
faas_result_size.restype = ctypes.c_size_t
faas_result_size.argtypes = []

faas_shm_size = _host_interface.faas_shm_size_py
faas_shm_size.restype = ctypes.c_size_t
faas_shm_size.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

faas_getInputSize = _host_interface.faas_getInputSize_py
faas_getInputSize.restype = ctypes.c_size_t
faas_getInputSize.argtypes = [ctypes.c_void_p]

faas_read_shm = _host_interface.faas_read_shm_py
faas_read_shm.restype = ctypes.c_size_t
faas_read_shm.argtypes = [ctypes.c_void_p]

faas_write_shm = _host_interface.faas_write_shm_py
faas_write_shm.restype = ctypes.c_size_t
faas_write_shm.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t]

# uint64
faas_chain_call = _host_interface.faas_chain_call_py
faas_chain_call.restype = ctypes.c_uint64
faas_chain_call.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_size_t]

# 返回值是 void*
faas_create_shm = _host_interface.faas_create_shm_py
faas_create_shm.restype = ctypes.c_void_p
faas_create_shm.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_size_t]

# 返回值是默认的int
faas_getInput = _host_interface.faas_getInput_py
faas_getInput.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]

faas_setOutput = _host_interface.faas_setOutput_py
faas_setOutput.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]

faas_get_call_result = _host_interface.faas_get_call_result_py
faas_get_call_result.argtypes = [ctypes.c_void_p, ctypes.c_uint64, ctypes.c_char_p, ctypes.c_size_t]

faas_delete_shm = _host_interface.faas_delete_shm_py
faas_delete_shm.argtypes = [ctypes.c_void_p, ctypes.c_char_p]


def get_input_len(handle: int) -> int:
    return faas_getInputSize(ctypes.c_void_p(handle))


def get_input(handle: int) -> bytes:
    input_len = faas_getInputSize(ctypes.c_void_p(handle))
    buff = ctypes.create_string_buffer(input_len)
    faas_getInput(ctypes.c_void_p(handle), buff, ctypes.c_size_t(input_len))
    return buff.raw


def set_output(handle: int, output: bytes) -> int:
    return faas_setOutput(ctypes.c_void_p(handle), output, ctypes.c_size_t(len(output)))


def chain_call(handle: int, funcname: str, input_data: bytes) -> int:
    faas_chain_call.restype = ctypes.c_uint64
    return faas_chain_call(ctypes.c_void_p(handle),
                           funcname.encode('utf-8'),
                           input_data, ctypes.c_size_t(len(input_data)))


def get_call_result(handle: int, request_id: int) -> bytes:
    result_size = faas_result_size()
    result = ctypes.create_string_buffer(result_size)
    faas_get_call_result(ctypes.c_void_p(handle),
                         ctypes.c_uint64(request_id),
                         result, ctypes.c_size_t(result_size))
    return result.value


def create_state(handle: int, length: int) -> bytes:
    uuid_size = faas_uuid_size()
    uuid = ctypes.create_string_buffer(uuid_size)
    faas_create_shm(ctypes.c_void_p(handle),
                    ctypes.c_size_t(length),
                    uuid, ctypes.c_size_t(uuid_size))
    return uuid.value


def read_state(handle: int, uuid: bytes, offset: int = 0, length: int = 0) -> (bytes, int):
    if length == 0:
        length = faas_shm_size(ctypes.c_void_p(handle), uuid)
    read_buffer = ctypes.create_string_buffer(length)
    read_size = faas_read_shm(ctypes.c_void_p(handle), uuid,
                              ctypes.c_size_t(offset),
                              read_buffer, ctypes.c_size_t(length))
    return read_buffer.raw, read_size


def write_state(handle: int, uuid: bytes, data: bytes, offset=0) -> int:
    write_size = faas_write_shm(ctypes.c_void_p(handle), uuid,
                                offset,
                                data,
                                len(data))
    return write_size


def delete_state(handle: int, uuid: bytes) -> int:
    return faas_delete_shm(ctypes.c_void_p(handle), uuid)
