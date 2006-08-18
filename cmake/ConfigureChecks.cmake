include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)

check_include_file(grp.h        HAVE_GRP_H)     # dbus-sysdeos-util-win.c
check_include_file(sys/poll.h   HAVE_POLL)      # dbus-sysdeps.c
check_include_file(unistd.h     HAVE_UNISTD_H)  # dbus-sysdeos-util-win.c

set(CMAKE_REQUIRED_INCLUDES sys/uio.h)
check_function_exists(writev   HAVE_WRITEV)                 #  dbus-sysdeps.c

set(CMAKE_REQUIRED_INCLUDES errno.h pwd.h)
check_function_exists(getpwnam_r   HAVE_POSIX_GETPWNAM_R)   #  dbus-sysdeps-util-unix.c

check_type_size(short     SIZEOF_SHORT)
check_type_size(int       SIZEOF_INT)
check_type_size(long      SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size(__int64   SIZEOF___INT64)

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