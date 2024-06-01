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
                vshell.c
                ctimer.c
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