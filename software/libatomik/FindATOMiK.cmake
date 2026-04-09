# FindATOMiK.cmake — CMake find module for libatomik
#
# Provides: ATOMiK::atomik imported target
#
# Usage in CMakeLists.txt:
#   find_package(ATOMiK REQUIRED)
#   target_link_libraries(myapp ATOMiK::atomik)

find_path(ATOMIK_INCLUDE_DIR
    NAMES libatomik.h
    PATHS ${ATOMIK_ROOT}/include /usr/local/include /usr/include
)

find_library(ATOMIK_LIBRARY
    NAMES atomik
    PATHS ${ATOMIK_ROOT}/lib /usr/local/lib /usr/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ATOMiK
    REQUIRED_VARS ATOMIK_LIBRARY ATOMIK_INCLUDE_DIR
)

if(ATOMiK_FOUND AND NOT TARGET ATOMiK::atomik)
    add_library(ATOMiK::atomik UNKNOWN IMPORTED)
    set_target_properties(ATOMiK::atomik PROPERTIES
        IMPORTED_LOCATION "${ATOMIK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ATOMIK_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(ATOMIK_INCLUDE_DIR ATOMIK_LIBRARY)
