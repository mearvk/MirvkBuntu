cmake_minimum_required(VERSION 3.20)
project(MirvkBuntuInstaller VERSION 1.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mirvkbuntu-installer
  main.cpp
  src/Installer.cpp
  src/Platform.cpp
  src/Storage.cpp
  src/SystemConfig.cpp
  src/Bootloader.cpp
  src/Accounts.cpp
  src/Network.cpp
  src/Payload.cpp
  src/Locale.cpp
)
target_include_directories(mirvkbuntu-installer PRIVATE include)
target_compile_definitions(mirvkbuntu-installer PRIVATE MIRVKBUNTU_VERSION="${PROJECT_VERSION}")
if(WIN32)
  target_link_libraries(mirvkbuntu-installer PRIVATE advapi32 userenv)
endif()
install(TARGETS mirvkbuntu-installer RUNTIME DESTINATION bin BUNDLE DESTINATION .)

include(CPack)
set(CPACK_PACKAGE_NAME "MirvkBuntu-Installer")
set(CPACK_PACKAGE_VENDOR "MEARVK LLC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MirvkBuntu operating-system installer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "MEARVK LLC")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MirvkBuntu")
set(CPACK_PACKAGE_FILE_NAME "MirvkBuntu-Installer-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
if(WIN32)
  set(CPACK_GENERATOR "NSIS;ZIP")
  set(CPACK_NSIS_DISPLAY_NAME "MirvkBuntu Installer")
  set(CPACK_NSIS_PACKAGE_NAME "MirvkBuntu Installer")
  set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
elseif(APPLE)
  set(CPACK_GENERATOR "productbuild;DragNDrop")
else()
  set(CPACK_GENERATOR "TGZ;DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "MEARVK LLC")
  set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
endif()
