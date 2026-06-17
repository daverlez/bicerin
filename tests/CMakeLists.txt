find_package(GTest CONFIG REQUIRED)

add_executable(unit_tests
        core/cpu_test.cpp
)

target_link_libraries(unit_tests PRIVATE
        GTest::gtest_main
        core_lib
)

add_test(NAME AllTests COMMAND unit_tests)