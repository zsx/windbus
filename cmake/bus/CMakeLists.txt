
include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories( ${CMAKE_INCLUDE_PATH} )

add_definitions(-DDBUS_COMPILATION)

SET (EFENCE "")

set (config_DATA
	session.conf				
	system.conf
)

FOREACH(file ${config_DATA})
	CONFIGURE_FILE( "${CMAKE_CURRENT_SOURCE_DIR}/${file}.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${file}" IMMEDIATE @ONLY)
ENDFOREACH(file)


set (XML_SOURCES config-loader-libxml.c)
include_directories(${LIBXML2_INCLUDE_DIR})
# xml2 requires iconv 
SET (LIBS ${LIBXML2_LIBRARIES} iconv)


if (DBUS_USE_EXPAT)
set (XML_SOURCES config-loader-expat.c)
endif (DBUS_USE_EXPAT)


file(GLOB BUS_SOURCES ${CMAKE_SOURCE_DIR}/../bus/*.c)
file(GLOB BUS_HEADERS ${CMAKE_SOURCE_DIR}/../bus/*.h)



list(REMOVE_ITEM BUS_SOURCES
	${CMAKE_SOURCE_DIR}/../bus/main.c					
	${CMAKE_SOURCE_DIR}/../bus/test-main.c
	${CMAKE_SOURCE_DIR}/../bus/config-loader-expat.c
	${CMAKE_SOURCE_DIR}/../bus/dir-watch-kqueue.c
	${CMAKE_SOURCE_DIR}/../bus/dir-watch-dnotify.c
	${CMAKE_SOURCE_DIR}/../bus/dir-watch-default.c)

add_executable(dbus-daemon ${BUS_SOURCES} ${BUS_HEADERS} ${CMAKE_SOURCE_DIR}/../bus/main.c)
target_link_libraries(dbus-daemon dbus-1 ${LIBS})
install_targets(/bin dbus-daemon )
install_files(/etc FILES ${config_DATA})

## note that TESTS has special meaning (stuff to use in make check)
## so if adding tests not to be run in make check, don't add them to 
## TESTS
#if DBUS_BUILD_TESTS
#TESTS_ENVIRONMENT=DBUS_TEST_DATA=$(top_builddir)/test/data DBUS_TEST_HOMEDIR=$(top_builddir)/dbus
#TESTS=bus-test 
#else
#TESTS=
#endif

## we use noinst_PROGRAMS not check_PROGRAMS so that we build 
## even when not doing "make check"
#noinst_PROGRAMS=$(TESTS) 

#bus_test_SOURCES=				
#	$(BUS_SOURCES)				
#	test-main.c


set (bus_test_SOURCES
	${BUS_SOURCES}
	test-main.c)

add_executable(bus-test ${BUS_SOURCES} ${BUS_HEADERS} ${CMAKE_SOURCE_DIR}/../bus/test-main.c	)
target_link_libraries(bus-test dbus-1 ${LIBS} )
install_targets(/bin bus-test)
add_test(bus-test ${EXECUTABLE_OUTPUT_PATH}/bus-test ${CMAKE_SOURCE_DIR}/../test/data)

## mop up the gcov files
#clean-local:
#	/bin/rm *.bb *.bbg *.da *.gcov || true

#install-data-hook:
#	$(mkinstalldirs) $(DESTDIR)/$(localstatedir)/run/dbus
#	$(mkinstalldirs) $(DESTDIR)/$(configdir)/system.d
#	$(mkinstalldirs) $(DESTDIR)/$(datadir)/dbus-1/services

##install_file(${configdir}/system.d FILE



#### Init scripts fun
#SCRIPT_IN_FILES=messagebus.in
#		rc.messagebus.in

## Red Hat start
#if DBUS_INIT_SCRIPTS_RED_HAT

#initddir=$(sysconfdir)/rc.d/init.d

#initd_SCRIPTS= 	
#	messagebus

#endif
# ## Red Hat end

## Slackware start
#if DBUS_INIT_SCRIPTS_SLACKWARE

#initddir=$(sysconfdir)/rc.d/

#initd_SCRIPTS= 	
#	rc.messagebus

#endif
## Slackware end

#MAN_IN_FILES=dbus-daemon.1.in
#man_MANS = dbus-daemon.1

#### Extra dist 

#EXTRA_DIST=$(CONFIG_IN_FILES) $(SCRIPT_IN_FILES) $(man_MANS) $(MAN_IN_FILES)
