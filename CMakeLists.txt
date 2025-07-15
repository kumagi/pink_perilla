cmake_minimum_required(VERSION 3.10)
include(FetchContent)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0074 NEW)

set(FETCHCONTENT_QUIET OFF)
set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_ENABLE_INSTALL ON)

project(PinkPerilla)

find_package(ZLIB REQUIRED)

# Option to enable LeakSanitizer.
option(USE_LEAK_SANITIZER "Enable LeakSanitizer" OFF)

if(USE_LEAK_SANITIZER)
  message(STATUS "LeakSanitizer is enabled.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=leak")
endif()

FetchContent_Declare(
  absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20230125.3
)
FetchContent_MakeAvailable(absl)

set(protobuf_BUILD_TESTS OFF)
FetchContent_Declare(
  protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v23.4
)
FetchContent_MakeAvailable(protobuf)
find_package(Protobuf REQUIRED)

FetchContent_GetProperties(protobuf)
# Declare and fetch GoogleTest for our tests
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main # Using main, consider pinning to a specific tag for stability
)
FetchContent_MakeAvailable(googletest)

# Find Readline
find_path(READLINE_INCLUDE_DIR readline/readline.h)
find_library(READLINE_LIBRARY readline)
set(READLINE_LIBRARIES ${READLINE_LIBRARY})
set(READLINE_INCLUDE_DIRS ${READLINE_INCLUDE_DIR})
if (NOT READLINE_INCLUDE_DIR OR NOT READLINE_LIBRARY)
    message(FATAL_ERROR "Readline not found. Please install it (e.g., 'brew install readline').")
endif()

# Add a rule to compile the .proto file
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB_RECURSE SUBSTRAIT_PROTO_FILES "external/substrait/proto/substrait/*.proto")
set(PROTO_FILES
  ${SUBSTRAIT_PROTO_FILES}
)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/proto_generated)

protobuf_generate(OUT_VAR substrait_cpp
  LANGUAGE cpp
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS
    external/substrait/proto
    ${protobuf_SOURCE_DIR}/src
  PROTOC_OUT_DIR proto_generated
)

add_library(substrait_files STATIC ${substrait_cpp})
target_include_directories(substrait_files PUBLIC
    ${PROJECT_BINARY_DIR}/proto_generated
    ${protobuf_SOURCE_DIR}/src
)
target_link_libraries(substrait_files PUBLIC protobuf::libprotobuf)

set(pink_perilla_lib_srcs
  src/pink_perilla.cpp
  src/sql_parser.cpp
  src/substrait_converter.cpp
)
add_library(pink_perilla_lib STATIC ${pink_perilla_lib_srcs})
set_target_properties(pink_perilla_lib PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(pink_perilla_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# By linking to substrait_files, pink_perilla_lib will transitively get the
# necessary include directories and libraries from both substrait_files and protobuf.
target_link_libraries(pink_perilla_lib PUBLIC
  substrait_files
  absl::strings
  absl::log
  absl::status
  absl::statusor
)

add_executable(pink_perilla src/main.cpp)
set_target_properties(pink_perilla PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
add_dependencies(pink_perilla substrait_files)

target_include_directories(pink_perilla PUBLIC SYSTEM
  ${CMAKE_BINARY_DIR}/proto_generated/src
  ${CMAKE_BINARY_DIR}/proto_generated
  ${READLINE_INCLUDE_DIRS}
)


target_link_libraries(pink_perilla
  PRIVATE
  pink_perilla_lib
  substrait_files
  ${READLINE_LIBRARIES}
)

target_compile_features(pink_perilla PRIVATE cxx_std_17)

enable_testing()
include(GoogleTest)

foreach(test_name IN ITEMS insert_test update_test delete_test select_test join_test misc_features_test robustness_test ddl_test table_definition_test parser_test)
    add_executable(
        ${test_name}
        tests/${test_name}.cpp
    )
    set_target_properties(${test_name} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
    target_link_libraries(
        ${test_name}
        GTest::gtest_main
        pink_perilla_lib
        substrait_files
            absl::log
        ${READLINE_LIBRARIES}
    )
    target_include_directories(${test_name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Add a linting target for CMake files
find_program(CMAKE_LINT_COMMAND cmakelint)
if(CMAKE_LINT_COMMAND)
  add_custom_target(lint
    COMMAND ${CMAKE_LINT_COMMAND} ${CMAKE_SOURCE_DIR}/CMakeLists.txt
    COMMENT "Running cmakelint on root CMakeLists.txt"
  )
else()
  add_custom_target(lint
    COMMAND ${CMAKE_COMMAND} -E echo "cmakelint not found. Skipping linting. (Install with: pip install cmakelint)"
  )
endif()
