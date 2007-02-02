project(Amarok)

find_package(KDE4)
#following lines are copied from the koffice CMakeLists.txt
#not sure what they do, but they sound useful. Remove if necessary
include(KDE4Defaults)
include(MacroLibrary)

add_definitions(-DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS) 

OPTION(AMAROK_AMAZON_SUPPORT   "Enable Amazon cover download support")

add_subdirectory( src )

