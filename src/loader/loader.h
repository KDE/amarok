/***************************************************************************
                          loader.h  -  loader application for amaroK
                             -------------------
    begin                : 2004/02/19
    copyright            : (C) 2004 by Mark Kretschmann
    email                : markey@web.de
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

#include <qapplication.h>    //baseclass

class OSDWidget;
class QTimerEvent;

class Loader : public QApplication
{
    Q_OBJECT

    public:
        Loader( int& argc, char** argv );

    private slots:
        void doExit();

    private:
        bool splashEnabled() const;
        void showSplash();
        int  tryConnect( bool verbose = false );
        void timerEvent( QTimerEvent* );

        enum SocketType { loader, Vis };

        QCString socketPath( SocketType type = loader );

// ATTRIBUTES ------
        static const int TIMER_INTERVAL = 50;
        static const int TIMEOUT = 60;

        int        m_sockfd;
        OSDWidget *m_pOsd;
};


#endif
