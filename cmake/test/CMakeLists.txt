
#if HAVE_GLIB
#    GLIB_SUBDIR=glib
#endif
#if HAVE_PYTHON
#    PYTHON_SUBDIR=python
#endif
#if HAVE_QTESTLIB
#    QT_SUBDIR=qt
#endif

add_subdirectory( name-test )

#SUBDIRS=name-test $(GLIB_SUBDIR) $(PYTHON_SUBDIR) $(QT_SUBDIR)
#DIST_SUBDIRS=glib python qt name-test

#INCLUDES=-I$(top_srcdir) $(DBUS_TEST_CFLAGS) 

include_directories( ${CMAKE_SOURCE_DIR}/.. )
include_directories( ${CMAKE_INCLUDE_PATH} )

if (WIN32)
add_definitions(-Dsleep=_sleep)
endif (WIN32)

set (TEST_SERVICE_DIR "/")
set (TEST_SOCKET_DIR "/")

#if (DBUS_BUILD_TESTS)
## break-loader removed for now
#TEST_BINARIES=test-service test-names test-shell-service shell-test spawn-test test-segfault test-exit test-sleep-forever

#enable stand alone make check test
#TESTS=shell-test
#else
#TEST_BINARIES=
#TESTS=
#endif

#if DBUS_GCOV_ENABLED
#GCOV_BINARIES=decode-gcov
#else
#GCOV_BINARIES=
#endif

set (test_service_SOURCES
	${CMAKE_SOURCE_DIR}/../test/test-service.c				
	${CMAKE_SOURCE_DIR}/../test/test-utils.c				
	${CMAKE_SOURCE_DIR}/../test/test-utils.h
)

set (test_names_SOURCES
	${CMAKE_SOURCE_DIR}/../test/test-names.c				
	${CMAKE_SOURCE_DIR}/../test/test-utils.c				
	${CMAKE_SOURCE_DIR}/../test/test-utils.h
)

set (break_loader_SOURCES
	${CMAKE_SOURCE_DIR}/../test/break-loader.c
)

set (test_shell_service_SOURCES
	${CMAKE_SOURCE_DIR}/../test/test-shell-service.c			
	${CMAKE_SOURCE_DIR}/../test/test-utils.c				
	${CMAKE_SOURCE_DIR}/../test/test-utils.h
)

set (test_shell_SOURCES
        ${CMAKE_SOURCE_DIR}/../test/shell-test.c
)

set (test_spawn_SOURCES
	${CMAKE_SOURCE_DIR}/../test/spawn-test.c
)

set (test_exit_SOURCES
	${CMAKE_SOURCE_DIR}/../test/test-exit.c
)

set (test_segfault_SOURCES
	${CMAKE_SOURCE_DIR}/../test/test-segfault.c
)

set (test_sleep_forever_SOURCES
	${CMAKE_SOURCE_DIR}/../test/test-sleep-forever.c
)

set (decode_gcov_SOURCES
	${CMAKE_SOURCE_DIR}/../test/decode-gcov.c
)

add_executable(test_service ${test_service_SOURCES})
target_link_libraries(test_service dbus-1)
install_targets(/bin test_service)

# (rh) does not work at now
ADD_TEST(test_service ${EXECUTABLE_OUTPUT_PATH}/test_service)

set (TEST_SERVICE_BINARY ${EXPANDED_BINDIR}/test_service.exe)
set (TEST_SEGFAULT_BINARY ${EXPANDED_BINDIR}/test_segfault.exe)

add_executable(test_names ${test_names_SOURCES})
target_link_libraries(test_names dbus-1)
install_targets(/bin test_names)
ADD_TEST(test_names ${EXECUTABLE_OUTPUT_PATH}/test_names)

add_executable(test_shell ${test_shell_SOURCES})
target_link_libraries(test_shell dbus-1)
install_targets(/bin test_shell)
ADD_TEST(test_shell ${EXECUTABLE_OUTPUT_PATH}/test_shell)

add_executable(test_shell_service ${test_shell_SOURCES})
target_link_libraries(test_shell_service dbus-1)
install_targets(/bin test_shell_service)
ADD_TEST(test_shell_service ${EXECUTABLE_OUTPUT_PATH}/test_shell_service)

add_executable(test_spawn ${test_spawn_SOURCES})
target_link_libraries(test_spawn dbus-1)
# (rh) does not work now
#ADD_TEST(test_spawn ${EXECUTABLE_OUTPUT_PATH}/test_spawn)

add_executable(test_exit ${test_exit_SOURCES})
target_link_libraries(test_exit dbus-1)
install_targets(/bin test_exit)
ADD_TEST(test_exit ${EXECUTABLE_OUTPUT_PATH}/test_exit)

add_executable(test_segfault ${test_segfault_SOURCES})
target_link_libraries(test_segfault dbus-1)
install_targets(/bin test_segfault)
ADD_TEST(test_segfault ${EXECUTABLE_OUTPUT_PATH}/test_segfault)

add_executable(test_sleep_forever ${test_sleep_forever_SOURCES})
target_link_libraries(test_sleep_forever dbus-1)
# (rh) don't know for what this test is good 
#ADD_TEST(test_sleep_forever ${EXECUTABLE_OUTPUT_PATH}/test_sleep_forever)

#add_executable(decode_gcov ${decode_gcov_SOURCES})
#target_link_libraries(decode_gcov dbus-1)

