# GoogleTest is downloaded on the first configure. That step needs a network.

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.18.0.tar.gz
  URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
)
# gtest's defaults: build gmock too, and install itself. We want neither.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(aerotest_tests
  version_test.cpp
  status_test.cpp
  message_test.cpp
  loopback_bus_test.cpp
  socketcan_bus_test.cpp
  fault_injector_test.cpp
  test_runner_test.cpp
)
target_link_libraries(aerotest_tests PRIVATE aerotest::aerotest GTest::gtest_main)
aerotest_set_warnings(aerotest_tests)

include(GoogleTest)
gtest_discover_tests(aerotest_tests
  PROPERTIES LABELS "step1"
  DISCOVERY_TIMEOUT 30
)

# Separate binary so the kernel-backed tests can carry their own ctest label.
# They still compile in CI. The test presets exclude the label, and the tests
# themselves stay DISABLED_ until task T7 is real.
add_executable(aerotest_vcan_tests
  vcan_bus_test.cpp
)
target_link_libraries(aerotest_vcan_tests PRIVATE aerotest::aerotest GTest::gtest_main)
aerotest_set_warnings(aerotest_vcan_tests)
gtest_discover_tests(aerotest_vcan_tests
  PROPERTIES LABELS "needs-vcan"
  DISCOVERY_TIMEOUT 30
)

add_test(NAME cli.version COMMAND $<TARGET_FILE:aerotest_cli> --version)
set_tests_properties(cli.version PROPERTIES
  LABELS "step1"
  PASS_REGULAR_EXPRESSION "aerotest 0\\.1\\.0"
)
add_test(NAME cli.help COMMAND $<TARGET_FILE:aerotest_cli> --help)
set_tests_properties(cli.help PROPERTIES
  LABELS "step1"
  PASS_REGULAR_EXPRESSION "docs/step-1.md"
)
add_test(NAME cli.no_args COMMAND $<TARGET_FILE:aerotest_cli>)
set_tests_properties(cli.no_args PROPERTIES
  LABELS "step1"
  PASS_REGULAR_EXPRESSION "docs/step-1.md"
)
