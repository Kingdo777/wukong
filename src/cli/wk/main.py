from invoke import Collection, Program
from wk import invoker
from wk import func
from wk import user
from wk import app

ns = Collection(invoker, func, user, app)

program = Program(namespace=ns, version='0.1.0')
