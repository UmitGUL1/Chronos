cmake_minimum_required(VERSION 3.24)
project(ChronOS VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(CHRONOS_BUILD_TESTS "Build GoogleTest tests" ON)
option(CHRONOS_BUILD_DESKTOP "Build Qt Quick desktop" OFF)

include(FetchContent)
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

add_library(chronos-core src/core/build_info.cpp src/core/model.cpp
  src/scheduling/scheduler.cpp src/simulation/engine.cpp)
target_include_directories(chronos-core PUBLIC include)
add_library(chronos-io src/configuration/io.cpp)
target_link_libraries(chronos-io PUBLIC chronos-core nlohmann_json::nlohmann_json)
target_include_directories(chronos-io PUBLIC include)
if(MSVC)
  target_compile_options(chronos-core PRIVATE /W4 /permissive-)
else()
  target_compile_options(chronos-core PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(chronos-cli apps/cli/main.cpp)
target_link_libraries(chronos-cli PRIVATE chronos-io)

if(CHRONOS_BUILD_TESTS)
  include(CTest)
  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(googletest)
  add_executable(chronos-tests tests/unit/build_info_test.cpp tests/unit/engine_test.cpp
    tests/integration/io_test.cpp tests/integration/scenario_test.cpp)
  target_compile_definitions(chronos-tests PRIVATE CHRONOS_SCENARIO_DIR="${CMAKE_CURRENT_SOURCE_DIR}/scenarios")
  find_package(Threads REQUIRED)
  target_link_libraries(chronos-tests PRIVATE chronos-io GTest::gtest_main Threads::Threads)
  include(GoogleTest)
  gtest_discover_tests(chronos-tests)
  add_test(NAME cli-help COMMAND chronos-cli --help)
  add_test(NAME cli-reference COMMAND chronos-cli --scenario ${CMAKE_CURRENT_SOURCE_DIR}/scenarios/basic-fcfs.json)
endif()

if(CHRONOS_BUILD_DESKTOP)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)
  qt_standard_project_setup(REQUIRES 6.5)
  find_package(Threads REQUIRED)
  qt_add_executable(chronos-desktop apps/desktop/main.cpp apps/desktop/controller.cpp apps/desktop/controller.hpp)
  # Keep diagnostics visible to CTest on Windows development/CI builds.
  set_target_properties(chronos-desktop PROPERTIES WIN32_EXECUTABLE FALSE)
  qt_add_qml_module(chronos-desktop URI ChronOS VERSION 1.0
    QML_FILES ui/qml/Main.qml ui/qml/Timeline.qml ui/qml/WaitGraph.qml)
  target_link_libraries(chronos-desktop PRIVATE chronos-io Qt6::Quick Qt6::QuickControls2 Threads::Threads)
  if(CHRONOS_BUILD_TESTS)
    add_test(NAME desktop-smoke COMMAND chronos-desktop --smoke)
    set_tests_properties(desktop-smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
  endif()
  install(TARGETS chronos-desktop RUNTIME DESTINATION bin)
endif()

add_executable(chronos-benchmark benchmarks/main.cpp)
target_link_libraries(chronos-benchmark PRIVATE chronos-core)
install(TARGETS chronos-cli RUNTIME DESTINATION bin)
install(DIRECTORY scenarios DESTINATION share/chronos)

option(CHRONOS_SANITIZERS "Enable address/undefined sanitizers (GNU/Clang only)" OFF)
if(CHRONOS_SANITIZERS AND NOT MSVC)
  foreach(target chronos-core chronos-io chronos-cli chronos-benchmark)
    target_compile_options(${target} PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PUBLIC -fsanitize=address,undefined)
  endforeach()
endif()

