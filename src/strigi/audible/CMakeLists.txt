find_package(Strigi REQUIRED)
include_directories(${STRIGI_INCLUDE_DIR})

# build the analyzer as a module
kde4_add_library(audible MODULE audiblethroughanalyzer.cpp)

# link with the required libraries
target_link_libraries(audible ${STRIGI_STREAMANALYZER_LIBRARY} ${STRIGI_STREAMS_LIBRARY})

# adjust the name so the module is recognized as a strigi plugin
set_target_properties(audible PROPERTIES PREFIX strigita_)

# install the module in the right directory so it is picked up
install(TARGETS audible LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strigi)
