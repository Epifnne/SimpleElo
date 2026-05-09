add_library(simpleelo_shared
  src/elo/eloCalculator.cpp
  src/protocol/jsonCodec.cpp
)

target_include_directories(simpleelo_shared
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(simpleelo_shared
  PUBLIC
    nlohmann_json::nlohmann_json
)
