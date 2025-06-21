CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

if (UNIX)
#[[	message("nframe makefiles process")
    execute_process(COMMAND make
  						RESULT_VARIABLE retcode
  						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})]]
endif()


