cmake_minimum_required(VERSION 3.1...3.23)
set(CMAKE_CXX_STANDARD 20)

project(
  moser
  VERSION 1.1
  LANGUAGES CXX
)


IF (WIN32)

  include_directories(include include/win32/xaml  ${_VCPKG_INSTALLED_DIR}/${_MSVC_CXX_ARCHITECTURE_FAMILY}-${Z_VCPKG_TARGET_TRIPLET_PLAT}/include)

  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/W4 /wd4505)
  set(VCPKG_CRT_LINKAGE dynamic) 

  find_package(nlohmann_json CONFIG REQUIRED)
  find_package(libffi CONFIG REQUIRED)
  link_directories(${CMAKE_SOURCE_DIR}/win32)
  set(LIBS "libffi" "WindowsApp.lib"  )

else()

  include_directories(include include/win32/xaml)
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)

  find_package(nlohmann_json CONFIG REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBFFI REQUIRED libffi) 

  set(LIBS "${LIBFFI_LIBRARIES}" )
endif()


set(SOURCES
  src/main.cpp 
  src/chunk.cpp 
  src/debug.cpp 
  src/value.cpp 
  src/scanner.cpp
  src/parser.cpp
  src/object.cpp
  src/compiler.cpp 
  src/vm.cpp 
  src/gc.cpp
  src/common.cpp
  src/serialize.cpp  
  src/native.cpp
  src/foreign.cpp
  src/lib.cpp
  src/must.cpp
  src/win32/dlopen.cpp
  src/win32/uni.cpp
  src/win32/tinfo.cpp
  src/win32/comobj.cpp
  src/win32/disp.cpp
)

IF (WIN32)
	set(SOURCES ${SOURCES} src/win32/xmoser.rc moser.manifest)
endif()

add_executable(moser 
  ${SOURCES}
 )
  
target_link_libraries(moser PRIVATE ${LIBS})

file(COPY lib DESTINATION  ${CMAKE_BINARY_DIR} )


if(UNIX) 

############################################
# install rules
############################################

  install(TARGETS moser DESTINATION bin)
  install(DIRECTORY "lib" DESTINATION "lib/moser/" )

  
############################################
# Create and install package configuration 
# and version files.
############################################

configure_file(
   ${moser_SOURCE_DIR}/moser-config.cmake.in
   ${moser_BINARY_DIR}/moser-config.cmake @ONLY)

configure_file(
   ${moser_SOURCE_DIR}/moser-config-version.cmake.in
   ${moser_BINARY_DIR}/moser-config-version.cmake @ONLY)

#install(FILES ${moser_BINARY_DIR}/moser-config.cmake
#         ${moser_BINARY_DIR}/moser-config-version.cmake
#         DESTINATION lib/cryptoneat-${VERSION})

############################################
# packaging
############################################

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
include(package)
           

endif()
