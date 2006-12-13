::@echo off
::
:: Ralf Habacker <ralf.habacker@freenet.de>
:: 
:: build dbus sources
:: 

set CMAKE=\cmake-2.4.4-win32-x86\bin\cmake.exe
set MINGW=\mingw
set PATH=%MINGW%\bin;%ProgramFILES%\gnuwin32\bin;%PATH%
set SOURCE_DIR=%CD%

if not exist ..\windbus-build (
	mkdir ..\windbus-build
) else (
	::rmdir /Q /S ..\windbus-build
	::mkdir ..\windbus-build
)

cd ..\windbus-build
%CMAKE% -G "MinGW Makefiles" %SOURCE_DIR%\cmake
mingw32-make
bin\dbus-test test\data
::bin\bus-test test\data
mingw32-make install 
mingw32-make package

pause
