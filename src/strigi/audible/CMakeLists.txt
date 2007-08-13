find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

find_package(Strigi REQUIRED)
include_directories(${STRIGI_INCLUDE_DIR})

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

MACRO(ADD_STRIGITA libname files)
  if(WIN32)
    # this is needed to have mingw and msvc libs installed in one directory
    if(MSVC)
      set(prefix msvc_strigita_)
    else(MSVC)
      # mingw
      set(prefix mingw_strigita_)
    endif(MSVC)
  else(WIN32)
    set(prefix strigita_)
  endif(WIN32)
  add_library(${libname} MODULE ${files})
  set_target_properties(${libname} PROPERTIES
    PREFIX ${prefix}
    VERSION ${STRIGI_VERSION}
    SOVERSION ${STRIGI_VERSION_MAJOR})
  target_link_libraries(${libname} ${STRIGI_STREAMANALYZER_LIBRARY} ${STRIGI_STREAMS_LIBRARY})
  install(TARGETS ${libname} LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)
ENDMACRO(ADD_STRIGITA)

ADD_STRIGITA(audible  audiblethroughanalyzer.cpp)
