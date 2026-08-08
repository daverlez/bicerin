find_package(SDL2 CONFIG REQUIRED)

add_executable(Bicerin main.cpp)

target_link_libraries(Bicerin PRIVATE
        bicerin_core
        SDL2::SDL2
)