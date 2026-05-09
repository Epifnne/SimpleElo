add_executable(simpleelo_tests
  jsonCodecTests.cpp
)

target_link_libraries(simpleelo_tests
  PRIVATE
    simpleelo_shared
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(simpleelo_tests)
