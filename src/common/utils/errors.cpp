//
// Created by kingdo on 2022/4/2.
//

#include <wukong/utils/errors.h>
namespace wukong::utils
{
    std::string errors()
    {
        switch (errno)
        {
        case EPERM:
            return "EPERM,Operation not permitted";
        case ENOENT:
            return "ENOENT,No such file or directory";
        case ESRCH:
            return "ESRCH,No such process";
        case EINTR:
            return "EINTR,Interrupted system call";
        case EIO:
            return "EIO,I/O error";
        case ENXIO:
            return "ENXIO,No such device or address";
        case E2BIG:
            return "E2BIG,Argument list too long";
        case ENOEXEC:
            return "ENOEXEC,Exec format error";
        case EBADF:
            return "EBADF,Bad file number";
        case ECHILD:
            return "ECHILD,No child processes";
        case EAGAIN:
            return "EAGAIN,Try again";
        case ENOMEM:
            return "ENOMEM,Out of memory";
        case EACCES:
            return "EACCES,Permission denied";
        case EFAULT:
            return "EFAULT,Bad address";
        case ENOTBLK:
            return "ENOTBLK,Block device required";
        case EBUSY:
            return "EBUSY,Device or resource busy";
        case EEXIST:
            return "EEXIST,File exists";
        case EXDEV:
            return "EXDEV,Cross-device link";
        case ENODEV:
            return "ENODEV,No such device";
        case ENOTDIR:
            return "ENOTDIR,Not a directory";
        case EISDIR:
            return "EISDIR,Is a directory";
        case EINVAL:
            return "EINVAL,Invalid argument";
        case ENFILE:
            return "ENFILE,File table overflow";
        case EMFILE:
            return "EMFILE,Too many open files";
        case ENOTTY:
            return "ENOTTY,Not a typewriter";
        case ETXTBSY:
            return "ETXTBSY,Text file busy";
        case EFBIG:
            return "EFBIG,File too large";
        case ENOSPC:
            return "ENOSPC,No space left on device";
        case ESPIPE:
            return "ESPIPE,Illegal seek";
        case EROFS:
            return "EROFS,Read-only file system";
        case EMLINK:
            return "EMLINK,Too many links";
        case EPIPE:
            return "EPIPE,Broken pipe";
        case EDOM:
            return "EDOM,Math argument out of domain of func";
        case ERANGE:
            return "ERANGE,Math result not representable";
        default:
            WK_CHECK_WITH_ASSERT(false, "errno is out of range");
            throw std::runtime_error("Unreachable");
        }
    }
}