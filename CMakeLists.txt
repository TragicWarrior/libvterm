cmake_minimum_required(VERSION 3.10.2)

# uncomment this to force build with gcc instead of clang
#set(CMAKE_C_COMPILER "gcc")

#set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

project(libvterm C)
set(MAJOR_VERSION 10)
set(MINOR_VERSION 4)

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

option(DEFINE_CURSES "Build with curses" ON)
option(ENABLE_BACKTRACE
    "Install a fatal-signal handler that writes a glibc backtrace to ./vterm-crash-<pid>.log"
    OFF)

# Link-time optimization: lets the compiler inline across translation
# unit boundaries.  libvterm has many small cross-TU helpers
# (vterm_buffer_get_active, clamp_cursor_to_bounds, the VCELL_* shapes)
# that benefit.  Disabled with ENABLE_BACKTRACE since that mode forces
# -O0 for predictable frames.
if(NOT ENABLE_BACKTRACE)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _libvterm_ipo_ok OUTPUT _libvterm_ipo_err)
    if(_libvterm_ipo_ok)
        message(STATUS "libvterm: link-time optimization ENABLED")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "libvterm: LTO unavailable: ${_libvterm_ipo_err}")
    endif()
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -std=c99 -Wall -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -fno-omit-frame-pointer -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -fsanitize=address -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -std=c99 -Wall -fstack-protector-strong -fPIC -fno-plt")
set(PROJECT_DESCRIPTION "A rxvt/vt100 terminal emulator library written in C.")

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 6.0)))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()

add_definitions("-D_REENTRANT")
add_definitions("-D_POSIX_C_SOURCE=200809L")

# needed for cchar_t definition and turn on "wide support"
add_definitions("-D_XOPEN_SOURCE_EXTENDED")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")
    add_definitions("-D__BSD_VISIBLE")
endif()


# set these before running the find_package directive for Curses
set(CURSES_NEED_NCURSES "TRUE")
set(CURSES_NEED_WIDE "TRUE")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")

    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-undefined -Wl,dynamic_lookup")

    set(CMAKE_PREFIX_PATH /usr/local/opt/ncurses)
    include_directories(/usr/local/opt/ncurses/include)

endif() 

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")

    set(CMAKE_PREFIX_PATH /usr/local/lib)
    include_directories(/usr/local/include)

endif()

find_package(Curses)

add_library(vterm-static STATIC
    color_cache.c
    color_math.c
    color_map.c
    mouse_driver.c
    mouse_driver_X10.c
    mouse_driver_SGR.c
    nc_wrapper.c
    vterm.c
    vterm_aio.c
    vterm_ctrl_char.c
    vterm_csi.c
    vterm_exec.c
    vterm_erase.c
    vterm_error.c
    vterm_escape.c
    vterm_escape_simple.c
    vterm_escape_suffixes.c
    vterm_buffer.c
    vterm_csi_CBT.c
    vterm_csi_CUP.c
    vterm_csi_CUx.c
    vterm_csi_DCH.c
    vterm_csi_DECSTBM.c
    vterm_csi_DL.c
    vterm_csi_ECH.c
    vterm_csi_ED.c
    vterm_csi_EL.c
    vterm_csi_ICH.c
    vterm_csi_IL.c
    vterm_csi_IRM.c
    vterm_csi_RESTORECUR.c
    vterm_csi_REP.c
    vterm_csi_RS1.c
    vterm_csi_SAVECUR.c
    vterm_csi_SD.c
    vterm_csi_DSR.c
    vterm_csi_SGR.c
    vterm_csi_SU.c
    vterm_csi_UNKNOWN.c
    vterm_cursor.c
    vterm_dec_DECALN.c
    vterm_dec_RM.c
    vterm_dec_SM.c
    vterm_esc_IND.c
    vterm_esc_NEL.c
    vterm_esc_RI.c
    vterm_history.c
    vterm_hook.c
    vterm_osc.c
    vterm_osc_DA.c
    vterm_read.c
    vterm_render.c
    vterm_reply.c
    vterm_resize.c
    vterm_scroll.c
    vterm_userptr.c
    vterm_utf8.c
    vterm_wnd.c
    vterm_write.c
    stringv.c
    )

