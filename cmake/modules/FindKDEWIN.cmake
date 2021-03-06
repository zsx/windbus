# - Try to find the install path of the required win32 supplementary libraries 
# 
# used environment vars 
#  KDEWIN_DIR  - kdewin root dir 
#
# this will define
#  KDEWIN_FOUND - system has KDEWIN
#  KDEWIN_DIR - the KDEWIN root installation dir
#
# Copyright (c) 2007, Ralf Habacker, <ralf.habacker@freenet.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (WIN32)
  IF (NOT KDEWIN_DIR)
    # check for enviroment variable
    file(TO_CMAKE_PATH "$ENV{KDEWIN_DIR}" KDEWIN_DIR)
    if(NOT KDEWIN_DIR)
      file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _progFiles)
      if (MINGW)
        set (DIR "kdewin-mingw")
      else (MINGW)
        set (DIR "kdewin-msvc")
      endif (MINGW)
      
      # search in the default program install folder
      find_file(KDEWIN_DIR_tmp ${DIR} gnuwin32 win32libs kdewin
      PATHS
        "${_progFiles}"
      )
      set (KDEWIN_DIR ${KDEWIN_DIR_tmp})
    endif (NOT KDEWIN_DIR)
    if (KDEWIN_DIR)
      message(STATUS "Found windows supplementary package location: ${KDEWIN_DIR}")
    endif (KDEWIN_DIR)
  endif (NOT KDEWIN_DIR)

  # this must be set every time 
  if (KDEWIN_DIR)
    # add include path and library to all targets, this is required because 
    # cmake's 2.4.6 FindZLib.cmake does not use CMAKE_REQUIRED... vars
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${KDEWIN_DIR}/include)
    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${KDEWIN_DIR}/lib)
     
    set (KDEWIN_FOUND 1)
  else(KDEWIN_DIR)
    message(STATUS "Could not find the location of the windows supplementary packages which is \n"
                    "\t\tenvironment variable KDEWIN_DIR\n"
                    "\t\t<ProgramFiles>/${DIR}\n"
                    "\t\t<ProgramFiles>/gnuwin32\n"
                    "\t\t<ProgramFiles>/win32libs\n" 
                    "\t\t<ProgramFiles>/kdewin\n")
   endif(KDEWIN_DIR)
  
endif (WIN32)
