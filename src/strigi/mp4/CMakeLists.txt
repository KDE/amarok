include_directories(${STRIGI_INCLUDE_DIR})

# build the analyzer as a module
kde4_add_library(mp4 MODULE mp4throughanalyzer.cpp)

# link with the required libraries
target_link_libraries(mp4 ${STRIGI_STREAMANALYZER_LIBRARY} ${STRIGI_STREAMS_LIBRARY})

# adjust the name so the module is recognized as a strigi plugin
set_target_properties(mp4 PROPERTIES PREFIX strigita_)

# install the module in the right directory so it is picked up
install(TARGETS mp4 LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)
