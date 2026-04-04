cmake_minimum_required(VERSION 3.10)
project(game_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Подключение Conan
if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
    conan_basic_setup(TARGETS)
endif()

# Добавляем исполняемый файл
add_executable(game_server
    src/main.cpp
    src/json_loader.cpp
    src/request_handler.cpp
    src/model.cpp
    src/http_server.cpp
)

# Подключаем заголовочные файлы
target_include_directories(game_server PRIVATE src)

# Находим компоненты Boost
find_package(Boost REQUIRED COMPONENTS system log log_setup program_options)

# Для Windows добавляем библиотеку ws2_32
if(WIN32)
    target_link_libraries(game_server ws2_32 wsock32)
endif()

# Для Linux добавляем библиотеки
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(game_server 
        ${Boost_LIBRARIES}
        pthread
        rt
    )
endif()

# Если не используем Conan, ищем Boost вручную
if(NOT CONAN_EXPORTED)
    target_include_directories(game_server PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(game_server ${Boost_LIBRARIES})
endif()
