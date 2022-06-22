add_library(Trainer STATIC Node.cpp Trainer.cpp)

target_include_directories(Trainer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Boost COMPONENTS serialization filesystem)
if(Boost_FOUND)
    target_include_directories(Trainer PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(Trainer ${Boost_LIBRARIES})
endif()
