cmake_minimum_required(VERSION 3.28)
project(obs_control_deck_windows_cross LANGUAGES CXX)
if(NOT DECK_SOURCE_DIR OR NOT DECK_OBS_SDK OR NOT DECK_QT_SDK)
  message(FATAL_ERROR "Set DECK_SOURCE_DIR, DECK_OBS_SDK and DECK_QT_SDK")
endif()
list(PREPEND CMAKE_PREFIX_PATH "${DECK_QT_SDK}")
find_package(Qt6 REQUIRED COMPONENTS Widgets)
add_library(
  obs-control-deck
  MODULE
  "${DECK_SOURCE_DIR}/src/plugin-main.cpp"
  "${DECK_SOURCE_DIR}/src/obs-backend.cpp"
  "${DECK_SOURCE_DIR}/src/deck-model.cpp"
  "${DECK_SOURCE_DIR}/src/deck-widget.cpp"
)
target_compile_features(obs-control-deck PRIVATE cxx_std_17)
target_compile_definitions(obs-control-deck PRIVATE HAVE_OBSCONFIG_H)
target_include_directories(
  obs-control-deck
  PRIVATE "${DECK_SOURCE_DIR}/src" "${DECK_OBS_SDK}/include" "${DECK_OBS_SDK}/include/obs"
)
target_link_libraries(
  obs-control-deck
  PRIVATE "${DECK_OBS_SDK}/lib/obs.lib" "${DECK_OBS_SDK}/lib/obs-frontend-api.lib" Qt6::Widgets
)
set_target_properties(obs-control-deck PROPERTIES PREFIX "" SUFFIX ".dll")
