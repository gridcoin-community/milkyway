# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)
SET(CrossCompile TRUE)
if(BUILD_32)
    SET(CMAKE_SYSTEM_PROCESSOR "i686")

    # which compilers to use for C and C++
    SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
    SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
    SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
    SET(CMAKE_LINKER i686-w64-mingw32-ld)
 
    # here is the target environment located
    SET(CMAKE_FIND_ROOT_PATH  /usr/i686-w64-mingw32 /home/weissj3/Desktop/CrossCompileAttempt )
else()
    SET(CMAKE_SYSTEM_PROCESSOR "x86_64")

    # which compilers to use for C and C++
    SET(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
    SET(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
    SET(CMAKE_LINKER x86_64-w64-mingw32-ld)
    # here is the target environment located
    SET(CMAKE_FIND_ROOT_PATH  /usr/x86_64-w64-mingw32 /home/weissj3/Desktop/CrossCompileAttempt )
endif()    
# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(HAVE_SYSTEM_THREADS 1)
set(HAVE_CLIENT_ID 1)
set(HAVE_VM_COUNTERS 1)
set(HAVE_THREAD_STATE 1)
