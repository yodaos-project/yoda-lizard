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

if (SSL_LINK STREQUAL "shared")
  set(findPackageArgs "SHARED_LIBS;${SSL_LIB}")
else()
  set(findPackageArgs "STATIC_LIBS;${SSL_LIB}")
endif()
if (SSL_LIB STREQUAL "mbedtls")
findPackage(ssl REQUIRED
  HINTS ${mbedtlsPrefix}
  HEADERS ssl.h
  INC_PATH_SUFFIX include/polarssl
  ${findPackageArgs}
)
set(lizardCXXFLAGS "-DHAS_SSL")
endif()

set(CMAKE_CXX_STANDARD 11)
if (BUILD_DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DLIZARD_DEBUG")
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
target_compile_options(lizard PRIVATE ${lizardCXXFLAGS})
target_include_directories(lizard PRIVATE
  include
  ${mutils_INCLUDE_DIRS}
  ${ssl_INCLUDE_DIRS}
)
target_link_libraries(lizard
  ${mutils_LIBRARIES}
  ${ssl_LIBRARIES}
)
add_library(lizard_static STATIC
  ${lizard_SOURCES}
)
set_target_properties(lizard_static PROPERTIES
  OUTPUT_NAME lizard
  POSITION_INDEPENDENT_CODE ON
)
target_compile_options(lizard_static PRIVATE ${lizardCXXFLAGS})
target_include_directories(lizard_static PRIVATE
  include
  ${mutils_INCLUDE_DIRS}
  ${ssl_INCLUDE_DIRS}
)

# install include files.
file(GLOB lizard_HEADERS
  include/*.h
)
install(FILES ${lizard_HEADERS}
  DESTINATION include/lizard
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install(TARGETS lizard lizard_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
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
  ${ssl_INCLUDE_DIRS}
)
target_link_libraries(websocket
  ${mutils_LIBRARIES}
  ${ssl_LIBRARIES}
  lizard
)
install(TARGETS simple-sock websocket
  RUNTIME DESTINATION bin
)
endif(BUILD_DEMO)