add_library(vterm.o OBJECT
    color_cache.c
    color_math.c
    color_map.c
    mouse_driver.c
    mouse_driver_X10.c
    mouse_driver_SGR.c
    nc_wrapper.c
    vterm.c
    vterm_aio.c
    vterm_buffer.c
    vterm_ctrl_char.c
    vterm_csi.c
    vterm_erase.c
    vterm_error.c
    vterm_escape.c
    vterm_escape_simple.c
    vterm_escape_suffixes.c
    vterm_exec.c
    vterm_csi_CBT.c
    vterm_csi_CUP.c
    vterm_csi_CUx.c
    vterm_csi_DCH.c
    vterm_csi_DECSTBM.c
    vterm_csi_DL.c
    vterm_csi_ECH.c
    vterm_csi_ED.c
    vterm_csi_EL.c
    vterm_csi_ICH.c
    vterm_csi_IL.c
    vterm_csi_IRM.c
    vterm_csi_RESTORECUR.c
    vterm_csi_REP.c
    vterm_csi_RS1.c
    vterm_csi_SAVECUR.c
    vterm_csi_SD.c
    vterm_csi_DSR.c
    vterm_csi_SGR.c
    vterm_csi_SU.c
    vterm_csi_UNKNOWN.c
    vterm_cursor.c
    vterm_dec_DECALN.c
    vterm_dec_RM.c
    vterm_dec_SM.c
    vterm_esc_IND.c
    vterm_esc_NEL.c
    vterm_esc_RI.c
    vterm_history.c
    vterm_hook.c
    vterm_osc.c
    vterm_osc_DA.c
    vterm_read.c
    vterm_render.c
    vterm_reply.c
    vterm_resize.c
    vterm_scroll.c
    vterm_userptr.c
    vterm_utf8.c
    vterm_wnd.c
    vterm_write.c
    stringv.c
    )

add_library(vterm-shared SHARED
    $<TARGET_OBJECTS:vterm.o>
    )

#
# Optional backtrace handler.  When ENABLE_BACKTRACE is ON, vterm_debug.c
# is compiled into the regular libvterm.so/.a and its constructor installs
# fatal-signal handlers (SEGV/ABRT/BUS/FPE/ILL).  On signal, a glibc
# backtrace is dumped to ./vterm-crash-<pid>.log in the process's CWD and
# the signal is re-raised under its default disposition so the process
# still terminates (and cores if rlimits allow).
#
# Enable with:    cmake -DENABLE_BACKTRACE=ON  .
# Disable with:   cmake -DENABLE_BACKTRACE=OFF .
#
if(ENABLE_BACKTRACE)
    message(STATUS "libvterm: backtrace handler ENABLED")

    target_sources(vterm-static PRIVATE vterm_debug.c)
    target_sources(vterm.o      PRIVATE vterm_debug.c)

    # -g for line numbers, -O0 for predictable frames,
    # -fno-omit-frame-pointer for reliable unwinding
    target_compile_options(vterm-static PRIVATE
        -g -O0 -fno-omit-frame-pointer)
    target_compile_options(vterm.o PRIVATE
        -g -O0 -fno-omit-frame-pointer)

    target_compile_definitions(vterm-static PRIVATE VTERM_DEBUG_BACKTRACE)
    target_compile_definitions(vterm.o      PRIVATE VTERM_DEBUG_BACKTRACE)

    # -rdynamic exposes symbols so backtrace_symbols_fd() can resolve names
    set_property(TARGET vterm-shared APPEND_STRING
        PROPERTY LINK_FLAGS " -rdynamic")
endif()


