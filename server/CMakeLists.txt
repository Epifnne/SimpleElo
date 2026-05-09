add_executable(simpleelo_server
  src/main.cpp
  src/config/serverConfig.cpp
  src/api/actionRouter.cpp
  src/engine/serverEngineCore.cpp
  src/engine/serverEngineStorage.cpp
  src/engine/serverEngineRoomUser.cpp
  src/engine/serverEngineMatch.cpp
  src/service/eloService.cpp
  src/service/voteService.cpp
  src/repo/sqliteRepo.cpp
)

target_link_libraries(simpleelo_server
  PRIVATE
    simpleelo_shared
)

set(SIMPLEELO_LOCAL_BOOST_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/boost")
if(EXISTS "${SIMPLEELO_LOCAL_BOOST_DIR}/libs/asio/include/boost/asio.hpp")
  file(GLOB SIMPLEELO_BOOST_INCLUDE_DIRS
    "${SIMPLEELO_LOCAL_BOOST_DIR}/libs/*/include"
    "${SIMPLEELO_LOCAL_BOOST_DIR}/libs/*/*/include"
  )
  target_include_directories(simpleelo_server PRIVATE ${SIMPLEELO_BOOST_INCLUDE_DIRS})
  target_compile_definitions(simpleelo_server PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
  message(STATUS "simpleelo_server uses vendored Boost headers from ${SIMPLEELO_LOCAL_BOOST_DIR}")
else()
  cmake_policy(SET CMP0167 OLD)
  find_package(Boost REQUIRED COMPONENTS system)
  target_link_libraries(simpleelo_server PRIVATE Boost::system)
  message(STATUS "simpleelo_server uses system Boost")
endif()

target_include_directories(simpleelo_server
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(simpleelo_server
  PRIVATE
    SIMPLEELO_API_SERVER
)

if(WIN32)
  target_link_libraries(simpleelo_server PRIVATE ws2_32)
endif()
