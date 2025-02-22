cmake_minimum_required(VERSION 3.22.0)
project(traffic-signal)

# List your source files here
set(SourceFiles
    simulator.c
    trafficGenerator.c
)

# Find SDL2 and manually add SDL2_ttf
find_package(SDL2 REQUIRED)
find_library(SDL2_TTF_LIBRARY SDL2_ttf)

# Include SDL2 and SDL2_ttf directories
include_directories(${SDL2_INCLUDE_DIRS})

# Add the source files to the executable
add_executable(${PROJECT_NAME} ${SourceFiles})

# Link SDL2 and SDL2_ttf libraries
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARY})
