cmake_minimum_required(VERSION 3.28)
project(Cpp_ConcurrentQueues)

message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Compiler Flags: ${CMAKE_CXX_FLAGS}")

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wno-interference-size)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
endif()

find_package(GTest REQUIRED)
find_package(Boost COMPONENTS circular_buffer thread callable_traits lockfree REQUIRED)

add_executable(queue_tests
        ConcurrentQueueTests.cpp
        random_num.h
        MutexDequeQueue.h
        StdAtomicSPSCQueue.h
        ConcurrentQueueConcept.h
        MutexRingBufferQueue.h
        RingBuffer.h
        MutexBoostRingBufferQueue.h
        CrashingConcurrentSumTest.cpp
        MoodeyCamelQueueAdapters.h
        Barrier.h
        AtomicQueueAdapters.h
        StdAtomicMPMCQueue.h
        alpha_spsc.h)

target_link_libraries(queue_tests
        GTest::gtest
        GTest::gtest_main
        Boost::circular_buffer
        Boost::thread
        Boost::callable_traits
)

add_executable(custom_benchmarks
        concurrent_queue_benchmarks.cpp
        MutexDequeQueue.h
        StdAtomicSPSCQueue.h
        ConcurrentQueueConcept.h
        MutexRingBufferQueue.h
        RingBuffer.h
        MutexBoostRingBufferQueue.h
        MoodeyCamelQueueAdapters.h
        AtomicQueueAdapters.h
        BoostLockFreeAdapters.h
        MutexListQueue.h
        StdAtomicMPMCQueue.h
        alpha_spsc.h)

target_link_libraries(custom_benchmarks
        Boost::circular_buffer
        Boost::thread
        Boost::callable_traits
        Boost::lockfree
)
