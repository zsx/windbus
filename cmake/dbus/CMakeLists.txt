project(dbus)

#INCLUDES=-I$(top_srcdir) $(DBUS_CLIENT_CFLAGS) -DDBUS_COMPILATION

include_directories(${CMAKE_SOURCE_DIR}/..)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dbus-arch-deps.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/dbus-arch-deps.h )

add_definitions(-DDBUS_COMPILATION)

set (dbusinclude_HEADERS
	../../dbus/dbus.h
	../../dbus/dbus-address.h
	../../dbus/dbus-bus.h
	../../dbus/dbus-connection.h
	../../dbus/dbus-errors.h
	../../dbus/dbus-macros.h
	../../dbus/dbus-memory.h
	../../dbus/dbus-message.h
	../../dbus/dbus-pending-call.h
	../../dbus/dbus-protocol.h
	../../dbus/dbus-server.h
	../../dbus/dbus-shared.h
	../../dbus/dbus-signature.h
	../../dbus/dbus-threads.h
	../../dbus/dbus-types.h
	dbus-arch-deps.h
)

### source code that goes in the installed client library
### and is specific to library functionality
set (DBUS_LIB_SOURCES
	../../dbus/dbus-address.c
	../../dbus/dbus-auth.c
	../../dbus/dbus-auth-script.c
	../../dbus/dbus-bus.c
	../../dbus/dbus-connection.c
	../../dbus/dbus-errors.c
	../../dbus/dbus-keyring.c
	../../dbus/dbus-marshal-header.c
	../../dbus/dbus-marshal-byteswap.c
	../../dbus/dbus-marshal-recursive.c
	../../dbus/dbus-marshal-validate.c
	../../dbus/dbus-message.c
	../../dbus/dbus-object-tree.c
	../../dbus/dbus-pending-call.c
	../../dbus/dbus-resources.c
	../../dbus/dbus-server.c
	../../dbus/dbus-server-debug-pipe.c
	../../dbus/dbus-server-unix.c
	../../dbus/dbus-sha.c
	../../dbus/dbus-signature.c
	../../dbus/dbus-timeout.c
	../../dbus/dbus-threads.c
	../../dbus/dbus-transport.c
	../../dbus/dbus-transport-unix.c
	../../dbus/dbus-watch.c
##dbus-md5.c
)
set (DBUS_LIB_HEADERS
	../../dbus/dbus-auth.h
	../../dbus/dbus-auth-script.h
	../../dbus/dbus-connection-internal.h
	../../dbus/dbus-keyring.h
	../../dbus/dbus-marshal-header.h
	../../dbus/dbus-marshal-byteswap.h
	../../dbus/dbus-marshal-recursive.h
	../../dbus/dbus-marshal-validate.h
	../../dbus/dbus-message-internal.h
	../../dbus/dbus-message-private.h
	../../dbus/dbus-object-tree.h
	../../dbus/dbus-resources.h
	../../dbus/dbus-server-debug-pipe.h
	../../dbus/dbus-server-protected.h
	../../dbus/dbus-server-unix.h
	../../dbus/dbus-sha.h
	../../dbus/dbus-timeout.h
	../../dbus/dbus-threads-internal.h
	../../dbus/dbus-transport.h
	../../dbus/dbus-transport-protected.h
	../../dbus/dbus-transport-unix.h
	../../dbus/dbus-watch.h
##dbus-md5.h
)
### source code that goes in the installed client library
### AND is generic utility functionality used by the 
### daemon or test programs (all symbols in here should 
### be underscore-prefixed)
set (DBUS_SHARED_SOURCES
	../../dbus/dbus-dataslot.c
	../../dbus/dbus-hash.c
	../../dbus/dbus-internals.c
	../../dbus/dbus-list.c
	../../dbus/dbus-marshal-basic.c
	../../dbus/dbus-memory.c
	../../dbus/dbus-mempool.c
	../../dbus/dbus-string.c
	../../dbus/dbus-sysdeps.c
	../../dbus/dbus-sysdeps-util.c
	../../dbus/dbus-userdb.c
)

set (DBUS_SHARED_HEADERS
	../../dbus/dbus-dataslot.h
	../../dbus/dbus-hash.h
	../../dbus/dbus-internals.h
	../../dbus/dbus-list.h
	../../dbus/dbus-marshal-basic.h
	../../dbus/dbus-mempool.h
	../../dbus/dbus-string.h
	../../dbus/dbus-string-private.h
	../../dbus/dbus-sysdeps.h
	../../dbus/dbus-userdb.h
)

