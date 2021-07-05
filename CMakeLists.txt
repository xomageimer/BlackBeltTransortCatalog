cmake_minimum_required(VERSION 3.17)
project(TransportCatalog)

set(CMAKE_CXX_STANDARD 20)

set(PROJECT_SOURCES json/json.cpp graph/graph.h graph/router.h src/data_manager.cpp src/route_manager.cpp src/render_manager.cpp src/distance.cpp src/requests.cpp src/responses.cpp xml/xml.cpp svg/svg.cpp)
set(PROJECT_HEADERS json/json.h src/headers/ranges.h src/headers/responses.h src/headers/data_manager.h src/headers/route_manager.h src/headers/render_manager.h src/headers/distance.h src/headers/requests.h xml/xml.h svg/svg.h)

add_executable(TransportCatalog src/main.cpp ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_include_directories(TransportCatalog PRIVATE src/headers json graph xml svg)