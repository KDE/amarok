add_subdirectory( data )
if(CMAKE_BUILD_TYPE MATCHES debugfull)
QT4_AUTOMOC( TestUpcomingEventsEngine.cpp )
endif(CMAKE_BUILD_TYPE MATCHES debugfull)
