from invoke import Collection, Program
from wk import invoker

ns = Collection.from_module(
    invoker
)

program = Program(namespace=ns, version='0.1.0')
