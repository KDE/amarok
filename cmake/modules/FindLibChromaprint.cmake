# - Try to find the GNU LibChromaprint library
# Once done this will define
#
#  LIBCHROMAPRINT_FOUND - system has the Libgcrypt library
#  LIBCHROMAPRINT_LIBS - The libraries needed to use Libgcrypt

# Copyright (c) 2013, Vedant Agarwala <vedant.kota@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(CheckIncludeFiles)

check_include_files(chromaprint.h HAVE_LIBCHROMAPRINT_H)

if (HAVE_LIBCHROMAPRINT_H)
   set(LIBCHROMAPRINT_HEADERS_FOUND TRUE)
endif (HAVE_LIBCHROMAPRINT_H)

if (LIBCHROMAPRINT_HEADERS_FOUND)
   find_library(LIBCHROMAPRINT_LIBS NAMES chromaprint )
endif (LIBCHROMAPRINT_HEADERS_FOUND)

if (LIBCHROMAPRINT_LIBS)
   set(LIBCHROMAPRINT_LIBS_FOUND TRUE)
   message(STATUS "Libgcrypt found: ${LIBGCRYPT_LIBS}")
endif (LIBGCRYPT_LIBS)
