if(KDE4_BUILD_TESTS)
install(
        FILES
        testsheet01-iso8859-1.cue
        testsheet01-utf8.cue
        invalid.cue
        test_silence.ogg
        DESTINATION ${DATA_INSTALL_DIR}/amarok/testdata/cue
)
endif()
