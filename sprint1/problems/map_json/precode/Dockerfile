# Этап сборки (build)
FROM gcc:11.3 as build

# Установка зависимостей
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Копируем conanfile.txt и устанавливаем зависимости
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing

# Копируем исходный код и CMakeLists.txt
COPY ./src /app/src
COPY CMakeLists.txt /app/

# Сборка проекта
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

# Этап запуска (run)
FROM ubuntu:22.04 as run

# Создаём непривилегированного пользователя
RUN groupadd -r www && useradd -r -g www www
USER www

# Копируем собранный сервер из этапа build
COPY --from=build /app/build/bin/game_server /app/
# Копируем данные
COPY ./data /app/data

# Запускаем сервер
ENTRYPOINT ["/app/game_server", "/app/data/config.json"]
