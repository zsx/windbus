if (DBUS_BUILD_TESTS)

set (NAMETEST_DIR ../../../test/name-test)

include_directories( ${CMAKE_SOURCE_DIR}/.. )
include_directories( ${CMAKE_INCLUDE_PATH} )
add_definitions(-DDBUS_COMPILATION)

# there is also a test_names executable, don't know if this is the same
add_executable(test_names1 ${NAMETEST_DIR}/test-names.c)
target_link_libraries(test_names1 dbus-1)
install_targets(/bin test_names1)
ADD_TEST(test_names ${EXECUTABLE_OUTPUT_PATH}/test_names1)

add_executable(test_pending_call_dispatch ${NAMETEST_DIR}/test-pending-call-dispatch.c)
target_link_libraries(test_pending_call_dispatch dbus-1)
install_targets(/bin test_pending_call_dispatch)
ADD_TEST(test_pending_call_dispatch ${EXECUTABLE_OUTPUT_PATH}/test_pending_call_dispatch)

add_executable(test_thread_init ${NAMETEST_DIR}/test-threads-init.c)
target_link_libraries(test_thread_init dbus-1)
install_targets(/bin test_thread_init)
ADD_TEST(test_names ${EXECUTABLE_OUTPUT_PATH}/test_thread_init)

endif (DBUS_BUILD_TESTS)
