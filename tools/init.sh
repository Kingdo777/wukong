wk user.register
wk app.create
wk func.register -n hello
wk func.register -n toupper
wk func.register -n who
wk func.register -n py_hello -c Python
wk func.register -n py_noop  -c Python
wk func.register -n py_who  -c Python
wk func.register -n py_toupper  -c Python
wk func.funcs

curl 127.0.0.1:8080

# 在没有创建实例之前，不能直接并发，这部分的逻辑还没写