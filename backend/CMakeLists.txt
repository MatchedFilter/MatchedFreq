cmake_minimum_required(VERSION 3.25)

find_package(Git QUIET)

# ------------------------------------------------------------------------------
# Git Version Generation
# ------------------------------------------------------------------------------
set(RAW_VERSION "v0.0.0-dev")

if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} status --porcelain
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    OUTPUT_VARIABLE GIT_STATUS_OUT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(IS_DIRTY FALSE)
  if(GIT_STATUS_OUT)
    set(IS_DIRTY TRUE)
  endif()

  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    OUTPUT_VARIABLE GIT_HASH_OUT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    OUTPUT_VARIABLE GIT_TAG_OUT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(NOT GIT_TAG_OUT)
    set(GIT_TAG_OUT "v0.0.0")
  endif()

  if(NOT GIT_TAG_OUT MATCHES "^v")
    set(GIT_TAG_OUT "v${GIT_TAG_OUT}")
  endif()

  if(IS_DIRTY)
    set(RAW_VERSION "${GIT_TAG_OUT}-dirty_${GIT_HASH_OUT}")
  else()
    set(RAW_VERSION "${GIT_TAG_OUT}_${GIT_HASH_OUT}")
  endif()
endif()

set(MATCHEDFREQ_VERSION "${RAW_VERSION}")
message(STATUS "MatchedFreq Version: ${MATCHEDFREQ_VERSION}")

string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)" VERSION_MATCH "${GIT_TAG_OUT}")

if(VERSION_MATCH)
  set(PROJ_MAJOR "${CMAKE_MATCH_1}")
  set(PROJ_MINOR "${CMAKE_MATCH_2}")
  set(PROJ_PATCH "${CMAKE_MATCH_3}")
  set(PROJECT_VERSION_STRING "${PROJ_MAJOR}.${PROJ_MINOR}.${PROJ_PATCH}")
else()
  set(PROJECT_VERSION_STRING "0.0.0")
endif()

project(
  MatchedFreqServer
  VERSION ${PROJECT_VERSION_STRING}
  DESCRIPTION "AGPLv3 Analog Circuit Simulator Backend"
  LANGUAGES CXX
)

string(REGEX REPLACE "^v" "" SANITIZED_MATCHEDFREQ_VERSION "${MATCHEDFREQ_VERSION}")
set(PROJECT_NAME_WITH_VERSION ${PROJECT_NAME}_${SANITIZED_MATCHEDFREQ_VERSION})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json"
  )
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h
  @ONLY
)

# ------------------------------------------------------------------------------
# Third-Party Dependencies (Submodules inside backend/ThirdParty)
# ------------------------------------------------------------------------------

# 1. Asio
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio/asio/include")
  message(FATAL_ERROR "Asio submodule missing! Run 'git submodule update --init --recursive'.")
endif()

add_library(Asio INTERFACE)
target_include_directories(
  Asio
  SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/asio/asio/include
)
target_compile_definitions(
  Asio
  INTERFACE ASIO_STANDALONE
)

# 2. Crow Framework
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Crow/include")
  message(FATAL_ERROR "Crow submodule missing! Run 'git submodule update --init --recursive'.")
endif()

add_library(Crow INTERFACE)
target_include_directories(
  Crow
  SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Crow/include
)

# 3. spdlog (Header-only target)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog/include")
  message(FATAL_ERROR "spdlog submodule missing! Run 'git submodule update --init --recursive'.")
endif()

add_library(spdlog INTERFACE)
target_include_directories(
  spdlog
  SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog/include
)

# 4. Eigen
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/eigen/CMakeLists.txt")
  message(FATAL_ERROR "Eigen submodule missing! Run 'git submodule update --init --recursive'.")
endif()

add_subdirectory(ThirdParty/eigen EXCLUDE_FROM_ALL)

if(TARGET Eigen3::Eigen)
  # Resolve the real target behind the ALIAS
  get_target_property(REAL_EIGEN_TARGET Eigen3::Eigen ALIASED_TARGET)
  if(NOT REAL_EIGEN_TARGET)
    set(REAL_EIGEN_TARGET "Eigen3::Eigen")
  endif()

  get_target_property(EIGEN_INTERFACE_INCLUDES ${REAL_EIGEN_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
  if(EIGEN_INTERFACE_INCLUDES)
    set_target_properties(${REAL_EIGEN_TARGET} PROPERTIES
      INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${EIGEN_INTERFACE_INCLUDES}"
    )
  endif()
endif()

# ------------------------------------------------------------------------------
# Executable Setup
# ------------------------------------------------------------------------------
file(GLOB_RECURSE MATCHED_FREQ_SRCS src/*.cpp)
add_executable(${PROJECT_NAME_WITH_VERSION} ${MATCHED_FREQ_SRCS})

target_link_libraries(
  ${PROJECT_NAME_WITH_VERSION}
  PRIVATE Asio Crow spdlog Eigen3::Eigen
)

target_include_directories(
  ${PROJECT_NAME_WITH_VERSION}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# Strict Compiler Flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(
    ${PROJECT_NAME_WITH_VERSION}
    PRIVATE -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
  )
endif()

find_program(CLANG_TIDY_EXE NAMES "clang-tidy" "clang-tidy-22")
if(CLANG_TIDY_EXE)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
endif()

target_compile_definitions(
  ${PROJECT_NAME_WITH_VERSION}
  PRIVATE
    $<$<CONFIG:Debug>:MATCHEDFREQ_DEBUG>
    $<$<CONFIG:Release>:NDEBUG MATCHEDFREQ_RELEASE>
)
