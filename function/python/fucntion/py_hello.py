import json

import wkpython


def faas_main(handle):
    input_date = wkpython.core.get_input(handle)
    print("input_data:{}".format(input_date))
    request_id = wkpython.core.chain_call(handle, "py_who", b"")
    print("request_id for call who:{}".format(request_id))
    result = wkpython.core.get_call_result(handle, request_id)
    print("who result:{}".format(result))
    uuid = wkpython.core.create_state(handle, len(result))
    wkpython.core.write_state(handle, uuid, result)
    toupper_input = {
        "uuid": uuid.decode("utf-8"),
        "length": len(result)
    }
    print(toupper_input)
    request_id = wkpython.core.chain_call(handle, "py_toupper", json.dumps(toupper_input).encode("utf-8"))
    print("request_id for call tupper:{}".format(request_id))
    result = wkpython.core.get_call_result(handle, request_id)
    print("tupper result:{}".format(result))
    output = b"Hello, " + result + b". I am " + input_date
    wkpython.core.set_output(handle, output)
    print("output_data:{}".format(output))

# import ctypes
#
# if __name__ == '__main__':
#     lib = ctypes.CDLL("libc.so.6")
#     printf = lib.printf
#     printf.restype = ctypes.c_char
#     ret = printf(b"123\n")
#     print(type(ret))
#     print(ret.value)
# print(type(ctypes.c_int64))
