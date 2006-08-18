# - Try to find LIBEXPAT
# Once done this will define
#
#  LIBEXPAT_FOUND - system has LIBEXPAT
#  LIBEXPAT_INCLUDE_DIR - the LIBEXPAT include directory
#  LIBEXPAT_LIBRARIES - the libraries needed to use LIBEXPAT
#  LIBEXPAT_DEFINITIONS - Compiler switches required for using LIBEXPAT

if (LIBEXPAT_INCLUDE_DIR AND LIBEXPAT_LIBRARIES)

    # in cache already
    SET(LIBEXPAT_FOUND TRUE)

else (LIBEXPAT_INCLUDE_DIR AND LIBEXPAT_LIBRARIES)

    IF (NOT WIN32)
        MESSAGE(FATAL_ERROR "Please set this to the correct values!")
        # use pkg-config to get the directories and then use these values
        # in the FIND_PATH() and FIND_LIBRARY() calls
        INCLUDE(UsePkgConfig)
        PKGCONFIG(LIBEXPAT-2.0 _LIBEXPATIncDir _LIBEXPATLinkDir _LIBEXPATLinkFlags _LiIconvCflags)
        SET(LIBEXPAT_DEFINITIONS ${_LIBEXPATCflags})
    ENDIF (NOT WIN32)

    FIND_PATH(LIBEXPAT_INCLUDE_DIR expat.h
      PATHS
     ${_LIBEXPATIncDir}
      PATH_SUFFIXES LIBEXPAT
    )

    FIND_LIBRARY(LIBEXPAT_LIBRARIES NAMES expat libexpat
      PATHS
      ${_LIBEXPATLinkDir}
    )

    if (LIBEXPAT_INCLUDE_DIR AND LIBEXPAT_LIBRARIES)
       set(LIBEXPAT_FOUND TRUE)
    endif (LIBEXPAT_INCLUDE_DIR AND LIBEXPAT_LIBRARIES)

    if (LIBEXPAT_FOUND)
      if (NOT LIBEXPAT_FIND_QUIETLY)
        message(STATUS "Found libexpat: ${LIBEXPAT_LIBRARIES}")
      endif (NOT LIBEXPAT_FIND_QUIETLY)
    else (LIBEXPAT_FOUND)
      if (LIBEXPAT_FIND_REQUIRED)
        message(SEND_ERROR "Could NOT find libexpat")
      endif (LIBEXPAT_FIND_REQUIRED)
    endif (LIBEXPAT_FOUND)

    MARK_AS_ADVANCED(LIBEXPAT_INCLUDE_DIR LIBEXPAT_LIBRARIES)

endif (LIBEXPAT_INCLUDE_DIR AND LIBEXPAT_LIBRARIES)
