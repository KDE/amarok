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

#include "collectionscannerdbushandler.h"
#include <scanneradaptor.h>

/////////////////////////////////////////////////////////////////////////////////////
// class DbusCollectionScannerHandler
/////////////////////////////////////////////////////////////////////////////////////

DbusCollectionScannerHandler::DbusCollectionScannerHandler()
        : QObject( kapp )
    {
    (void)new ScannerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/Scanner"), this); 
    }

    void DbusCollectionScannerHandler::pause()
    {
        //do nothing for now
        emit pauseRequest();
    }

    void DbusCollectionScannerHandler::unpause()
    {
        //do nothing for now
        emit unpauseRequest();
    }

#include "collectionscannerdbushandler.moc"
