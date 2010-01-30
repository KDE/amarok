if(CMAKE_BUILD_TYPE MATCHES debugfull)
QT4_AUTOMOC( TestMetaCueCueFileItem.cpp
             TestMetaCueTrack.cpp
)
endif(CMAKE_BUILD_TYPE MATCHES debugfull)
