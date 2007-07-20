

include_directories(${CMAKE_SOURCE_DIR}/..)

add_executable(pkg-config ../../pkg-config/pkg-config.cpp) 
target_link_libraries(pkg-config ${DBUS_1})
install_targets(/bin pkg-config )
