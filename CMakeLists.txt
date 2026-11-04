cmake_minimum_required(VERSION 3.28...3.30)
set(PLUGIN_BUILD_NUMBER 1 CACHE STRING "Build number")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)
set(CONTROL_DECK_LOCAL_SDK "" CACHE PATH "Existing OBS SDK prefix (skip dependency downloads)")

include(compilerconfig)
if(CONTROL_DECK_LOCAL_SDK)
  get_filename_component(CONTROL_DECK_LOCAL_SDK "${CONTROL_DECK_LOCAL_SDK}" ABSOLUTE)
  list(
    PREPEND CMAKE_PREFIX_PATH
    "${CONTROL_DECK_LOCAL_SDK}"
    "${CONTROL_DECK_LOCAL_SDK}/obs-deps-qt6-2025-07-11-universal"
  )
  list(PREPEND CMAKE_FRAMEWORK_PATH "${CONTROL_DECK_LOCAL_SDK}/Frameworks")
  set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")
else()
  include(defaults)
endif()
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

if(CONTROL_DECK_LOCAL_SDK AND APPLE)
  add_library(OBS::libobs SHARED IMPORTED)
  set_target_properties(
    OBS::libobs
    PROPERTIES
      IMPORTED_LOCATION "${CONTROL_DECK_LOCAL_SDK}/Frameworks/libobs.framework/Versions/A/libobs"
      INTERFACE_INCLUDE_DIRECTORIES "${CONTROL_DECK_LOCAL_SDK}/Frameworks/libobs.framework/Headers"
  )
  add_library(OBS::obs-frontend-api SHARED IMPORTED)
  set_target_properties(
    OBS::obs-frontend-api
    PROPERTIES
      IMPORTED_LOCATION "${CONTROL_DECK_LOCAL_SDK}/lib/obs-frontend-api.dylib"
      INTERFACE_INCLUDE_DIRECTORIES "${CONTROL_DECK_LOCAL_SDK}/include/obs"
  )
else()
  find_package(libobs REQUIRED)
  find_package(obs-frontend-api REQUIRED)
endif()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(NOT ENABLE_FRONTEND_API OR NOT ENABLE_QT)
  message(FATAL_ERROR "Control Deck requires ENABLE_FRONTEND_API and ENABLE_QT")
endif()
if(ENABLE_FRONTEND_API)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
  if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(_gl WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    if(_gl)
      list(FILTER _gl EXCLUDE REGEX "AGL\\.framework")
      set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES INTERFACE_LINK_LIBRARIES "${_gl}")
    endif()
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_library(deck-ui STATIC src/deck-model.cpp src/deck-widget.cpp)
target_link_libraries(deck-ui PUBLIC Qt6::Core Qt6::Widgets)
target_include_directories(deck-ui PUBLIC src)
set_target_properties(deck-ui PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.cpp src/obs-backend.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE deck-ui)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
if(APPLE AND CONTROL_DECK_LOCAL_SDK)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-" XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
  )
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(deck-tests tests/deck-tests.cpp)
  target_link_libraries(deck-tests PRIVATE deck-ui)
  add_test(NAME deck-tests COMMAND deck-tests -platform offscreen)
  add_executable(deck-preview tests/deck-preview.cpp)
  target_link_libraries(deck-preview PRIVATE deck-ui)
endif()
