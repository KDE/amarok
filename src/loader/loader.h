/***************************************************************************
                          loader.h  -  loader application for Amarok
                             -------------------
    begin                : 2004/02/19
    copyright            : (C) 2004 Mark Kretschmann <markey@web.de>
                           (C) 2005 Max Howell
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LOADER_H
#define LOADER_H

#include <qapplication.h>

class QProcess;
class QStringList;

class Loader : public QApplication
{
public:
    Loader( QStringList );
   ~Loader();

private:
    virtual void timerEvent( QTimerEvent* );

    QProcess *m_proc;
    int       m_counter;
    QWidget  *m_splash;

    static const int INTERVAL = 10; //ms
};

static bool isSplashEnabled();
static bool amarokIsRunning();

#define foreach( x ) \
    for( QStringList::ConstIterator it = x.begin(), end = x.end(); it != end; ++it )

#endif
