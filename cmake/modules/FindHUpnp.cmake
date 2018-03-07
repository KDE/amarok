# - Find HUpnp
# HUpnp is a Universal Plug and Play (UPnP) library
# used by the UPnP collection.
# Defines:
# HUPNP_INCLUDE_DIR
# HUPNP_LIBRARIES
# HUPNP_FOUND

find_path(HUPNP_INCLUDE_DIR HUpnp HINTS ${KDE4_INCLUDE_DIR})

find_library(HUPNP_LIBRARIES HUpnp PATHS ${KDE4_LIB_DIR})

if(HUPNP_INCLUDE_DIR AND HUPNP_LIBRARIES)
  set(HUPNP_FOUND TRUE)
  message(STATUS "Found HUpnp")
else()
  set(HUPNP_FOUND FALSE)
  if(HUPNP_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find required package HUpnp: <http://herqq.org>")
  endif()
endif()

