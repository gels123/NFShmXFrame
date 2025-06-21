CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(NFNetPlugin)

AUX_SOURCE_DIRECTORY(. SRC)
AUX_SOURCE_DIRECTORY(Evpp SRC)
AUX_SOURCE_DIRECTORY(Bus SRC)
AUX_SOURCE_DIRECTORY(Enet SRC)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	SET(LIBRARY_OUTPUT_PATH "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Release")
	ADD_LIBRARY( ${PROJECT_NAME} STATIC ${SRC} )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	SET(LIBRARY_OUTPUT_PATH "${CMAKE_NFSHM_SOURCE_DIR}/Build/Lib/Debug")
	ADD_LIBRARY( ${PROJECT_NAME} STATIC ${SRC} )
elseif (CMAKE_BUILD_TYPE STREQUAL "DynamicRelease")
	SET(LIBRARY_OUTPUT_PATH "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Release")
	ADD_LIBRARY( ${PROJECT_NAME} SHARED ${SRC} )
	add_dependencies(${PROJECT_NAME} evpp)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} libevpp.a libevent_core.a libevent_openssl.a libevent_extra.a libevent.a libcurl.a libssl.a libcrypto.a libz.a libenet.a)
elseif(CMAKE_BUILD_TYPE STREQUAL "DynamicDebug")
	SET(LIBRARY_OUTPUT_PATH "${CMAKE_NFSHM_SOURCE_DIR}/Install/Bin/Dynamic_Debug")
	ADD_LIBRARY( ${PROJECT_NAME} SHARED ${SRC} )
	add_dependencies(${PROJECT_NAME} evpp)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} libevpp.a libevent_core.a libevent_openssl.a libevent_extra.a libevent.a libcurl.a libssl.a libcrypto.a libz.a libenet.a)
endif()