#EXTRA_DIST=
#
### keep these in creation order, i.e. uppermost dirs first 
set (TESTDIRS
	test/data					
	test/data/valid-messages			
	test/data/invalid-messages			
	test/data/incomplete-messages		
	test/data/auth				
	test/data/sha-1				
	test/data/valid-config-files			
	test/data/valid-config-files/basic.d		
	test/data/valid-config-files/system.d	
	test/data/valid-service-files		
	test/data/invalid-config-files		
	test/data/equiv-config-files			
	test/data/equiv-config-files/basic		
	test/data/equiv-config-files/basic/basic.d	
	test/data/equiv-config-files/entities	
	test/data/equiv-config-files/entities/basic.d
)
set (CONFIG_VERBOSE 0)

### copy tests to builddir so that generated tests and static tests 
### are all in one place.
FOREACH(FILE_TYPE *.message *.message-raw *.auth-script *.sha1 *.txt *.conf *.service)
	FOREACH(DIR ${TESTDIRS})
		FILE(GLOB FILES "${CMAKE_SOURCE_DIR}/../${DIR}/${FILE_TYPE}" )
		FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${DIR})
		FOREACH(FILE ${FILES})
			GET_FILENAME_COMPONENT(FILENAME ${FILE} NAME)
			SET (TARGET ${CMAKE_BINARY_DIR}/${DIR}/${FILENAME})
			configure_file(${FILE} ${TARGET} COPYONLY)
			IF (CONFIG_VERBOSE)
				MESSAGE("FROM: ${FILE}\nTO: ${TARGET}\n")
			ENDIF (CONFIG_VERBOSE)
		ENDFOREACH(FILE)
	ENDFOREACH(DIR)
ENDFOREACH(FILE_TYPE)

### generate test files
SET (CONFIG_FILES test/data/valid-config-files)

configure_file(${CMAKE_SOURCE_DIR}/${CONFIG_FILES}/debug-allow-all-sha1.conf.cmake ${CMAKE_BINARY_DIR}/${CONFIG_FILES}/debug-allow-all-sha1.conf )
configure_file(${CMAKE_SOURCE_DIR}/${CONFIG_FILES}/debug-allow-all.conf.cmake ${CMAKE_BINARY_DIR}/${CONFIG_FILES}/debug-allow-all.conf )

# (rh) disabled copying because bus-test could not parse this files for unknown reasons
#FILE(GLOB FILES "${CMAKE_BINARY_DIR}/bus/*.conf" )
#FOREACH(FILE ${FILES})	
#	GET_FILENAME_COMPONENT(FILENAME ${FILE} NAME)
#	SET (TARGET ${CMAKE_BINARY_DIR}/${CONFIG_FILES}/${FILENAME})
#	configure_file(${FILE} ${TARGET})
#	IF (CONFIG_VERBOSE)
#		MESSAGE("FROM: ${FILE}\nTO: ${TARGET}\n")
#	ENDIF (CONFIG_VERBOSE)
#ENDFOREACH(FILE)

SET (SERVICE_FILES test/data/valid-service-files)
FILE(GLOB FILES "${CMAKE_SOURCE_DIR}/../${SERVICE_FILES}/*.service.in" )
FOREACH(FILE ${FILES})	
	GET_FILENAME_COMPONENT(FILENAME ${FILE} NAME_WE)
	SET (TARGET ${CMAKE_BINARY_DIR}/${SERVICE_FILES}/${FILENAME}.service)
	IF (CONFIG_VERBOSE)
		MESSAGE("FROM: ${FILE}\nTO: ${TARGET}\n")
	ENDIF (CONFIG_VERBOSE)
	configure_file(${FILE} ${TARGET} )
	install_files(/data/dbus-1/services FILES ${TARGET})
ENDFOREACH(FILE)

#
#FIND_TESTS=find -name "*.message" -o -name "*.message-raw" -o -name "*.auth-script" -o -name "*.sha1" -o -name "*.txt" -o -name "*.conf" -o -name "*.service"
#
### copy tests to builddir so that generated tests and static tests 
### are all in one place.
#all-local:
#	for D in $(TESTDIRS); do								
#		test -d $(top_builddir)/test/$$D || mkdir $(top_builddir)/test/$$D || exit 1 ;	
#	done ;											
#	if ! (test $(srcdir) = . || test $(srcdir) -ef .) ; then								
#		FILES=`(cd $(srcdir) && $(FIND_TESTS))` ;					
#	        for F in $$FILES; do								
#			SRC=$(srcdir)/$$F ;							
#			DEST=$(top_builddir)/test/$$F ;						
#	                echo '-- Copying test file '$$F ;					
#			cp $$SRC $$DEST || exit 1 ;						
#			chmod u+w $$DEST || exit 1 ;						
#		done ;										
#	else											
#		echo '-- No need to copy test data as srcdir = builddir' ;			
#	fi ;											
#	echo '-- Copying' $(top_builddir)/bus/*.conf 'to test directory' ;			
#	cp $(top_builddir)/bus/*.conf $(top_builddir)/test/data/valid-config-files || exit 1 ;  
#	chmod u+w $(top_builddir)/test/data/valid-config-files/*.conf || exit 1
#
### this doesn't clean generated test data files when srcdir=builddir
#clean-local:
#	if test $(srcdir) != . ; then					
#		FILES=`(cd $(top_builddir)/test && $(FIND_TESTS))` ;	
#	        for F in $$FILES; do					
#			DEST=$(top_builddir)/test/$$F ;			
#	                echo '-- Deleting test file '$$F ;		
#			rm $$DEST || exit 1 ;				
#		done ;							
#		REVERSEDIRS= ;						
#		for D in $(TESTDIRS); do				
#			REVERSEDIRS="$$D $$REVERSEDIRS" ;		
#		done ;							
#		for D in $$REVERSEDIRS; do				
#			rmdir $(top_builddir)/test/$$D || exit 1 ;	
#		done ;							
#	fi
