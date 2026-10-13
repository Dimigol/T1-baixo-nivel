cmake_minimum_required (VERSION 2.8.9)

project(bagunceitor)


set(INCLUDE_DIRS include)

include_directories(${INCLUDE_DIRS})

file(GLOB SOURCE_FILES main.c)
file(GLOB INCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(bagunceitor ${SOURCE_FILES} ${INCLUDE_FILES})

target_link_libraries(bagunceitor ${LIBRARIES} m)

