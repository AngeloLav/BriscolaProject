cmake_minimum_required(VERSION 3.16)

project(BriscolaProject)

find_package(OpenCV REQUIRED)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/model
)

add_executable(
    briscola
    src/main.cpp
    src/detector.cpp
    src/JsonReader.cpp
)

target_link_libraries(
    briscola
    ${OpenCV_LIBS}
)