cmake_minimum_required(VERSION 2.8.11)

project(avim)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# proto select
option(AVPROTO_ENABLE "use libavproto" ON)
option(AVPROTO_3RD_ENABLE "use libavproto_3rd" OFF)

# service on-off
option(SERVICE_GROUP_ENABLE "build group service" ON)

find_package(Boost 1.54 REQUIRED COMPONENTS thread system program_options coroutine context regex date_time chrono filesystem )
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${Boost_INCLUDE_DIRS}
	${OPENSSL_INCLUDE_DIR}
	include/
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
link_libraries(${Boost_LIBRARIES})
link_libraries(${OPENSSL_LIBRARIES})

add_subdirectory(libavim)

add_subdirectory(src)
