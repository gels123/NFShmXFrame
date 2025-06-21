CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

if (UNIX)
#[[add_definitions(
        -Werror=format
)]]
endif ()

ADD_SUBDIRECTORY("NFrame")
ADD_SUBDIRECTORY("NFTest")
ADD_SUBDIRECTORY("NFServer")


