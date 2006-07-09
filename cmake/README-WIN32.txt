dbus-daemon for win32 
=====================

News
----
2006-06-07
- added msvc patches from Peter Kümmel 
- fixed installation problem (qt headers are not more available)

2006-06-06
- stored patches in subdir patches 

2006-05-31
- install dbus-platform-deps.h
- enabled int64 type 
- disabled qt directory

2006-05-15
- initial msvc support (libdbus is created a static library because no DBUS_EXPORT definitions are available)
- initial glib support (not complete)
- dbus-monitor uses dbus main loop instead of glib main loop

2006-05-10
- add missing libraries libxml.dll and libiconv2.dll to binary package, gnuwin32 isn't required for the binary package

2006-05-09
- add installation target 
- config.h is completly written by build system 

- dbus-daemon could be started in session mode
		set PATH=c:\Programme\gnuwin32\bin;c:\Qt\4.1.2\bin;%PATH%
		start bin\dbus-daemon --session 

- service activation example added 
	  set DBUS_SESSION_BUS_ADDRESS=tcp:host=localhost,port=12434
		C:\Programme\dbus>bin\dbus-send  --dest=org.freedesktop.DBus --print-reply --type=method_call / org.freedesktop.DBus.StartServiceByName string:org.freedesktop.DBus.TestSuiteEchoService  uint32:2
			method return sender=org.freedesktop.DBus -> dest=:1.8
          uint32 2

- renamed dbus libary to dbus-1 and dbus-qt4 into dbus-qt4-1
- renamed qdbus to dbus 
	
2006-05-08
- uses patches from Tor Lillqvist 
- extended _dbus_verbose - it prints now the function name (gcc limited) 
- changes behavior of environment variable DBUS_VERBOSE - message are only printed if DBUS_VERBOSE=1, 
  makes it able to disable message printing using set DBUS_VERBOSE=0
- more testapplication are running (dbus-test,bus-test)
- qt bindings are now build 
- added non gui related qt examples 
- renamed qt/dbus test application to qdbus to avoid name clash with dbus library 
- fix returning user homedir in case domain controller isn't available
- seg fault fix accessing user_info in dbus/dbus-sysdeps.c:fill_win32_user_info_homedir()
- added new win32 related function dbus/dbus-sysdeps.c:_dbus_lm_strerror() for returning lan manager error messages
- added usage to qt/examples/dbus.cpp


2006-04-28
- first runable version allows connecting and sending data 
- authentification is currently disabled by hardcoding user to root (uid=0, gid=0)

TODO
====
- specifiy well known path for win32 unix protocol emulation in system.conf  (user's home or tmp dir ?)

- uses more dbus_warn() in case of errors instead of dbus_verbose()

- build system: 
	- add test running
	- build and install documentation 
	- complete file installation 
	- add win32 support to kdelibs FindQtDBus.cmake
	
- use binary path of running dbus-daemon to detect config base dir, because install in 
  windows may be different from compile time install prefix

- complete windows service support (see bus/service-win32.c )

- to be continued 



PLATFORM INDEPENDENT NOTES
==========================

While inspecting the source and docs I recognized some things which could be extended. 

-	rule description in dbus-monitor man page is missing, currently the source has to be 
  inspected to see how it works 

	type
		"method_call"  
		"method_return"
		"signal"       
		"error"        
	sender
	interface
	member
	path=
	destination
	
- config parser 
  - includedir should have an ignore_missing parameter 
 




How to compile dbus-daemon  (with mingw) 
----------------------------------------

1. Install Mingw from www.mingw.org  into c:\Mingw
2. Install gnuwin32 packages from http://webdev.cegit.de/snapshots/kde-windows/gnuwin32 into a subdir 
	 gnuwin32 in your program installation eg c:\Programme\gnuwin32 (german) or "c:\Program Files\gnuwin32" (english) 
3. download cmake from http://www.cmake.org/files/v2.4/cmake-2.4.1-win32.exe
4. install cmake into c:\cmake
5. unpack dbus-win32-2006-05-09-src.zip into c:\daten\dbus-win32
6. open command shell and run

		set PATH=c:\Mingw\bin;c:\Programme\gnuwin32\bin;%PATH%
		mkdir c:\daten\dbus-win32-build 
		c:\daten\cd dbus-win32-build 
		c:\Programme\cmake\bin\cmake -G "MinGW Makefiles" c:\daten\dbus-win32
		mingw32-make 

		set DBUS_SESSION_BUS_ADDRESS=tcp:host=localhost,port=12434
		
7. run test applications 

	dbus library check 
	
		C:\Daten\dbus-win32-build>bin\dbus-test c:\daten\dbus-win32\test\data
	
	bus daemon check 
		C:\Daten\dbus-win32-build>bin\bus-test c:\daten\dbus-win32\test\data

	check available names 
		C:\Daten\dbus-win32-build>bin\test_names.exe 

	check if dbus-daemon is accessable 

		C:\Daten\dbus-win32-build>bin\dbus-send --session --type=method_call --print-reply --dest=org.freedesktop.DBus / org.freedesktop.DBus.ListNames
		method return sender=org.freedesktop.DBus -> dest=:1.4
	  array [
	      string "org.freedesktop.DBus"
	      string ":1.4"
	   ]

		
How to run dbus-daemon from binary package
------------------------------------------

1. unpack binary package into c:\Programme\dbus  

2. open command shell and set environment
	c:\Programme\dbus
	
set DBUS_VERBOSE=0  
			(=1 for getting debug infos)
	
3. start dbus-daemon 

	start bin\dbus-daemon --session

4. run test service 
  set DBUS_SESSION_BUS_ADDRESS=tcp:host=localhost,port=12434

	C:\Programme\dbus>bin\dbus-send  --dest=org.freedesktop.DBus --print-reply --type=method_call / org.freedesktop.DBus.StartServiceByName string:org.freedesktop.DBus.TestSuiteEchoService  uint32:455
		method return sender=org.freedesktop.DBus -> dest=:1.8
   		uint32 2
