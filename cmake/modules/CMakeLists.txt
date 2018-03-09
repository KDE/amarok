# install the cmake files

file(GLOB cmakeFiles "${CMAKE_CURRENT_SOURCE_DIR}/*.cmake")

set(module_install_dir ${KDE_INSTALL_DATADIR}/cmake/modules )

install( FILES  ${cmakeFiles} DESTINATION ${module_install_dir}  )

