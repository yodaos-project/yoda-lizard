CMAKE_MINIMUM_REQUIRED(VERSION 3.2)
project(lizard)
set(VERSION 0.1)

include (${CUSTOM_CMAKE_MODULES}/common.mk)

option(BUILD_DEBUG "debug or release" OFF)
option(BUILD_DEMO "build demo and test programs" OFF)

findPackage(mutils REQUIRED
	HINTS ${mutilsPrefix}
	HEADERS uri.h
  INC_PATH_SUFFIX include/misc
  STATIC_LIBS misc
)

findPackage(mbedtls REQUIRED
  HINTS ${mbedtlsPrefix}
  HEADERS ssl.h
  INC_PATH_SUFFIX include/polarssl
  SHARED_LIBS mbedtls
)

set(CMAKE_CXX_STANDARD 11)
if (BUILD_DEBUG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
	set(CMAKE_SHARED_LINKER_FLAGS "-s")
endif()
if (CROSS_COMPILE_CXXFLAGS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CROSS_COMPILE_CXXFLAGS}")
endif()

file(GLOB_RECURSE lizard_SOURCES 
	include/*.h
	src/*.cpp
)

add_library(lizard SHARED
	${lizard_SOURCES}
)
target_include_directories(lizard PRIVATE
  include
	${mutils_INCLUDE_DIRS}
	${mbedtls_INCLUDE_DIRS}
)
target_link_libraries(lizard
	${mutils_LIBRARIES}
	${mbedtls_LIBRARIES}
)

# install include files.
file(GLOB lizard_HEADERS
	include/*.h
)
install(FILES ${lizard_HEADERS}
	DESTINATION include/lizard
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(TARGETS lizard
	LIBRARY DESTINATION lib
)

if (BUILD_DEMO)
add_executable(simple-sock demo/examples/simple-sock.cpp)
target_include_directories(simple-sock PRIVATE
  include
  ${mutils_INCLUDE_DIRS}
)
target_link_libraries(simple-sock
  ${mutils_LIBRARIES}
  lizard
)
add_executable(websocket demo/examples/websocket.cpp)
target_include_directories(websocket PRIVATE
  include
  ${mutils_INCLUDE_DIRS}
  ${mbedtls_INCLUDE_DIRS}
)
target_link_libraries(websocket
  ${mutils_LIBRARIES}
  ${mbedtls_LIBRARIES}
  lizard
)
endif(BUILD_DEMO)
