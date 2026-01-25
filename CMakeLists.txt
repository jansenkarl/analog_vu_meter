cmake_minimum_required(VERSION 3.20)
project(AnalogVUMeterQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSEAUDIO REQUIRED libpulse)

add_executable(analog_vu_meter
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/StereoVUMeterWidget.cpp
    src/StereoVUMeterWidget.h
    src/AudioCapture.cpp
    src/AudioCapture.h
    src/VUBallistics.cpp
    src/VUBallistics.h
)

# Include directories for PulseAudio
target_include_directories(analog_vu_meter PRIVATE
    ${PULSEAUDIO_INCLUDE_DIRS}
)

# Link Qt and PulseAudio
target_link_libraries(analog_vu_meter PRIVATE
    Qt6::Widgets
    ${PULSEAUDIO_LIBRARIES}
)

# PulseAudio sometimes needs extra compiler flags
target_compile_options(analog_vu_meter PRIVATE ${PULSEAUDIO_CFLAGS_OTHER})
