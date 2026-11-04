set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(CMAKE_CXX_COMPILER "$ENV{DECK_LLVM_ROOT}/bin/clang-cl")
set(CMAKE_LINKER "$ENV{DECK_LLD_ROOT}/bin/lld-link")
set(CMAKE_AR "$ENV{DECK_LLVM_ROOT}/bin/llvm-lib")
set(CMAKE_RC_COMPILER "$ENV{DECK_LLVM_ROOT}/bin/llvm-rc")
set(CMAKE_MT "$ENV{DECK_LLVM_ROOT}/bin/llvm-mt")
set(_xwin "$ENV{DECK_XWIN_ROOT}")
set(_flags "--target=x86_64-pc-windows-msvc -Wno-unused-command-line-argument")
foreach(
  dir
  crt/include
  sdk/include/ucrt
  sdk/include/shared
  sdk/include/um
  sdk/include/winrt
)
  string(APPEND _flags " /imsvc\"${_xwin}/${dir}\"")
endforeach()
set(CMAKE_CXX_FLAGS_INIT "${_flags}")
set(
  _libs
  "/libpath:\"${_xwin}/crt/lib/x86_64\" /libpath:\"${_xwin}/sdk/lib/ucrt/x86_64\" /libpath:\"${_xwin}/sdk/lib/um/x86_64\""
)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_libs}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_libs}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_libs}")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)
