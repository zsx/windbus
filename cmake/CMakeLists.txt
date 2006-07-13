set (PACKAGE dbus)
set (VERSION 0.62)
project(${PACKAGE})

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/modules")

#enable building of shared library
SET(BUILD_SHARED_LIBS ON)

# search packages used by KDE
if (WIN32)
	find_package(GNUWIN32)
endif (WIN32)
find_package(LIBXML2 REQUIRED)
find_package(QT4)

# make some more macros available
include (MacroLibrary)

if(MSVC)

	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Zi   -wd4996 -wd4800" CACHE STRING "runtime-library flags" FORCE)
	SET(CMAKE_CXX_FLAGS_RELEASE  "${CMAKE_CXX_FLAGS_RELEASE}  -wd4996 -wd4800" CACHE STRING "runtime-library flags" FORCE)
	SET(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
	SET(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else(MSVC)
endif(MSVC)


#########################################################################
# Disallow in-source build
#macro_ensure_out_of_source_build("dbus requires an out of source build. Please create a separate build directory and run 'cmake path_to_dbus [options]' there.")

# ... and warn in case of an earlier in-source build
#set(generatedFileInSourceDir EXISTS ${dbus_SOURCE_DIR}/config.h)
#if(${generatedFileInSourceDir})
#   message(STATUS "config.h exists in your source directory.")
#endif(${generatedFileInSourceDir})
#########################################################################

if (WIN32)
	set (LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/bin)
else (WIN32)
	set (LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/lib)
endif (WIN32)

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# for including config.h and for includes like <dir/foo.h>
include_directories( ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_INCLUDE_PATH} )

# linker search directories
link_directories(${DBUS_LIB_DIR} ${LIBRARY_OUTPUT_PATH} )
include_directories( ${CMAKE_LIBRARY_PATH}  )

set(DBUS_INCLUDES)

ENABLE_TESTING()

########### basic vars ###############

if (WIN32)
	set(prefix                   ${CMAKE_INSTALL_PREFIX})
	set(EXPANDED_LIBDIR          ${CMAKE_INSTALL_PREFIX}/lib)
	set(EXPANDED_INCLUDEDIR      ${CMAKE_INSTALL_PREFIX}/include)
	set(EXPANDED_BINDIR          ${CMAKE_INSTALL_PREFIX}/bin)
	set(EXPANDED_SYSCONFDIR      ${CMAKE_INSTALL_PREFIX}/etc)
	set(EXPANDED_DATADIR         ${CMAKE_INSTALL_PREFIX}/data)
else (WIN32)
endif (WIN32)

if (WIN32)
	set (DBUS_HAVE_INT64 1)
	set (DBUS_INT64_TYPE "long long int")
	set (DBUS_INT32_TYPE int)
	set (DBUS_INT16_TYPE short)
else (WIN32)
	set (DBUS_HAVE_INT64 1)
	set (DBUS_INT64_TYPE "long long int")
	set (DBUS_INT32_TYPE int)
	set (DBUS_INT16_TYPE short)
endif (WIN32)

########### command line options ###############
# TODO: take check from configure.in 

#AC_ARG_ENABLE(qt, AS_HELP_STRING([--enable-qt],[enable Qt-friendly client library]),enable_qt=$enableval,enable_qt=auto)
#AC_ARG_ENABLE(qt-debug, AS_HELP_STRING([--enable-qt-debug],[enable Qt-friendly client library, linked to debug Qt libraries]),enable_qt_debug=$enableval,enable_qt_debug=no)
#AC_ARG_WITH(qt_moc, AS_HELP_STRING([--with-qt-moc=<path>],[moc for Qt]))
#AC_ARG_ENABLE(qt3, AS_HELP_STRING([--enable-qt3],[enable Qt3-friendly client library]),enable_qt3=$enableval,enable_qt3=auto)
#AC_ARG_WITH(qt3_moc, AS_HELP_STRING([--with-qt3-moc=<path>],[moc for Qt3]))
#AC_ARG_ENABLE(glib, AS_HELP_STRING([--enable-glib],[enable GLib-friendly client library]),enable_glib=$enableval,enable_glib=auto)
#AC_ARG_ENABLE(gtk, AS_HELP_STRING([--enable-gtk],[enable GTK-requiring executables]),enable_gtk=$enableval,enable_gtk=auto)
#AC_ARG_ENABLE(tests, AS_HELP_STRING([--enable-tests],[enable unit test code]),enable_tests=$enableval,enable_tests=$USE_MAINTAINER_MODE)

set (DBUS_BUILD_TESTS 1)

if (DBUS_BUILD_TESTS)
	add_definitions(-DDBUS_BUILD_TESTS=1)
	if(NOT MSVC AND NOT CMAKE_BUILD_TYPE MATCHES Release)
		add_definitions(-g)
	endif(NOT MSVC AND NOT CMAKE_BUILD_TYPE MATCHES Release)
endif (DBUS_BUILD_TESTS)

#AC_ARG_ENABLE(ansi, AS_HELP_STRING([--enable-ansi],[enable -ansi -pedantic gcc flags]),enable_ansi=$enableval,enable_ansi=no)
#AC_ARG_ENABLE(verbose-mode, AS_HELP_STRING([--enable-verbose-mode],[support verbose debug mode]),enable_verbose_mode=$enableval,enable_verbose_mode=$USE_MAINTAINER_MODE)

set (DBUS_ENABLE_VERBOSE_MODE 1)

if (DBUS_ENABLE_VERBOSE_MODE)
	add_definitions(-DDBUS_ENABLE_VERBOSE_MODE=1)
endif (DBUS_ENABLE_VERBOSE_MODE)

#AC_ARG_ENABLE(asserts, AS_HELP_STRING([--enable-asserts],[include assertion checks]),enable_asserts=$enableval,enable_asserts=$USE_MAINTAINER_MODE)
#AC_ARG_ENABLE(checks, AS_HELP_STRING([--enable-checks],[include sanity checks on public API]),enable_checks=$enableval,enable_checks=yes)
#AC_ARG_ENABLE(xml-docs, AS_HELP_STRING([--enable-xml-docs],[build XML documentation (requires xmlto)]),enable_xml_docs=$enableval,enable_xml_docs=auto)
#AC_ARG_ENABLE(doxygen-docs, AS_HELP_STRING([--enable-doxygen-docs],[build DOXYGEN documentation (requires Doxygen)]),enable_doxygen_docs=$enableval,enable_doxygen_docs=auto)
#AC_ARG_ENABLE(gcov, AS_HELP_STRING([--enable-gcov],[compile with coverage profiling instrumentation (gcc only)]),enable_gcov=$enableval,enable_gcov=no)
#AC_ARG_ENABLE(abstract-sockets, AS_HELP_STRING([--enable-abstract-sockets],[use abstract socket namespace (linux only)]),enable_abstract_sockets=$enableval,enable_abstract_sockets=auto)
#AC_ARG_ENABLE(gcj, AS_HELP_STRING([--enable-gcj],[build gcj bindings]),enable_gcj=$enableval,enable_gcj=no)
#AC_ARG_ENABLE(mono, AS_HELP_STRING([--enable-mono],[build mono bindings]),enable_mono=$enableval,enable_mono=no)
#AC_ARG_ENABLE(mono_docs, AS_HELP_STRING([--enable-mono-docs],[build mono docs]),enable_mono_docs=$enableval,enable_mono_docs=no)
#AC_ARG_ENABLE(python, AS_HELP_STRING([--enable-python],[build python bindings]),enable_python=$enableval,enable_python=auto)
#AC_ARG_ENABLE(selinux, AS_HELP_STRING([--enable-selinux],[build with SELinux support]),enable_selinux=$enableval,enable_selinux=auto)
#AC_ARG_ENABLE(dnotify, AS_HELP_STRING([--enable-dnotify],[build with dnotify support (linux only)]),enable_dnotify=$enableval,enable_dnotify=auto)
#
#AC_ARG_WITH(xml, AS_HELP_STRING([--with-xml=[libxml/expat]],[XML library to use]))
#AC_ARG_WITH(init-scripts, AS_HELP_STRING([--with-init-scripts=[redhat]],[Style of init scripts to install]))
#AC_ARG_WITH(session-socket-dir, AS_HELP_STRING([--with-session-socket-dir=[dirname]],[Where to put sockets for the per-login-session message bus]))

#### Find socket directories
#if ! test -z "$TMPDIR" ; then
#   DEFAULT_SOCKET_DIR=$TMPDIR
#elif ! test -z "$TEMP" ; then
#   DEFAULT_SOCKET_DIR=$TEMP
#elif ! test -z "$TMP" ; then
#   DEFAULT_SOCKET_DIR=$TMP
#else
#   DEFAULT_SOCKET_DIR=/tmp
#fi
set (DBUS_SESSION_SOCKET_DIR /tmp)

#AC_ARG_WITH(test-socket-dir, AS_HELP_STRING([--with-test-socket-dir=[dirname]],[Where to put sockets for make check]))

#AC_ARG_WITH(system-pid-file, AS_HELP_STRING([--with-system-pid-file=[pidfile]],[PID file for systemwide daemon]))

#if ! test -z "$with_system_pid_file"; then
#   DBUS_SYSTEM_PID_FILE=$with_system_pid_file
#elif test x$operating_system = xredhat ; then
#   DBUS_SYSTEM_PID_FILE=${EXPANDED_LOCALSTATEDIR}/run/messagebus.pid
#else
#   DBUS_SYSTEM_PID_FILE=${EXPANDED_LOCALSTATEDIR}/run/dbus/pid
#fi
# TODO: fix redhet 
if (WIN32)
	set (DBUS_SYSTEM_PID_FILE "")
else (WIN32)
	set (DBUS_SYSTEM_PID_FILE ${EXPANDED_LOCALSTATEDIR}/run/dbus/pid)
endif (WIN32)

#AC_ARG_WITH(system-socket, AS_HELP_STRING([--with-system-socket=[filename]],[UNIX domain socket for systemwide daemon]))

#AC_ARG_WITH(console-auth-dir, AS_HELP_STRING([--with-console-auth-dir=[dirname]],[directory to check for console ownerhip]))

if (WIN32)
	set (DBUS_CONSOLE_AUTH_DIR "")
else (WIN32)
	set (DBUS_CONSOLE_AUTH_DIR "/var/run/console/")
endif (WIN32)

#AC_ARG_WITH(dbus_user, AS_HELP_STRING([--with-dbus-user=<user>],[User for running the DBUS daemon (messagebus)]))

set (DBUS_USER )

#    AC_DEFINE(DBUS_DISABLE_ASSERT,1,[Disable assertion checking])
#    AC_DEFINE(G_DISABLE_ASSERT,1,[Disable GLib assertion macros])
#if test x$enable_checks = xno; then
#    AC_DEFINE(DBUS_DISABLE_CHECKS,1,[Disable public API sanity checking])
#    AC_DEFINE(G_DISABLE_CHECKS,1,[Disable GLib public API sanity checking])


if (WIN32)
  set (DBUS_SESSION_BUS_DEFAULT_ADDRESS "tcp:host=localhost,port=12434")
  set (DBUS_SYSTEM_BUS_DEFAULT_ADDRESS "tcp:host=localhost,port=12434")
  set (DBUS_SERVICE_DIR "${EXPANDED_DATADIR}/dbus-1/services")
  set (DBUS_SYSTEM_PID_FILE "")
  set (DBUS_SYSTEM_CONFIG_FILE "${EXPANDED_SYSCONFDIR}/system.conf")
  set (DBUS_SESSION_CONFIG_FILE "${EXPANDED_SYSCONFDIR}/session.conf")
else (WIN32)
  set (DBUS_SYSTEM_BUS_DEFAULT_ADDRESS unix:tmpdir=)
  set (DBUS_SESSION_BUS_DEFAULT_ADDRESS unix:path=${DBUS_SESSION_SOCKET_DIR})
  set (DBUS_SERVICE_DIR ${EXPANDED_DATADIR}/dbus-1/services)
  set (DBUS_SYSTEM_PID_FILE "")
  set (sysconfdir "")
  set (configdir ${sysconfdir}/dbus-1 )
  set (DBUS_SYSTEM_CONFIG_FILE  ${configdir}/system.conf)
  set (DBUS_SESSION_CONFIG_FILE ${configdir}/session.conf)
endif (WIN32)

set (DAEMON_NAME dbus-daemon)  

########### create config.h ###############

#include(ConfigureChecks.cmake)

# better use flags for gcc 
if (MINGW)
	set (HAVE_GNUC_VARARGS 1)
endif(MINGW)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

# compiler definitions 
add_definitions(-DHAVE_CONFIG_H=1)
add_definitions(${DBUS_BUS_CFLAGS} -DDBUS_API_SUBJECT_TO_CHANGE -DDBUS_COMPILATION )


########### subdirs ###############

add_subdirectory( dbus )
add_subdirectory( bus )
add_subdirectory( test )
#add_subdirectory( tools )
# is now located in kde svn
#add_subdirectory( qt )


message("                  D-BUS ${VERSION}                                    ")
message("                  ==============                                      ")
message("                                                                      ")
message("        prefix:                   ${prefix}                           ")
message("        exec_prefix:              ${exec_prefix}                      ")
message("        libdir:                   ${EXPANDED_LIBDIR}                  ")
message("        bindir:                   ${EXPANDED_BINDIR}                  ")
message("        sysconfdir:               ${EXPANDED_SYSCONFDIR}              ")
message("        localstatedir:            ${EXPANDED_LOCALSTATEDIR}           ")
message("        datadir:                  ${EXPANDED_DATADIR}                 ")
message("        source code location:	   ${srcdir}                           ")
message("        compiler:            		 ${CC}                               ")       
message("        cflags:	                 ${CFLAGS}                           ") 
message("        cppflags:             	   ${CPPFLAGS}                         ")       
message("        cxxflags:            		 ${CXXFLAGS}                         ")       
message("        64-bit int:		           ${DBUS_INT64_TYPE}                  ")     
message("        32-bit int:		           ${DBUS_INT32_TYPE}                  ")     
message("        16-bit int:		           ${DBUS_INT16_TYPE}                  ")     
message("        Doxygen:                  ${DOXYGEN}                          ")
message("        xmlto:                    ${XMLTO}                            ")


message("        Maintainer mode:          ${USE_MAINTAINER_MODE}              ")
message("        gcc coverage profiling:   ${enable_gcov}                      ")
message("        Building unit tests:      ${enable_tests}                     ")
message("        Building verbose mode:    ${enable_verbose_mode}              ")
message("        Building assertions:      ${enable_asserts}                   ")
message("        Building checks:          ${enable_checks}                    ")
message("        Building Qt4 bindings:    ${have_qt}                          ")
message("        Building Qt3 bindings:    ${have_qt3}                         ")
message("        Building GLib bindings:   ${have_glib}                        ")
message("        Building Python bindings: ${have_python}                      ")
message("        Building SELinux support: ${have_selinux}                     ")
message("        Building dnotify support: ${have_dnotify}                     ")
message("        Building Mono bindings:	  ${enable_mono}                     ")
message("        Building Mono docs:	      ${enable_mono_docs}                ")
message("        Building GTK+ tools:      ${have_gtk}                         ")
message("        Building X11 code:        ${enable_x11}                       ")
message("        Building Doxygen docs:    ${enable_doxygen_docs}              ")
message("        Building XML docs:        ${enable_xml_docs}                  ")
message("        Gettext libs (empty OK):  ${INTLLIBS}                         ")
message("        Using XML parser:         ${with_xml}                         ")
message("        Init scripts style:       ${with_init_scripts}                ")
message("        Abstract socket names:    ${have_abstract_sockets}            ")
message("        System bus socket:        ${DBUS_SYSTEM_SOCKET}               ")
message("        System bus address:       ${DBUS_SYSTEM_BUS_DEFAULT_ADDRESS}  ")
message("        System bus PID file:      ${DBUS_SYSTEM_PID_FILE}             ")
message("        Session bus socket dir:   ${DBUS_SESSION_SOCKET_DIR}          ")
message("        Console auth dir:         ${DBUS_CONSOLE_AUTH_DIR}            ")
message("        System bus user:          ${DBUS_USER}                        ")
message("        Session bus services dir: ${DBUS_SERVICE_DIR}                 ")
message("        'make check' socket dir:  ${TEST_SOCKET_DIR}                  ")


#if test x$enable_tests = xyes; then
#        echo "NOTE: building with unit tests increases the size of the installed library and renders it insecure."
#fi
#if test x$enable_tests = xyes -a x$enable_asserts = xno; then
#        echo "NOTE: building with unit tests but without assertions means tests may not properly report failures (this configuration is only useful when doing something like profiling the tests)"
#fi
#if test x$enable_gcov = xyes; then
#        echo "NOTE: building with coverage profiling is definitely for developers only."
#fi
#if test x$enable_verbose_mode = xyes; then
#        echo "NOTE: building with verbose mode increases library size, may slightly increase security risk, and decreases performance."
#fi
#if test x$enable_asserts = xyes; then
#        echo "NOTE: building with assertions increases library size and decreases performance."
#fi
#if test x$enable_checks = xno; then
#        echo "NOTE: building without checks for arguments passed to public API makes it harder to debug apps using D-BUS, but will slightly decrease D-BUS library size and _very_ slightly improve performance."
#fi


