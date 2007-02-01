
include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/amarok/scripts/templates FILES  python_qt_template.py      ruby_qt_template.rb      amarok.rb )



#original Makefile.am contents follow:

#templatedir = \
#    $(kde_datadir)/amarok/scripts/templates
#
#template_DATA = \
#    python_qt_template.py \
#    ruby_qt_template.rb \
#    amarok.rb
#
#
