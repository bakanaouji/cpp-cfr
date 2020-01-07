cmake_minimum_required(VERSION 3.5)
project(game)

set(CMAKE_CXX_STANDARD 14)

file(GLOB CFRSource ../RegretMinimization/Trainer/*.cpp)
file(GLOB AgentSource ../RegretMinimization/Agent/*.cpp)
file(GLOB KuhnSource Kuhn/*.cpp)
add_executable(game main.cpp ${KuhnSource} ${CFRSource} ${AgentSource})

include_directories(Kuhn ../RegretMinimization/Agent ../RegretMinimization/Trainer ../lib)

find_package(Boost COMPONENTS serialization filesystem)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(game ${Boost_LIBRARIES})
endif()

add_subdirectory(Kuhn)
