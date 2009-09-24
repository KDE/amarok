set(ocsclient_SRCS
   providerinitjob.cpp
   eventjob.cpp
   eventlistjob.cpp
   eventparser.cpp
   event.cpp
   activity.cpp
   activityparser.cpp
   activitylistjob.cpp
   person.cpp
   personparser.cpp
   personjob.cpp
   personlistjob.cpp
   provider.cpp
   postjob.cpp
   folder.cpp
   folderlistjob.cpp
   folderparser.cpp
   message.cpp
   messagelistjob.cpp
   messageparser.cpp
   category.cpp
   categorylistjob.cpp
   categoryparser.cpp
   content.cpp
   contentjob.cpp
   contentlistjob.cpp
   contentparser.cpp
   knowledgebase.cpp
   knowledgebasejob.cpp
   knowledgebaselistjob.cpp
   knowledgebaseparser.cpp
 )



kde4_add_library(amarokocsclient SHARED ${ocsclient_SRCS})
set_target_properties(amarokocsclient PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION})

target_link_libraries(amarokocsclient ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KABC_LIBS})

install(TARGETS amarokocsclient ${INSTALL_TARGETS_DEFAULT_ARGS})
