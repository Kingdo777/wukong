//
// Created by kingdo on 2022/4/2.
//

#include <wukong/utils/errors.h>
namespace wukong::utils
{
    std::string errors(const std::string& op)
    {
        switch (errno)
        {
        case EPERM:
            return op + ": EPERM,Operation not permitted";
        case ENOENT:
            return op + ": ENOENT,No such file or directory";
        case ESRCH:
            return op + ": ESRCH,No such process";
        case EINTR:
            return op + ": EINTR,Interrupted system call";
        case EIO:
            return op + ": EIO,I/O error";
        case ENXIO:
            return op + ": ENXIO,No such device or address";
        case E2BIG:
            return op + ": E2BIG,Argument list too long";
        case ENOEXEC:
            return op + ": ENOEXEC,Exec format error";
        case EBADF:
            return op + ": EBADF,Bad file number";
        case ECHILD:
            return op + ": ECHILD,No child processes";
        case EAGAIN:
            return op + ": EAGAIN,Try again";
        case ENOMEM:
            return op + ": ENOMEM,Out of memory";
        case EACCES:
            return op + ": EACCES,Permission denied";
        case EFAULT:
            return op + ": EFAULT,Bad address";
        case ENOTBLK:
            return op + ": ENOTBLK,Block device required";
        case EBUSY:
            return op + ": EBUSY,Device or resource busy";
        case EEXIST:
            return op + ": EEXIST,File exists";
        case EXDEV:
            return op + ": EXDEV,Cross-device link";
        case ENODEV:
            return op + ": ENODEV,No such device";
        case ENOTDIR:
            return op + ": ENOTDIR,Not a directory";
        case EISDIR:
            return op + ": EISDIR,Is a directory";
        case EINVAL:
            return op + ": EINVAL,Invalid argument";
        case ENFILE:
            return op + ": ENFILE,File table overflow";
        case EMFILE:
            return op + ": EMFILE,Too many open files";
        case ENOTTY:
            return op + ": ENOTTY,Not a typewriter";
        case ETXTBSY:
            return op + ": ETXTBSY,Text file busy";
        case EFBIG:
            return op + ": EFBIG,File too large";
        case ENOSPC:
            return op + ": ENOSPC,No space left on device";
        case ESPIPE:
            return op + ": ESPIPE,Illegal seek";
        case EROFS:
            return op + ": EROFS,Read-only file system";
        case EMLINK:
            return op + ": EMLINK,Too many links";
        case EPIPE:
            return op + ": EPIPE,Broken pipe";
        case EDOM:
            return op + ": EDOM,Math argument out of domain of func";
        case ERANGE:
            return op + ": ERANGE,Math result not representable";
        default:
            return "errno is out of range";
            perror("");
        }
    }
}