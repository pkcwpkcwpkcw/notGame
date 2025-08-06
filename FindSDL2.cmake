# FindSDL2.cmake - Find SDL2 library for Windows with MSYS2/MinGW/vcpkg

# SDL2 library paths - vcpkg를 우선으로 검색
set(SDL2_SEARCH_PATHS
    "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/x86-windows"
    "C:/msys64/ucrt64"
    "C:/msys64/mingw64"
)

# Find SDL2 include directory
find_path(SDL2_INCLUDE_DIR
    NAMES SDL.h
    PATH_SUFFIXES include/SDL2 include
    PATHS ${SDL2_SEARCH_PATHS}
)

# Find SDL2 library
find_library(SDL2_LIBRARY
    NAMES SDL2 libSDL2 SDL2.dll
    PATH_SUFFIXES lib bin
    PATHS ${SDL2_SEARCH_PATHS}
)

# Find SDL2main library
find_library(SDL2MAIN_LIBRARY
    NAMES SDL2main libSDL2main
    PATH_SUFFIXES lib
    PATHS ${SDL2_SEARCH_PATHS}
)

# Set SDL2 variables
if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
    set(SDL2_FOUND TRUE)
    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    set(SDL2_LIBRARIES ${SDL2_LIBRARY})
    
    if(SDL2MAIN_LIBRARY)
        list(APPEND SDL2_LIBRARIES ${SDL2MAIN_LIBRARY})
    endif()
    
    message(STATUS "SDL2 found at ${SDL2_INCLUDE_DIR}")
    message(STATUS "SDL2 library: ${SDL2_LIBRARY}")
else()
    set(SDL2_FOUND FALSE)
endif()