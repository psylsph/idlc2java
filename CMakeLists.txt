cmake_minimum_required(VERSION 3.15)
project(idlc_java VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

set(CYCLONEDDS_ROOT "/opt/cyclonedds" CACHE PATH "Path to CycloneDDS installation")

include_directories(
    ${CYCLONEDDS_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

link_directories(${CYCLONEDDS_ROOT}/lib)

set(PLUGIN_SOURCES
    src/generator.c
    src/java_generator.c
    src/java_type.c
    src/java_record.c
    src/package_resolver.c
    src/annotation.c
    src/string_builder.c
)

add_library(idlc_java SHARED ${PLUGIN_SOURCES})

set_target_properties(idlc_java PROPERTIES
    PREFIX ""
    OUTPUT_NAME "idlc_java"
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_link_libraries(idlc_java
    cycloneddsidl
    dl
)

install(TARGETS idlc_java
    LIBRARY DESTINATION ${CYCLONEDDS_ROOT}/lib/idlc
)

enable_testing()
add_subdirectory(tests)
