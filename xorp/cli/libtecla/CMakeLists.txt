# rtrmgr build
# Copyright (c) 2021 Michał Zagórski (zagura)
# SPDX-License-Identifier: GPL-2.0-or-later
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_library(tecla SHARED 
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

target_compile_definitions(tecla PUBLIC 
                                        HAVE_INTTYPES_H=1
                                        HAVE_MEMORY_H=1
                                        HAVE_STDINT_H=1
                                        HAVE_STDLIB_H=1
                                        HAVE_STRINGS_H=1
                                        HAVE_STRING_H=1
                                        HAVE_SYS_STAT_H=1
                                        HAVE_SYS_TYPES_H=1
                                        HAVE_TERMINFO_H=1
                                        HAVE_UNISTD_H=1
                                        STDC_HEADERS=1
                                        HAVE_SYS_IOCTL_H=1)
if (NOT MINGW)
    target_compile_definitions(tecla PUBLIC HAVE_SYS_IOCTL_H=1
                                    HAVE_TERMIOS_H=1)
endif()

install(TARGETS tecla RUNTIME DESTINATION lib)

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
target_include_directories(tecla PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CURSES_INCLUDE_DIRS} ${XORP_BUILD_ROOT})
target_link_libraries(tecla ${CURSES_LIBRARIES})



# HAVE_INTTYPES_H=1 
# HAVE_MEMORY_H=1 
# HAVE_STDINT_H=1 
# HAVE_STDLIB_H=1 
# HAVE_STRINGS_H=1 
# HAVE_STRING_H=1 
# HAVE_SYS_STAT_H=1 
# HAVE_SYS_TYPES_H=1 
# HAVE_TERMINFO_H=1 
# HAVE_UNISTD_H=1 
# STDC_HEADERS=1