cmake_minimum_required(VERSION 2.8)

project(libevaio C)

set(CMAKE_C_FLAGS "$ENV{CFLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "$ENV{LDFLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "$ENV{LDFLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS "$ENV{LDFLAGS}")

if(CMAKE_COMPILER_IS_GNUCC OR ${CMAKE_C_COMPILER_ID} STREQUAL "clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fvisibility=hidden -std=gnu99 -Wstrict-aliasing -Wchar-subscripts -Wcomment -Wformat=2 -Wimplicit-int -Werror-implicit-function-declaration -Wparentheses -Wsequence-point -Wreturn-type -Wswitch -Wtrigraphs -Wunused -Wunknown-pragmas -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wwrite-strings -Wsign-compare -Waggregate-return -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-noreturn -Wformat -Wmissing-format-attribute -Wno-deprecated-declarations -Wpacked -Wredundant-decls -Wnested-externs -Winline -Wconversion -pipe -Wextra -Wcast-align -Wno-missing-field-initializers -Wmain -Waddress -fstack-protector")

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie -Wl,-z,relro,-z,now")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,relro,-z,now")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-z,relro,-z,now")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(CMAKE_C_FLAGS_RELEASE "-O3 -Wuninitialized -DNDEBUG -D_FORTIFY_SOURCE=2")
        set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
    else()
            set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb3 -Wunreachable-code")
    endif()

endif()

add_definitions(`getconf LFS_CFLAGS`)

set(LIBRARY_OUTPUT_PATH lib)

add_subdirectory(src)
add_subdirectory(tests)

enable_testing()
add_test(evaio-test tests/${EXECUTABLE_OUTPUT_PATH}/evaio-test)
SET_TESTS_PROPERTIES(evaio-test PROPERTIES FAIL_REGULAR_EXPRESSION "failed")
