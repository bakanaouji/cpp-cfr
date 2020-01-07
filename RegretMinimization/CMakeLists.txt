cmake_minimum_required(VERSION 3.5)
project(cpp_cfr)

set(CMAKE_CXX_STANDARD 14)

file(GLOB KuhnSource ../Game/Kuhn/*.cpp)
add_executable(cpp_cfr main.cpp Trainer/Trainer.cpp Trainer/Node.cpp ${KuhnSource})

include_directories(Trainer ../Game/Kuhn ../lib)

find_package(Boost COMPONENTS serialization filesystem)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(cpp_cfr ${Boost_LIBRARIES})
endif()

add_subdirectory(Trainer)
