cmake_minimum_required(VERSION 3.10)

project(ProcessCommunication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SENDER_SOURCES Sender.cpp)
set(RECEIVER_SOURCES Receiver.cpp)

add_executable(Sender ${SENDER_SOURCES})
add_executable(Receiver ${RECEIVER_SOURCES})

option(BUILD_SENDER "Build and run the Sender program" OFF)
option(BUILD_RECEIVER "Build and run the Receiver program" OFF)

if (BUILD_SENDER)
    set(RUN_TARGET Sender)
elseif (BUILD_RECEIVER)
    set(RUN_TARGET Receiver)
else()
    message(WARNING "No target specified. Use -DBUILD_SENDER=ON or -DBUILD_RECEIVER=ON to select the target.")
endif()

if (RUN_TARGET)
    add_custom_target(run
        COMMAND ${CMAKE_BINARY_DIR}/${RUN_TARGET}
        DEPENDS ${RUN_TARGET}
        COMMENT "Running ${RUN_TARGET}...")
endif()