### source code that is generic utility functionality used
### by the bus daemon or test apps, but is NOT included
### in the D-BUS client library (all symbols in here 
### should be underscore-prefixed but don't really need 
### to be unless they move to DBUS_SHARED_SOURCES later)
set (DBUS_UTIL_SOURCES
	../../dbus/dbus-auth-util.c
	../../dbus/dbus-mainloop.c
	../../dbus/dbus-marshal-byteswap-util.c
	../../dbus/dbus-marshal-recursive-util.c
	../../dbus/dbus-marshal-validate-util.c
	../../dbus/dbus-message-factory.c
	../../dbus/dbus-message-util.c
	../../dbus/dbus-shell.c
	../../dbus/dbus-string-util.c
	../../dbus/dbus-userdb-util.c
)

if (DBUS_BUILD_TESTS)
	set (DBUS_UTIL_SOURCES 
		${DBUS_UTIL_SOURCES}
		../../dbus/dbus-test.c
	)
endif (DBUS_BUILD_TESTS)

set (DBUS_UTIL_HEADERS
	../../dbus/dbus-mainloop.h
	../../dbus/dbus-message-factory.h
	../../dbus/dbus-shell.h
	../../dbus/dbus-spawn.h
	../../dbus/dbus-test.h
)

### platform specific settings
if (WIN32)
	set (DBUS_SHARED_SOURCES ${DBUS_SHARED_SOURCES} 
#		../../dbus/dbus-dirent-win.c
		../../dbus/dbus-sysdeps-win.c
#		../../dbus/dbus-sysdeps-util-win.c
		../../dbus/dbus-spawn-win.c
	)
	set (LIBS ${LIBS} ws2_32 advapi32 netapi32)

else (WIN32)
	set (DBUS_SHARED_SOURCES ${DBUS_SHARED_SOURCES} 
		../../dbus/dbus-dirent.c
		../../dbus/dbus-spawn.c
	)
endif (WIN32)

set(libdbus_SOURCES
	${DBUS_LIB_SOURCES}
	${DBUS_SHARED_SOURCES}
	# for debugging
	${DBUS_UTIL_SOURCES}
)

set(libdbus_HEADERS 
	${DBUS_LIB_HEADERS}
	${DBUS_SHARED_HEADERS}
	# for debugging
	${DBUS_UTIL_HEADERS}
)

if(MSVC)
	set(CMAKE_DEBUG_POSTFIX "d")
	add_library(dbus-1 STATIC ${libdbus_SOURCES} ${libdbus_HEADERS} )
else(MSVC)
	add_library(dbus-1 ${libdbus_SOURCES} ${libdbus_HEADERS} )
endif(MSVC)

target_link_libraries(dbus-1 ${LIBS} )

install_targets(/lib dbus-1 )
install_files(/include/dbus FILES ${dbusinclude_HEADERS})

#ADD_EXECUTABLE(dbus-example dbus-example.c)
#target_link_libraries(${CMAKE_SOURCE_DIR}/../dbus/dbus-example dbus-1)
#install_targets(/bin dbus-example )

## note that TESTS has special meaning (stuff to use in make check)
## so if adding tests not to be run in make check, don't add them to 
## TESTS
#if DBUS_BUILD_TESTS
#TESTS_ENVIRONMENT=DBUS_TEST_DATA=$(top_builddir)/test/data DBUS_TEST_HOMEDIR=$(top_builddir)/dbus
#TESTS=dbus-test
#else
#TESTS=
#endif

if (DBUS_BUILD_TESTS)
	set (TESTS_ENVIRONMENT "DBUS_TEST_DATA=${CMAKE_SOURCE_DIR}/test/data DBUS_TEST_HOMEDIR=${CMAKE_BUILD_DIR}/dbus")
	ADD_EXECUTABLE(dbus-test  ${CMAKE_SOURCE_DIR}/../dbus/dbus-test-main.c)
	target_link_libraries(dbus-test dbus-1 ${LIBS})
	add_test(dbus-test ${EXECUTABLE_OUTPUT_PATH}/dbus-test ${CMAKE_SOURCE_DIR}/../test/data)
	install_targets(/bin dbus-test)
ENDIF (DBUS_BUILD_TESTS)

if (UNIX)
# set version info
ENDIF (UNIX)


## mop up the gcov files
#clean-local:
#/bin/rm *.bb *.bbg *.da *.gcov .libs/*.da .libs/*.bbg || true
