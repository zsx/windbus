include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckStructMember)
include(CheckTypeSize)

check_include_file(time.h       HAVE_TIME_H)    # dbus-sysdeps-win.c
check_include_file(grp.h        HAVE_GRP_H)     # dbus-sysdeps-util-win.c
check_include_file(sys/poll.h   HAVE_POLL)      # dbus-sysdeps.c, dbus-sysdeps-win.c
check_include_file(sys/time.h   HAVE_SYS_TIME_H)# dbus-sysdeps-win.c
check_include_file(sys/wait.h   HAVE_SYS_WAIT_H)# dbus-sysdeps-win.c
check_include_file(unistd.h     HAVE_UNISTD_H)  # dbus-sysdeps-util-win.c

check_symbol_exists(backtrace    "execinfo.h"       HAVE_BACKTRACE)          #  dbus-sysdeps.c, dbus-sysdeps-win.c
check_symbol_exists(getgrouplist "grp.h"            HAVE_GETGROUPLIST)       #  dbus-sysdeps.c
check_symbol_exists(getpeerucred "ucred.h"          HAVE_GETPEERUCRED)       #  dbus-sysdeps.c, dbus-sysdeps-win.c
check_symbol_exists(nanosleep    "time.h"           HAVE_NANOSLEEP)          #  dbus-sysdeps.c
check_symbol_exists(getpwnam_r   "errno.h pwd.h"    HAVE_POSIX_GETPWNAM_R)   #  dbus-sysdeps-util-unix.c
check_symbol_exists(setenv       "stdlib.h"         HAVE_SETENV)             #  dbus-sysdeps.c
check_symbol_exists(socketpair   "sys/socket.h.h"   HAVE_SOCKETPAIR)         #  dbus-sysdeps.c
check_symbol_exists(unsetenv     "stdlib.h"         HAVE_UNSETENV)           #  dbus-sysdeps.c
check_symbol_exists(writev       "sys/uio.h"        HAVE_WRITEV)             #  dbus-sysdeps.c, dbus-sysdeps-win.c

check_struct_member(cmsgcred cmcred_pid "sys/types.h sys/socket.h" HAVE_CMSGCRED)   #  dbus-sysdeps.c

# missing:
# HAVE_ABSTRACT_SOCKETS
# DBUS_USE_ATOMIC_INT_486 // check for cpu type
# DBUS_HAVE_GCC33_GCOV

check_type_size("short"     SIZEOF_SHORT)
check_type_size("int"       SIZEOF_INT)
check_type_size("long"      SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("__int64"   SIZEOF___INT64)

# DBUS_INT64_TYPE
if(SIZEOF_INT EQUAL 8)
    set (DBUS_HAVE_INT64 1)
    set (DBUS_INT64_TYPE "int")
else(SIZEOF_INT EQUAL 8)
    if(SIZEOF_LONG EQUAL 8)
        set (DBUS_HAVE_INT64 1)
        set (DBUS_INT64_TYPE "long")
    else(SIZEOF_LONG EQUAL 8)
        if(SIZEOF_LONG_LONG EQUAL 8)
            set (DBUS_HAVE_INT64 1)
            set (DBUS_INT64_TYPE "long long")
        else(SIZEOF_LONG_LONG EQUAL 8)
            if(SIZEOF___INT64 EQUAL 8)
                set (DBUS_HAVE_INT64 1)
                set (DBUS_INT64_TYPE "__int64")
            endif(SIZEOF___INT64 EQUAL 8)
        endif(SIZEOF_LONG_LONG EQUAL 8)
    endif(SIZEOF_LONG EQUAL 8)
endif(SIZEOF_INT EQUAL 8)

# DBUS_INT32_TYPE
if(SIZEOF_INT EQUAL 4)
    set (DBUS_INT32_TYPE "int")
else(SIZEOF_INT EQUAL 4)
    if(SIZEOF_LONG EQUAL 4)
        set (DBUS_INT32_TYPE "long")
    else(SIZEOF_LONG EQUAL 4)
        if(SIZEOF_LONG_LONG EQUAL 4)
            set (DBUS_INT32_TYPE "long long")
        endif(SIZEOF_LONG_LONG EQUAL 4)
    endif(SIZEOF_LONG EQUAL 4)
endif(SIZEOF_INT EQUAL 4)

# DBUS_INT16_TYPE
if(SIZEOF_INT EQUAL 2)
    set (DBUS_INT16_TYPE "int")
else(SIZEOF_INT EQUAL 2)
    if(SIZEOF_SHORT EQUAL 2)
        set (DBUS_INT16_TYPE "short")
    endif(SIZEOF_SHORT EQUAL 2)
endif(SIZEOF_INT EQUAL 2)

#DBUS_HAVE_ATOMIC_INT