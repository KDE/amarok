        include_directories(
            ../
            ${Amarok_SOURCE_DIR}/src/
            ${Amarok_SOURCE_DIR}/src/core-impl/collections
            ${Amarok_SOURCE_DIR}/src/network
            ${Amarok_SOURCE_DIR}/src/statusbar
            ${CMAKE_CURRENT_BINARY_DIR}/../../..
            
            ${QCA2_INCLUDE_DIR}
            
            )

if( LIBLASTFM_FOUND )
    include_directories(
        ${LIBLASTFM_INCLUDE_DIR}
    )
endif()

add_subdirectory( images )

########### next target ###############

set(libampache_account_login_SRCS
    AmpacheAccountLogin.cpp)
add_library(ampache_account_login SHARED ${libampache_account_login_SRCS})
target_link_libraries(ampache_account_login
    amaroklib
    amarokcore
    KF5::KDELibs4Support
    
    KF5::KIOCore
    ${QCA2_LIBRARIES}
)
install(TARGETS ampache_account_login DESTINATION ${INSTALL_TARGETS_DEFAULT_ARGS})

########### next target ###############
        set(amarok_service_ampache_PART_SRCS 
            AmpacheService.cpp
            AmpacheServiceCollection.cpp
            AmpacheServiceQueryMaker.cpp
            AmpacheMeta.cpp
            AmpacheConfig.cpp
        )
        if( LIBLASTFM_FOUND )
            set(amarok_service_ampache_PART_SRCS
                ${amarok_service_ampache_PART_SRCS}
                LastfmInfoParser.cpp )
        endif()



        add_library(amarok_service_ampache MODULE ${amarok_service_ampache_PART_SRCS})
        target_link_libraries(amarok_service_ampache
            ampache_account_login
            amarokcore
            amaroklib
            KF5::KDELibs4Support
            
            KF5::KIOCore
            KF5::ThreadWeaver
            Qt5::Xml
            
        )
        if( LIBLASTFM_FOUND )
            target_link_libraries(amarok_service_ampache
                ${LIBLASTFM_LIBRARY}
            )
        endif()


        install(TARGETS amarok_service_ampache DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

        set(kcm_amarok_service_ampache_PART_SRCSS AddServerDialog.cpp AmpacheSettings.cpp AmpacheConfig.cpp )

        ki18n_wrap_ui( kcm_amarok_service_ampache_PART_SRCSS AmpacheConfigWidget.ui NewServerWidget.ui )


        add_library(kcm_amarok_service_ampache MODULE ${kcm_amarok_service_ampache_PART_SRCSS} )


        target_link_libraries( kcm_amarok_service_ampache
            ampache_account_login  Qt5::Core
            Qt5::Gui   )

        install(TARGETS kcm_amarok_service_ampache  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

        install( FILES amarok_service_ampache.desktop DESTINATION ${SERVICES_INSTALL_DIR})
        install( FILES amarok_service_ampache_config.desktop DESTINATION ${SERVICES_INSTALL_DIR})
