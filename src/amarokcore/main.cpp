/***************************************************************************
                         main.cpp  -  description
                            -------------------
   begin                : Mit Okt 23 14:35:18 CEST 2002
   copyright            : (C) 2002 by Mark Kretschmann
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

#include "playerapp.h"

#include <qstring.h>


PlayerApp *pApp;


int main( int argc, char *argv[] )
{
    KApplication::disableAutoDcopRegistration();
    
    QString arg0 = argv[0];
    arg0.replace( "amarokapp", "amarok" );
    argv[0] = const_cast<char*>( arg0.latin1() );
    
    PlayerApp::initCliArgs( argc, argv );
    PlayerApp app;
    
    return app.exec();
}
