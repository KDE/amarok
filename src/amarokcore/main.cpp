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

#include "amarok.h"
#include "app.h"
#include <kaboutdata.h>


KAboutData aboutData( "amarok",
    I18N_NOOP( "amaroK" ), APP_VERSION,
    I18N_NOOP( "The audio player for KDE" ), KAboutData::License_GPL,
    I18N_NOOP( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2004, The amaroK Development Squad" ),
    I18N_NOOP( "IRC:\nserver: irc.freenode.net / channel: #amarok\n\nFeedback:\namarok-devel@lists.sourceforge.net" ),
    I18N_NOOP( "http://amarok.kde.org" ) );


int main( int argc, char *argv[] )
{
    aboutData.addAuthor( "Christian 'Babe-Magnet' Muehlhaeuser", I18N_NOOP( "developer, stud" ), "chris@chris.de", "http://www.chris.de" );
    aboutData.addAuthor( "Frederik 'Ich bin kein Deustcher!' Holljen", I18N_NOOP( "developer, 733t code, OSD improvement, patches" ), "fh@ez.no" );
    aboutData.addAuthor( "Mark 'It's good, but it's not Irssi' Kretschmann", I18N_NOOP( "project founder, developer, maintainer" ), "markey@web.de" );
    aboutData.addAuthor( "Max 'Turtle-Power' Howell", I18N_NOOP( "developer, knight of the regression round-table" ), "max.howell@methylblue.com", "http://www.methyblue.com" );
    aboutData.addAuthor( "Mike 'Purple is not girly!' Diehl", I18N_NOOP( "Preci-i-o-u-u-s handbook maintainer" ), "madpenguin8@yahoo.com" );
    aboutData.addAuthor( "Roman 'And God said, let there be Mac' Becker", I18N_NOOP( "graphics: amaroK logo, splash screen, icons" ), "roman@formmorf.de", "http://www.formmorf.de" );
    aboutData.addAuthor( "Stanislav 'All you need is DCOP' Karchebny", I18N_NOOP( "developer, DCOP, improvements, cleanups, i18n" ), "berk@upnet.ru" );

    aboutData.addCredit( "Adam Pigg", I18N_NOOP( "analyzers, patches" ), "adam@piggz.fsnet.co.uk" );
    aboutData.addCredit( "Adeodato Simó", I18N_NOOP( "patches" ), "asp16@alu.ua.es" );
    aboutData.addCredit( "Dan Leinir Turthra Jensen", I18N_NOOP( "first-run wizard" ), "admin@REMOVEleinir.dk" );
    aboutData.addCredit( "Enrico Ros", I18N_NOOP( "analyzers, king of openGL" ), "eros.kde@email.it" );
    aboutData.addCredit( "Jarkko Lehti", I18N_NOOP( "tester, IRC channel operator, whipping" ), "grue@iki.fi" );
    aboutData.addCredit( "Josef Spillner", I18N_NOOP( "KDE RadioStation code" ), "spillner@kde.org" );
    aboutData.addCredit( "Melchior Franz", I18N_NOOP( "new FFT routine, bugfixes" ), "mfranz@kde.org" );
    aboutData.addCredit( "Pierpaolo Di Panfilo", I18N_NOOP("Playlist browser"), "pippo_dp@libero.it" );
    aboutData.addCredit( "Scott Wheeler", "TagLib", "wheeler@kde.org" );
    aboutData.addCredit( "Stefan Bogner", I18N_NOOP( "tester, patches, translation" ), "bochi@online.ms" );
    aboutData.addCredit( "Whitehawk Stormchaser", I18N_NOOP( "tester, patches" ), "zerokode@gmx.net" );


    KApplication::disableAutoDcopRegistration();

    //make amarokapp into amarok
    for( uint x = 1, len = strlen( argv[0] ); x & 3; ++x )
        argv[0][len-x] = '\0';

    App::initCliArgs( argc, argv );
    App app;

    return app.exec();
}