if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

    if (Curses_FOUND)
        if(DEFINE_CURSES)
            add_executable(vshell
                demo/vshell.c
                demo/ctimer.c
                )
        endif(DEFINE_CURSES)
    endif()


    if ((Curses_FOUND) AND (DEFINE_CURSES))
        target_link_libraries(
                    vshell
                    vterm-shared
                    vterm-static
                    -lutil
                    -lm
                    -lrt
                    -ldl
                    ${CURSES_LIBRARIES})
    else()
        target_link_libraries(
                    vterm-shared
                    vterm-static
                    -lutil
                    -lm
                    -lrt
                    -ldl
                    ${CURSES_LIBRARIES})
    endif()
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")

    if(DEFINE_CURSES)
        add_executable(vshell
            demo/vshell.c
            demo/ctimer.c
            )
    endif(DEFINE_CURSES)

    if (DEFINE_CURSES)
        target_link_libraries(
                    vshell
                    vterm-shared
                    vterm-static
                    -lc
                    -lutil
                    -lm
                    -lncursesw)
    else()

        target_link_libraries(
                    vterm-shared
                    vterm-static
                    -lc
                    -lutil
                    -lm
                    -lncursesw)
    endif()
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")

    if(DEFINE_CURSES)
        add_executable(vshell
            demo/vshell.c
            demo/ctimer.c
            )
    endif(DEFINE_CURSES)

    #link_directories(/usr/local/opt/ncurses/lib/)

    if (DEFINE_CURSES)
        target_link_libraries(
                    vshell
                    vterm-shared
                    vterm-static
                    -lutil
                    -lm
                    /usr/local/opt/ncurses/lib/libncursesw.dylib)
    else()
        target_link_libraries(
                    vterm-shared
                    vterm-static
                    -lutil
                    -lm)
    endif()
endif()


# CMake doesn't allow targets with the same name.  This renames them properly afterward.
SET_TARGET_PROPERTIES(vterm-static PROPERTIES OUTPUT_NAME vterm CLEAN_DIRECT_OUTPUT 1)
SET_TARGET_PROPERTIES(vterm-shared PROPERTIES OUTPUT_NAME vterm CLEAN_DIRECT_OUTPUT 1)

# set the subst vars that configure_file() will use
set(PROJECT_VERSION ${MAJOR_VERSION}.${MINOR_VERSION})

set(PKG_CONFIG_LIBS
    "-lvterm"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkg-config.pc.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc"
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")

    if (Curses_FOUND)
        if(DEFINE_CURSES)
            install (TARGETS vshell DESTINATION bin)
        endif(DEFINE_CURSES)
    endif()

endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD")

    if(DEFINE_CURSES)
        install (TARGETS vshell DESTINATION bin)
    endif(DEFINE_CURSES)

endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")

    if(DEFINE_CURSES)
        install (TARGETS vshell DESTINATION bin)
    endif(DEFINE_CURSES)

endif()

install (TARGETS vterm-static vterm-shared DESTINATION lib)
install (FILES vterm.h DESTINATION include)

# Install pkgconfig files to libdata on BSD, otherwise lib
if(CMAKE_SYSTEM_NAME MATCHES "BSD")
    set(PKG_CONFIG_INSTALL_DIR "libdata/pkgconfig")
else()
    set(PKG_CONFIG_INSTALL_DIR "lib/pkgconfig")
endif()
install (FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION ${PKG_CONFIG_INSTALL_DIR})

#
# Optional regression harnesses (OFF by default; not installed).
#
#   cmake -DBUILD_TESTS=ON .
#   make
#   ./test_resize          # 8000 deterministic resize/scroll/copy iters
#   ./test_resize <seed>   # alternate size sequence
#   ./test_bufapi          # direct buffer-API: alt-screen clone + dealloc
#
# Built to corner the resize failure mode that sank earlier contiguous-
# buffer attempts.  Run under ASan for full value:
#   cmake -DBUILD_TESTS=ON -DCMAKE_C_FLAGS="-fsanitize=address -g" .
#
option(BUILD_TESTS "Build the libvterm regression test harnesses" OFF)
if(BUILD_TESTS)
    add_executable(test_resize test/test_resize.c)
    add_executable(test_bufapi test/test_bufapi.c)
    add_executable(test_shift test/test_shift.c)
    add_executable(test_history test/test_history.c)
    add_executable(test_render test/test_render.c)
    foreach(_t test_resize test_bufapi test_shift test_history test_render)
        target_include_directories(${_t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(${_t} vterm-static -lncursesw -lutil -lm -lrt)
    endforeach()
endif()

set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Bryan")
SET(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
SET(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${MAJOR_VERSION}.${MINOR_VERSION}")
set(CPACK_COMPONENTS_ALL Libraries)
include(CPack)
