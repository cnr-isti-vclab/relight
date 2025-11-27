# FindLCMS2.cmake
# Find the Little CMS 2 color management library
#
# This module defines:
#  LCMS2_FOUND - system has LCMS2
#  LCMS2_INCLUDE_DIRS - the LCMS2 include directory
#  LCMS2_LIBRARIES - Link these to use LCMS2
#  LCMS2_VERSION - the version of LCMS2 found

find_path(LCMS2_INCLUDE_DIR
    NAMES lcms2.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
)

find_library(LCMS2_LIBRARY
    NAMES lcms2 liblcms2
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
)

# Extract version from header file if found
if(LCMS2_INCLUDE_DIR AND EXISTS "${LCMS2_INCLUDE_DIR}/lcms2.h")
    file(STRINGS "${LCMS2_INCLUDE_DIR}/lcms2.h" LCMS2_VERSION_LINE
         REGEX "^#define[\t ]+LCMS_VERSION[\t ]+[0-9]+")
    string(REGEX REPLACE "^#define[\t ]+LCMS_VERSION[\t ]+([0-9]+).*" "\\1" LCMS2_VERSION "${LCMS2_VERSION_LINE}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LCMS2
    REQUIRED_VARS LCMS2_LIBRARY LCMS2_INCLUDE_DIR
    VERSION_VAR LCMS2_VERSION
)

if(LCMS2_FOUND)
    set(LCMS2_LIBRARIES ${LCMS2_LIBRARY})
    set(LCMS2_INCLUDE_DIRS ${LCMS2_INCLUDE_DIR})
    
    if(NOT TARGET LCMS2::LCMS2)
        add_library(LCMS2::LCMS2 UNKNOWN IMPORTED)
        set_target_properties(LCMS2::LCMS2 PROPERTIES
            IMPORTED_LOCATION "${LCMS2_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LCMS2_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(LCMS2_INCLUDE_DIR LCMS2_LIBRARY)
