# libtecla build
# Copyright (c) 2021-2024 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_library(tecla
                        cplfile.c
                        cplmatch.c
                        direader.c
                        expand.c
                        freelist.c
                        getline.c
                        hash.c
                        history.c
                        homedir.c
                        keytab.c
                        pathutil.c
                        pcache.c
                        stringrp.c
                        strngmem.c
                        version.c
            )

foreach(DEF IN ITEMS
                     HAVE_INTTYPES_H
                     HAVE_MEMORY_H
                     HAVE_STDINT_H
                     HAVE_STDLIB_H
                     HAVE_STRINGS_H
                     HAVE_STRING_H
                     HAVE_SYS_STAT_H
                     HAVE_SYS_TYPES_H
                     HAVE_TERMINFO_H
                     HAVE_UNISTD_H
                     STDC_HEADERS
                     HAVE_SYS_IOCTL_H
                     HAVE_SYS_UIO_H
                     HAVE_TERMIOS_H
        )
   if(${DEF})
       target_compile_definitions(tecla PUBLIC "${DEF}=1")
   endif()
endforeach()

install(TARGETS tecla LIBRARY DESTINATION ${XORP_LIBDIR})

# target_compile_options(tecla PUBLIC -Werror)
find_package(Curses)
# target_compile_options(tecla PUBLIC  -Wall
# -Wwrite-strings
# -Wbad-function-cast
# -Wmissing-prototypes
# -Wcast-qual
# -Wmissing-declarations
# -Wpointer-arith
# -Wcast-align
# -Wstrict-prototypes
# -Wnested-externs)
target_include_directories(tecla PUBLIC ${CURSES_INCLUDE_DIRS})
target_link_libraries(tecla ${CURSES_LIBRARIES})
if (RTLD_ORIGIN)
    set_target_properties(tecla PROPERTIES BUILD_RPATH "$ORIGIN:$ORIGIN/../lib"
                                                   INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()
if (BUILDDIR_RUN)
    add_custom_command(TARGET tecla POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $<TARGET_FILE:tecla>
                        ${XORP_ALIAS_LIBDIR}/$<TARGET_FILE_NAME:tecla>)
endif()
