cmake_minimum_required(VERSION 3.10.2)

# uncomment this to force build with gcc instead of clang
#set(CMAKE_C_COMPILER "gcc")

#set(CMAKE_OSX_DEPLOYMENT_TARGET "10.14" CACHE STRING "Minimum OS X deployment version")

project(libvterm C)
set(MAJOR_VERSION 6)
set(MINOR_VERSION 19)

message("CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

option(DEFINE_CURSES "Build with curses" ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -fno-omit-frame-pointer -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=c99 -Wall -fsanitize=address -fPIC -fno-plt")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O1 -std=c99 -Wall -fstack-protector-strong -fPIC -fno-plt")
set(PROJECT_DESCRIPTION "A rxvt/vt100 terminal emulator library written in C.")

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 6.0)))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()

# the following is until we learn how to reorder the gcc arguments to correctly link on Ubuntu

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
    color_math.c
    color_cache.c
    mouse_driver.c
    mouse_driver_X10.c
    mouse_driver_SGR.c
    vterm.c
    vterm_ctrl_char.c
    vterm_csi.c
    vterm_exec.c
    vterm_erase.c
    vterm_escape.c
    vterm_escape_simple.c
    vterm_escape_suffixes.c
    vterm_buffer.c
    vterm_color_map.c
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
    vterm_hook.c
    vterm_misc.c
    vterm_osc.c
    vterm_osc_DA.c
    vterm_read.c
    vterm_render.c
    vterm_reply.c
    vterm_resize.c
    vterm_scroll.c
    vterm_utf8.c
    vterm_wnd.c
    vterm_write.c
    stringv.c
    )

add_library(vterm.o OBJECT
    color_math.c
    color_cache.c
    mouse_driver.c
    mouse_driver_X10.c
    mouse_driver_SGR.c
    vterm.c
    vterm_buffer.c
    vterm_ctrl_char.c
    vterm_csi.c
    vterm_erase.c
    vterm_escape.c
    vterm_escape_simple.c
    vterm_escape_suffixes.c
    vterm_exec.c
    vterm_color_map.c
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
    vterm_hook.c
    vterm_misc.c
    vterm_osc.c
    vterm_osc_DA.c
    vterm_read.c
    vterm_render.c
    vterm_reply.c
    vterm_resize.c
    vterm_scroll.c
    vterm_utf8.c
    vterm_wnd.c
    vterm_write.c
    stringv.c
    )

add_library(vterm-shared SHARED
    $<TARGET_OBJECTS:vterm.o>
    )


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
                    ${CURSES_LIBRARIES})
    else()
        target_link_libraries(
                    vterm-shared
                    vterm-static
                    -lutil
                    -lm
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
                    -lutil
                    -lm
                    -lncursesw)
    else()

        target_link_libraries(
                    vterm-shared
                    vterm-static
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
set(PROJECT_VERSION 1.0)

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

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Bryan")
SET(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
SET(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${MAJOR_VERSION}.${MINOR_VERSION}")
set(CPACK_COMPONENTS_ALL Libraries)
include(CPack)
