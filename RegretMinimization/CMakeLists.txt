add_executable(run_cfr main.cpp)

target_include_directories(run_cfr PRIVATE ../lib)
target_link_libraries(run_cfr Trainer Kuhn)

add_subdirectory(Agent)
add_subdirectory(Trainer)
