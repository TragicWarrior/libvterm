cmake_minimum_required(VERSION 2.8)

project(libvterm)

option(DEFINE_CURSES "Build with curses" ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")
set(PROJECT_DESCRIPTION "A rxvt/vt100 terminal emulator library written in C.")

if (("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_GREATER 4.0)))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra")
endif()

# the following is until we learn how to reorder the gcc arguments to correctly link on Ubuntu
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")

add_definitions("-D_REENTRANT")
#add_definitions("-D_POSIX_C_SOURCE=200112L")
add_definitions("-D_POSIX_C_SOURCE=200809L")


set(CURSES_NEED_NCURSES "TRUE")
find_package(Curses)

if (!DEFINE_CURSES)
    add_definitions("-DNOCURSES")
endif()

if (!Curses_FOUND)
    add_definitions("-DNOCURSES")
endif()

add_library(vterm-static STATIC
    vterm.c
    vterm_exec.c
    vterm_colors.c
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
    vterm_csi_RESTORECUR.c
    vterm_csi_SAVECUR.c
    vterm_csi_SGR.c
    vterm_csi_UNKNOWN.c
    vterm_dec_RM.c
    vterm_dec_SM.c
    vterm_erase.c
    vterm_escape.c
    vterm_misc.c
    vterm_read.c
    vterm_render.c
    vterm_resize.c
    vterm_scroll.c
    vterm_utf8.c
    vterm_wnd.c
    vterm_write.c
    )

add_library(vterm.o OBJECT
    vterm.c
    vterm_exec.c
    vterm_colors.c
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
    vterm_csi_RESTORECUR.c
    vterm_csi_SAVECUR.c
    vterm_csi_SGR.c
    vterm_csi_UNKNOWN.c
    vterm_dec_RM.c
    vterm_dec_SM.c
    vterm_erase.c
    vterm_escape.c
    vterm_misc.c
    vterm_read.c
    vterm_render.c
    vterm_resize.c
    vterm_scroll.c
    vterm_utf8.c
    vterm_wnd.c
    vterm_write.c
    )

add_library(vterm-shared SHARED
    $<TARGET_OBJECTS:vterm.o>
    )

if (Curses_FOUND)
    if(DEFINE_CURSES)
        add_executable(vshell
            demo/vshell.c
            )
    endif(DEFINE_CURSES)
endif()

if ((Curses_FOUND) AND (DEFINE_CURSES))
target_link_libraries(vshell vterm-shared vterm-static -lutil ${CURSES_LIBRARIES} )
else()
target_link_libraries(vterm-shared vterm-static -lutil ${CURSES_LIBRARIES} )
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

if (Curses_FOUND)
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
