project(dbus)

include_directories(${CMAKE_SOURCE_DIR}/..)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/dbus-arch-deps.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/dbus-arch-deps.h )

#INCLUDES=-I$(top_srcdir) $(DBUS_CLIENT_CFLAGS) -DDBUS_COMPILATION

file(GLOB DBUS_SOURCES ${CMAKE_SOURCE_DIR}/../dbus/*.c)
file(GLOB DBUS_HEADERS ${CMAKE_SOURCE_DIR}/../dbus/*.h)


list(REMOVE_ITEM DBUS_SOURCES
	${CMAKE_SOURCE_DIR}/../dbus/dbus-md5.c
	${CMAKE_SOURCE_DIR}/../dbus/dbus-spawn-win.c
	${CMAKE_SOURCE_DIR}/../dbus/dbus-spawn.c
	${CMAKE_SOURCE_DIR}/../dbus/dbus-test-main.c
	)

set(DBUS_HEADERS ${DBUS_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/dbus-arch-deps.h)

if (WIN32)
	set (DBUS_SPAWN_SOURCE ${CMAKE_SOURCE_DIR}/../dbus/dbus-spawn-win.c)
else (WIN32)
	set (DBUS_SPAWN_SOURCE ${CMAKE_SOURCE_DIR}/../dbus/dbus-spawn.c)
endif (WIN32)



 
if (WIN32)
	set (LIBS ${LIBS} ws2_32 advapi32 netapi32)
	set(CMAKE_DEBUG_POSTFIX "d")
	add_library(dbus-1 STATIC ${DBUS_SOURCES} ${DBUS_SPAWN_SOURCE} ${DBUS_HEADERS} )
else(WIN32)
	add_library(dbus-1 ${DBUS_SOURCES} ${DBUS_SPAWN_SOURCE} ${DBUS_HEADERS} )
endif(WIN32)

target_link_libraries(dbus-1 ${LIBS} )
install_targets(/lib dbus-1 )



install_files(/include/dbus FILES 
	${CMAKE_SOURCE_DIR}/../dbus/dbus.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-address.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-bus.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-connection.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-errors.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-macros.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-memory.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-message.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-pending-call.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-protocol.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-server.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-shared.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-signature.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-threads.h
	${CMAKE_SOURCE_DIR}/../dbus/dbus-types.h
	${CMAKE_CURRENT_BINARY_DIR}/dbus-arch-deps.h
)


#ADD_EXECUTABLE(dbus-example dbus-example.c)
#target_link_libraries(${CMAKE_SOURCE_DIR}/../dbus/dbus-example dbus-1)
#install_targets(/bin dbus-example )


if (DBUS_BUILD_TESTS)
	MESSAGE("TEST")
	set (TESTS_ENVIRONMENT "DBUS_TEST_DATA=${CMAKE_SOURCE_DIR}/test/data DBUS_TEST_HOMEDIR=${CMAKE_BUILD_DIR}/dbus")
	ADD_EXECUTABLE(dbus-test  ${CMAKE_SOURCE_DIR}/../dbus/dbus-test-main.c)
	target_link_libraries(dbus-test dbus-1 ${LIBS})
	add_test(dbus-test ${EXECUTABLE_OUTPUT_PATH}/dbus-test ${CMAKE_SOURCE_DIR}/test/data)
	install_targets(/bin dbus-test )
ENDIF (DBUS_BUILD_TESTS)


set (BUILT_SOURCES ${dbusarchinclude_HEADERS} ${DBUS_GLIB_BUILT_INCLUDES})

#EXTRA_DIST=dbus-arch-deps.h.in make-dbus-glib-error-enum.sh
#CLEANFILES=$(DBUS_GLIB_BUILT_INCLUDES)

## this library is the same as libdbus, but exports all the symbols
## and is only used for static linking within the dbus package.
#noinst_LTLIBRARIES=libdbus-convenience.la

#libdbus_1_la_LIBADD= $(DBUS_CLIENT_LIBS)
## don't export symbols that start with "_" (we use this 
## convention for internal symbols)

if (UNIX)
	set (libdbus_1_la_LDFLAGS -export-symbols-regex "^[^_].*" -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -no-undefined)
ENDIF (UNIX)

## note that TESTS has special meaning (stuff to use in make check)
## so if adding tests not to be run in make check, don't add them to 
## TESTS
#if DBUS_BUILD_TESTS
#TESTS_ENVIRONMENT=DBUS_TEST_DATA=$(top_builddir)/test/data DBUS_TEST_HOMEDIR=$(top_builddir)/dbus
#TESTS=dbus-test
#else
#TESTS=
#endif

## we use noinst_PROGRAMS not check_PROGRAMS so that we build 
## even when not doing "make check"
#noinst_PROGRAMS=$(TESTS)

#dbus_test_SOURCES=
#dbus-test-main.c

#dbus_test_LDADD=libdbus-convenience.la

## mop up the gcov files
#clean-local:
#/bin/rm *.bb *.bbg *.da *.gcov .libs/*.da .libs/*.bbg || true
