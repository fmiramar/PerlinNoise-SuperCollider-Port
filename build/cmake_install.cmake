# Install script for directory: /Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/build/stage")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/PerlinNoise" TYPE MODULE FILES "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/build/PerlinNoise.scx")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/PerlinNoise/PerlinNoise.scx" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/PerlinNoise/PerlinNoise.scx")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/PerlinNoise/PerlinNoise.scx")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/PerlinNoise/Classes" TYPE FILE FILES
    "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/Classes/PerlinNoise.sc"
    "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/Classes/PerlinNoise2D.sc"
    "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/Classes/PerlinNoise3D.sc"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/PerlinNoise/HelpSource/Classes" TYPE FILE FILES
    "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/HelpSource/Classes/PerlinNoise.schelp"
    "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/HelpSource/Classes/PerlinNoise2D.schelp"
    "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/HelpSource/Classes/PerlinNoise3D.schelp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/PerlinNoise/HelpSource/Guides" TYPE FILE FILES "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/HelpSource/Guides/PerlinNoisePorts.schelp")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/build/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
if(CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_COMPONENT MATCHES "^[a-zA-Z0-9_.+-]+$")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
  else()
    string(MD5 CMAKE_INST_COMP_HASH "${CMAKE_INSTALL_COMPONENT}")
    set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INST_COMP_HASH}.txt")
    unset(CMAKE_INST_COMP_HASH)
  endif()
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/fmm/Desktop/SuperCollider UGen Ports/02-hodgepodge-ports/PerlinNoise/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
