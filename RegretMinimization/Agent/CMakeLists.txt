add_library(Agent STATIC CFRAgent.cpp)

target_include_directories(Agent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Agent Trainer)

find_package(Boost COMPONENTS serialization filesystem)
if(Boost_FOUND)
    target_include_directories(Agent PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(Agent ${Boost_LIBRARIES})
endif()
