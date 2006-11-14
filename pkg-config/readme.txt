
Most dbus bindings require pkg-config to detect compiler/linker flags settings. 

To avoid patching the related build sytems we think it would be better to have  
a small sized pkgconfig special for dbus, which supports different compilers,
is able to support different installation locations and does not need glib. 

This application could be dropped easily in case the full sized pkg-config tool 
is able to handle all the need we have. 

Ralf Habacker 
