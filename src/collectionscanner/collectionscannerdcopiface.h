/***************************************************************************
                          collectionscannerdcopiface.h  -  DCOP Interface
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

#ifndef COLLECTIONSCANNER_DCOPIFACE_H
#define COLLECTIONSCANNER_DCOPIFACE_H

#include <dcopobject.h>

///////////////////////////////////////////////////////////////////////
// WARNING! Please ask on #amarok before modifying the DCOP interface!
///////////////////////////////////////////////////////////////////////


class CollectionScannerInterface : virtual public DCOPObject
{
   K_DCOP

k_dcop:
    virtual void pause() = 0;                           ///< Pause the scanner
    virtual void unpause() = 0;                         ///< Unpause the scanner
};

#endif
