# - Try to find libusb-1.0
# Once done this will define
#
#  LIBUSB_1_FOUND - system has libusb
#  LIBUSB_1_INCLUDE_DIRS - the libusb include directory
#  LIBUSB_1_LIBRARIES - Link these to use libusb
#  LIBUSB_1_DEFINITIONS - Compiler switches required for using libusb
#
#  Adapted from cmake-modules Google Code project
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#
#  (Changes for libusb) Copyright (c) 2008 Kyle Machulis <kyle@nonpolynomial.com>
#
# Redistribution and use is allowed according to the terms of the New BSD license.
#
# CMake-Modules Project New BSD License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the CMake-Modules Project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#


if (LIBUSB_1_LIBRARIES AND LIBUSB_1_INCLUDE_DIRS)
  # in cache already
  set(LIBUSB_1_FOUND TRUE)
else (LIBUSB_1_LIBRARIES AND LIBUSB_1_INCLUDE_DIRS)
  find_path(LIBUSB_1_INCLUDE_DIR
    NAMES
	libusb.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
	PATH_SUFFIXES
	  libusb-1.0
  )

  set(LIBUSB_1_LIBRARY_NAME usb-1.0)
  if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(LIBUSB_1_LIBRARY_NAME usb)
  endif(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")

if(NOT bananapi-r2)
  find_library(LIBUSB_1_LIBRARY
    NAMES ${LIBUSB_1_LIBRARY_NAME}
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )
endif(NOT bananapi-r2)

#bananapi-r2 ubuntu 16.04 has wrong libusb-1.0.so path
#when compiling on bananapi-r2 run cmake with -DBananapi-r2=YES argument like so:
#  $(builddir)/cmake -Dbananapi-r2=YES ../
if(bananapi-r2)
    message(STATUS "DEBUG --->>>> BANANAPI")
    find_library(LIBUSB_1_LIBRARY
    NAMES ${LIBUSB_1_LIBRARY_NAME}
    PATHS
      /lib/arm-linux-gnueabihf/
      /usr/lib/arm-linux-gnueabihf/
    NO_DEFAULT_PATH
  )
endif(bananapi-r2)  

  set(LIBUSB_1_INCLUDE_DIRS
    ${LIBUSB_1_INCLUDE_DIR}
  )
  set(LIBUSB_1_LIBRARIES
    ${LIBUSB_1_LIBRARY}
)

  if (LIBUSB_1_INCLUDE_DIRS AND LIBUSB_1_LIBRARIES)
     set(LIBUSB_1_FOUND TRUE)
  endif (LIBUSB_1_INCLUDE_DIRS AND LIBUSB_1_LIBRARIES)

  if (LIBUSB_1_FOUND)
    if (NOT libusb-1.0_FIND_QUIETLY)
      message(STATUS "Found libusb-1.0:")
	    message(STATUS " - LIBUSB_1_INCLUDE_DIRS: ${LIBUSB_1_INCLUDE_DIRS}")
	    message(STATUS " - LIBUSB_1_LIBRARIES: ${LIBUSB_1_LIBRARIES}")
    endif (NOT libusb-1.0_FIND_QUIETLY)
  else (LIBUSB_1_FOUND)
    if (libusb-1.0_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libusb")
    endif (libusb-1.0_FIND_REQUIRED)
  endif (LIBUSB_1_FOUND)

  # show the LIBUSB_1_INCLUDE_DIRS and LIBUSB_1_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBUSB_1_INCLUDE_DIRS LIBUSB_1_LIBRARIES)
  mark_as_advanced(LIBUSB_1_INCLUDE_DIR LIBUSB_1_LIBRARY)

endif (LIBUSB_1_LIBRARIES AND LIBUSB_1_INCLUDE_DIRS)

set(libusb-1.0_FOUND ${LIBUSB_1_FOUND})