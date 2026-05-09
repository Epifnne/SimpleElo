set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/ThirdParty/imgui)

add_library(simpleelo_imgui
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(simpleelo_imgui
  PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(simpleelo_imgui
  PUBLIC
    glfw
)

add_executable(simpleelo_client
  src/main.cpp
  src/app/guiApp.cpp
  src/app/pageHelpers.cpp
  src/app/screens.cpp
  src/pages/connectPage.cpp
  src/pages/authPage.cpp
  src/pages/lobbyPage.cpp
  src/pages/roomPage.cpp
  src/pages/profilePage.cpp
  src/pages/userInspectPage.cpp
  src/components/connectSections.cpp
  src/components/authSections.cpp
  src/components/lobbySections.cpp
  src/components/profileSections.cpp
  src/components/userInspectSections.cpp
  src/components/roomSections.cpp
  src/components/roomDisplaySection.cpp
  src/components/roomResultSection.cpp
  src/uiCommon.cpp
  src/net/httpClient.cpp
)

target_link_libraries(simpleelo_client
  PRIVATE
    simpleelo_shared
    simpleelo_imgui
)

target_include_directories(simpleelo_client
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_compile_definitions(simpleelo_client
  PRIVATE
    SIMPLEELO_API_CLIENT
)

if(MSVC)
  target_compile_options(simpleelo_client PRIVATE /utf-8)
else()
  target_compile_options(simpleelo_client PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

if(WIN32)
  target_link_libraries(simpleelo_client PRIVATE ws2_32 opengl32)
endif()
