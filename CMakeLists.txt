cmake_minimum_required(VERSION 3.16)

# Projenin adı, versiyonu ve kullanılan dil
project(SensorSystem
    VERSION 1.0.0
    DESCRIPTION "Multithreaded Sensor Data Processing System"
    LANGUAGES CXX
)

# C++17 standardını zorunlu kıl
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)    # Bu standart ZORUNLU, düşük versiyona geçmesin
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # IDE'ler için derleme komutlarını dışa aktar

# ══════════════════════════════════════════════════════════════
# Ana Çalıştırılabilir Dosya (Executable)
# ══════════════════════════════════════════════════════════════

# include/ klasöründeki header dosyalarını bul
# GLOB_RECURSE: alt klasörlerde de arar
file(GLOB_RECURSE HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.h")

# src/ klasöründeki kaynak dosyalarını bul (main.cpp hariç)
file(GLOB_RECURSE SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/main.cpp")

# Ana programı oluştur
add_executable(sensor_system
    src/main.cpp
    ${SOURCE_FILES}
)

# Header dosyalarının bulunduğu klasörü belirt
# PUBLIC: Bu projeyi kullanan diğer projeler de bu header'ları görebilir
target_include_directories(sensor_system PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Linux'ta thread kütüphanesini bağla
# Threads::Threads: CMake'in platform-bağımsız thread bulma mekanizması
find_package(Threads REQUIRED)
target_link_libraries(sensor_system PRIVATE Threads::Threads)

# ══════════════════════════════════════════════════════════════
# Test Yapılandırması (GoogleTest)
# ══════════════════════════════════════════════════════════════

# FetchContent: Dışarıdan kütüphane indirmek için CMake modülü
include(FetchContent)

# GoogleTest'i GitHub'dan indir
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Windows'ta runtime library çakışmasını önle
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# CTest'i etkinleştir (ctest komutu ile test çalıştırma)
enable_testing()

# ── Queue Testleri ──
add_executable(test_queue tests/test_queue.cpp ${SOURCE_FILES})
target_include_directories(test_queue PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_queue PRIVATE GTest::gtest_main Threads::Threads)
add_test(NAME QueueTests COMMAND test_queue)

# ── Sensor Testleri ──
add_executable(test_sensor tests/test_sensor.cpp ${SOURCE_FILES})
target_include_directories(test_sensor PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_sensor PRIVATE GTest::gtest_main Threads::Threads)
add_test(NAME SensorTests COMMAND test_sensor)

# ── Processor Testleri ──
add_executable(test_processor tests/test_processor.cpp ${SOURCE_FILES})
target_include_directories(test_processor PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_processor PRIVATE GTest::gtest_main Threads::Threads)
add_test(NAME ProcessorTests COMMAND test_processor)
