rm -rf /tmp/wukong
wk func.delete -f hello
wk func.delete -f who
wk func.delete -f toupper
wk func.delete -f py_hello
wk func.delete -f py_noop
wk func.delete -f py_who
wk func.delete -f py_toupper
wk func.register -n hello --concurrency 2
wk func.register -n toupper --concurrency 5
wk func.register -n who --concurrency 10
wk func.register -n py_hello -c Python
wk func.register -n py_noop  -c Python --concurrency 2
wk func.register -n py_who  -c Python
wk func.register -n py_toupper  -c Python
wk func.funcs
