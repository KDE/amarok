/***************************************************************************
                          amarokdcopiface.h  -  DCOP Interface
                             -------------------
    begin                : Sat Oct 11 2003
    copyright            : (C) 2003 by Stanislav Karchebny
    email                : berkus@users.sf.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DCOPIFACE_H
#define AMAROK_DCOPIFACE_H

//#include <qobject.h>
//#include <qstringlist.h>

#include <dcopobject.h>

class AmarokIface : virtual public DCOPObject
{
   K_DCOP

k_dcop:
   virtual void play() = 0;
   virtual void stop() = 0;
   virtual void next() = 0;
   virtual void prev() = 0;
   virtual void pause() = 0;

   virtual QString nowPlaying() = 0;
   virtual bool isPlaying() = 0;
};

#endif
