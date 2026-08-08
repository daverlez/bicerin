add_executable(core_tests
        cpu_test.cpp
)

target_link_libraries(core_tests PRIVATE
        GTest::gtest_main
        bicerin_core
)

add_test(NAME CoreTests COMMAND core_tests)