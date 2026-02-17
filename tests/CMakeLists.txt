cmake_minimum_required(VERSION 3.15)

find_package(PkgConfig REQUIRED)

set(CYCLONEDDS_ROOT "/opt/cyclonedds" CACHE PATH "Path to CycloneDDS installation")

include_directories(
    ${CYCLONEDDS_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

link_directories(${CYCLONEDDS_ROOT}/lib)

add_executable(run_tests run_tests.c)

target_link_libraries(run_tests
    cycloneddsidl
)

add_test(NAME basic_generation COMMAND run_tests)
