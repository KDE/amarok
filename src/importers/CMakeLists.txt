include_directories(
    ${KDE4_INCLUDE_DIR}
    ${QT_INCLUDES}
    ${Amarok_SOURCE_DIR}/src
)

add_subdirectory( itunes )
add_subdirectory( fastforward )
add_subdirectory( rhythmbox )
