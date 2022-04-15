import json

import wkpython


def faas_main(handle):
    input_date = wkpython.core.get_input(handle)
    print(input_date)
    uuid = json.loads(input_date)['uuid'].encode('utf-8')
    print(uuid)
    result = wkpython.core.read_state(handle, uuid)
    print(result)
    wkpython.core.delete_state(handle, uuid)

    wkpython.core.set_output(handle, result[0].decode('utf-8').upper().encode('utf-8'))
