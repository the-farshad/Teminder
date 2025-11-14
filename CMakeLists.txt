# Set the minimum CMake version and project name
cmake_minimum_required(VERSION 3.16)
project(Teminder VERSION 1.0)

# Set the C++ standard to 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

# Include the FetchContent module to download dependencies
include(FetchContent)

# 1. Fetch and configure FTXUI
FetchContent_Declare(
    FTXUI
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(FTXUI)

# 2. Fetch and configure SQLiteCpp
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.3
)
set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # Don't build examples
set(SQLITECPP_BUILD_TEST OFF CACHE BOOL "" FORCE) # Don't build tests
FetchContent_MakeAvailable(SQLiteCpp)

# 3. Fetch and configure nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# 4. Fetch and configure cpr
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.12.0
)
FetchContent_MakeAvailable(cpr)

# 5. Fetch and configure hiredis
FetchContent_Declare(
    hiredis
    GIT_REPOSITORY https://github.com/redis/hiredis.git
    GIT_TAG v1.3.0
)
set(DISABLE_TESTS ON CACHE BOOL "" FORCE) # Avoid hiredis tests
FetchContent_MakeAvailable(hiredis)

# 6. Ad our main executable
add_executable(
    Teminder
    main.cpp
    DatabaseManager.cpp
    ConfigManager.cpp
    AIAssistant.cpp
    TaskListView.cpp
    RedisManager.cpp
    GoogleSheets.cpp
)

# 7. Link all libraries to our executable
target_link_libraries(
    Teminder
    PRIVATE

    # FXUI libs
    ftxui::screen
    ftxui::component

    # SQLiteCpp lib
    SQLiteCpp

    # JSON lib
    nlohmann_json::nlohmann_json

    # HTTP (cpr) lib
    cpr::cpr

    # Redis (hiredis) lib
    hiredis::hiredis
)

# 8. Add include directories for hiredis

if (DEFINED hiredis_SOURCE_DIR)
  target_include_directories(
      Teminder
      PRIVATE
      ${hiredis_SOURCE_DIR}
  )
endif()
