if (DBUS_BUILD_TESTS)

set (NAMETEST_DIR ../../../test/name-test)

#INCLUDES=-I$(top_srcdir) $(DBUS_CLIENT_CFLAGS) $(DBUS_GLIB_CFLAGS) -DDBUS_COMPILATION

include_directories( ${CMAKE_SOURCE_DIR}/.. )
include_directories( ${CMAKE_INCLUDE_PATH} )
add_definitions(-DDBUS_COMPILATION)

#
### note that TESTS has special meaning (stuff to use in make check)
### so if adding tests not to be run in make check, don't add them to 
### TESTS
#if DBUS_BUILD_TESTS
#TESTS_ENVIRONMENT=DBUS_TOP_BUILDDIR=$(ABSOLUTE_TOP_BUILDDIR)
#TESTS=run-test.sh
#else
#TESTS=
#endif
#
#EXTRA_DIST=run-test.sh 
#
#if DBUS_BUILD_TESTS
#
### we use noinst_PROGRAMS not check_PROGRAMS for TESTS so that we
### build even when not doing "make check"
#noinst_PROGRAMS=test-names test-pending-call-dispatch test-threads-init
#
#test_names_SOURCES=				\
#	test-names.c
#
#test_names_LDADD=$(top_builddir)/dbus/libdbus-1.la $(top_builddir)/dbus/libdbus-convenience.la

# there is also a test_names executable, don't know if this is the same
add_executable(test_names1 ${NAMETEST_DIR}/test-names.c)
target_link_libraries(test_names1 dbus-1)
install_targets(/bin test_names1)
ADD_TEST(test_names ${EXECUTABLE_OUTPUT_PATH}/test_names1)
#
#test_pending_call_dispatch_SOURCES =		\
#	test-pending-call-dispatch.c
#
#test_pending_call_dispatch_LDADD=$(top_builddir)/dbus/libdbus-1.la $(top_builddir)/dbus/libdbus-convenience.la

add_executable(test_pending_call_dispatch ${NAMETEST_DIR}/test-pending-call-dispatch.c)
target_link_libraries(test_pending_call_dispatch dbus-1)
install_targets(/bin test_pending_call_dispatch)
ADD_TEST(test_names ${EXECUTABLE_OUTPUT_PATH}/test_pending_call_dispatch)

#
#
#test_threads_init_SOURCES =            \
#	test-threads-init.c
#
#test_threads_init_LDADD=$(top_builddir)/dbus/libdbus-1.la $(top_builddir)/dbus/libdbus-convenience.la
#
add_executable(test_thread_init ${NAMETEST_DIR}/test-threads-init.c)
target_link_libraries(test_thread_init dbus-1)
install_targets(/bin test_thread_init)
ADD_TEST(test_names ${EXECUTABLE_OUTPUT_PATH}/test_thread_init)

#endif
#
endif (DBUS_BUILD_TESTS)
