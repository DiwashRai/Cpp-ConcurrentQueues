cmake_minimum_required(VERSION 3.28)
project(Cpp_ConcurrentQueues)

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SANITIZE_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZE_FLAGS}")
endif()

find_package(GTest REQUIRED)
find_package(Boost COMPONENTS circular_buffer thread callable_traits REQUIRED)

add_executable(queue_tests
        ConcurrentQueueTests.cpp
        random_num.h
        UnboundedBufferDequeBased.h
        TwoLockUnboundedQueue.h
        ConcurrentQueueConcept.h
        BoundedBufferRingBased.h
        RingBuffer.h
        BoostBoundedBufferRingBased.h
        CrashingConcurrentSumTest.cpp
        MoodeyCamelQueues.h
        Barrier.h)

target_link_libraries(queue_tests
        GTest::gtest
        GTest::gtest_main
        Boost::circular_buffer
        Boost::thread
        Boost::callable_traits
)

add_executable(custom_benchmarks
        concurrent_queue_benchmarks.cpp
        UnboundedBufferDequeBased.h
        TwoLockUnboundedQueue.h
        ConcurrentQueueConcept.h
        BoundedBufferRingBased.h
        RingBuffer.h
        BoostBoundedBufferRingBased.h
        MoodeyCamelQueues.h)

target_link_libraries(custom_benchmarks
        Boost::circular_buffer
        Boost::thread
        Boost::callable_traits
)
