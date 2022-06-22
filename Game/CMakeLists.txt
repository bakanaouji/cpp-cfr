add_executable(game main.cpp)

target_link_libraries(game Kuhn Trainer Agent)
target_include_directories(game PRIVATE ../lib)

add_subdirectory(Kuhn)
