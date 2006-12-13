@echo off
::
:: Ralf Habacker <ralf.habacker@freenet.de>
:: 
:: patch dbus sources
:: check if patch exists on several places
:: 
if exist \cygwin\bin\patch.exe (
	\cygwin\bin\patch.exe -p0 -i DBus-win32.patch
) else (
	if exist %HOMEDRIVE%\cygwin\bin\patch.exe (
		%HOMEDRIVE%\cygwin\bin\patch.exe -p0 -i DBus-win32.patch
	) else (
		patch.exe -p0 -i DBus-win32.patch
	)
)
pause