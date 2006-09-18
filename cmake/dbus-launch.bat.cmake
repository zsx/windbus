:: environment setting for dbus clients
@echo off

:: session bus address
set DBUS_SESSION_BUS_ADDRESS=@DBUS_SESSION_BUS_DEFAULT_ADDRESS@

:: system bus address
set DBUS_SYSTEM_BUS_DEFAULT_ADDRESS=@DBUS_SYSTEM_BUS_DEFAULT_ADDRESS@ 

if exist bus\build-session.conf (
  @echo "starting local dbus daemon"
	start bin\dbus-daemon --config-file=bus\build-session.conf
) else (
  @echo "starting global dbus daemon"
	start c:/Programme/dbus/bin/dbus-daemon --session
)