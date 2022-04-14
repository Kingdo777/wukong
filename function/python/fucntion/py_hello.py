import wkpython


def faas_main(handle):
    print(wkpython.core.get_input_len(handle))
    input_date = wkpython.core.get_input(handle)
    output = b"Hello, " + input_date
    wkpython.core.set_output(handle, output)
