/***************************************************************************
                          loader.h  -  description
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
class QProcess;

class Loader : public QApplication
{
    Q_OBJECT
    
    public:
        Loader( int& argc, char** argv );
        ~Loader();
        
    private slots:
        void gotPrefix();
        void loaded();
        
    private:
        int tryConnect();

// ATTRIBUTES ------
        int        m_argc;        
        char**     m_argv;
        
        OSDWidget* m_pOsd;
        QProcess*  m_pPrefixProc;
};


#endif
