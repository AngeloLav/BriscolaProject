find_package(OpenCV REQUIRED)

add_executable(briscola src/main.cpp)

target_link_libraries(
    briscola
    PRIVATE
    ${OpenCV_LIBS}
)