/***************************************************************************
                          collectionscannerdcophandler.h  -  DCOP Interface
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

#ifndef COLLECTIONSCANNER_DBUS_HANDLER_H
#define COLLECTIONSCANNER_DBUS_HANDLER_H


#include <QObject>

#include <kapplication.h>

class DbusCollectionScannerHandler : public QObject
{
      Q_OBJECT

   public:
      DbusCollectionScannerHandler();

   signals:
      void pauseRequest();
      void unpauseRequest();

   public:
      virtual void pause();
      virtual void unpause();
};

#endif
