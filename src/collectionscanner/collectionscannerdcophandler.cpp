/***************************************************************************
                          collectionscannerdcophandler.cpp  -  DCOP Implementation
                             -------------------
    begin                : 16/08/05
    copyright            : (C) 2006 by Jeff Mitchell
    email                : kde-dev@emailgoeshere.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "collectionscannerdcophandler.h"

#include <dcopclient.h>

/////////////////////////////////////////////////////////////////////////////////////
// class DcopCollectionScannerHandler
/////////////////////////////////////////////////////////////////////////////////////

    DcopCollectionScannerHandler::DcopCollectionScannerHandler()
        : DCOPObject( "scanner" )
        , QObject( kapp )
    {
        // Register with DCOP
        if ( !kapp->dcopClient()->isRegistered() ) {
            kapp->dcopClient()->registerAs( "amarokcollectionscanner", false );
            kapp->dcopClient()->setDefaultObject( objId() );
        }
    }

    void DcopCollectionScannerHandler::pause()
    {
        //do nothing for now
        emit pauseRequest();
    }

    void DcopCollectionScannerHandler::unpause()
    {
        //do nothing for now
        emit unpauseRequest();
    }

#include "collectionscannerdcophandler.moc"
