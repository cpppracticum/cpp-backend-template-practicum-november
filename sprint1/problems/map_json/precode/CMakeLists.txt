cmake_minimum_required(VERSION 3.10)
project(game_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Подключение Conan
if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

# Добавляем исполняемый файл (только .cpp файлы, .h подключаются автоматически)
add_executable(game_server
    src/main.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/model.cpp
)

# Подключаем заголовочные файлы
target_include_directories(game_server PRIVATE src)

# Для Windows добавляем библиотеку ws2_32
if(WIN32)
    target_link_libraries(game_server ws2_32 wsock32)
endif()

# Если не используем Conan, ищем Boost вручную
if(NOT CONAN_EXPORTED)
    find_package(Boost REQUIRED COMPONENTS system)
    target_include_directories(game_server PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(game_server ${Boost_LIBRARIES})
endif()
