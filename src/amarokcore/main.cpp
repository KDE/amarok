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
    aboutData.addAuthor( "Christian 'Babe-Magnet' Muehlhaeuser", I18N_NOOP( "Stud (muesli)" ), "chris@chris.de", "http://www.chris.de" );
    aboutData.addAuthor( "Frederik 'Ich bin kein Deustcher!' Holljen", I18N_NOOP( "733t code, OSD improvement, patches (Larson)" ), "fh@ez.no" );
    aboutData.addAuthor( "Mark 'It's good, but it's not Irssi' Kretschmann", I18N_NOOP( "Project founder (markey)" ), "markey@web.de" );
    aboutData.addAuthor( "Max 'Turtle-Power' Howell", I18N_NOOP( "Knight of the regression round-table (mxcl)" ), "max.howell@methylblue.com", "http://www.methyblue.com" );
    aboutData.addAuthor( "Mike 'Purple is not girly!' Diehl", I18N_NOOP( "Preci-i-o-u-u-s handbook maintainer (madpenguin8)" ), "madpenguin8@yahoo.com" );
    aboutData.addAuthor( "Pierpaolo 'Spaghetti Coder' Di Panfilo", I18N_NOOP( "Playlist-browser, cover-manager (teax)" ), "pippo_dp@libero.it" );
    aboutData.addAuthor( "Roman 'And God said, let there be Mac' Becker", I18N_NOOP( "amaroK logo, splash screen, icons" ), "roman@formmorf.de", "http://www.formmorf.de" );
    aboutData.addAuthor( "Stanislav 'All you need is DCOP' Karchebny", I18N_NOOP( "DCOP, improvements, cleanups, i18n (berkus)" ), "berk@upnet.ru" );
    aboutData.addAuthor( "Stefan 'MacroMolecularMooModeler' Bogner", I18N_NOOP( "Geeeeza (Bochi)" ), "bochi@online.ms" );

    aboutData.addCredit( "Adam Pigg", I18N_NOOP( "analyzers, patches" ), "adam@piggz.fsnet.co.uk" );
    aboutData.addCredit( "Adeodato Simó", I18N_NOOP( "patches" ), "asp16@alu.ua.es" );
    aboutData.addCredit( "Andreas Mair", I18N_NOOP( "MySQL support" ), "am_ml@linogate.com" );
    aboutData.addCredit( "Dan Leinir Turthra Jensen", I18N_NOOP( "first-run wizard, usability" ), "admin@REMOVEleinir.dk" );
    aboutData.addCredit( "Enrico Ros", I18N_NOOP( "analyzers, king of openGL" ), "eros.kde@email.it" );
    aboutData.addCredit( "Jarkko Lehti", I18N_NOOP( "tester, IRC channel operator, whipping" ), "grue@iki.fi" );
    aboutData.addCredit( "Josef Spillner", I18N_NOOP( "KDE RadioStation code" ), "spillner@kde.org" );
    aboutData.addCredit( "Kenneth Wesley Wimer II", I18N_NOOP( "icons" ), "kwwii@bootsplash.org" );
    aboutData.addCredit( "Melchior Franz", I18N_NOOP( "new FFT routine, bugfixes" ), "mfranz@kde.org" );
    aboutData.addCredit( "Michael Pyne", I18N_NOOP( "K3B export code" ), "michael.pyne@kdemail.net" );
    aboutData.addCredit( "Nenad Grujicic", I18N_NOOP( "splash screen for 1.1" ), "mchitman@neobee.net" );
    aboutData.addCredit( "Olivier Bédard", I18N_NOOP( "website hosting" ), "paleo@pwsp.net" );
    aboutData.addCredit( "Roland Gigler", I18N_NOOP( "MAS engine" ), "rolandg@web.de" );
    aboutData.addCredit( "Sami Nieminen", I18N_NOOP( "Audioscrobbler support" ), "sami.nieminen@iki.fi" );
    aboutData.addCredit( "Scott Wheeler", I18N_NOOP( "TagLib & ktrm code" ), "wheeler@kde.org" );
    aboutData.addCredit( "Seb Ruiz", I18N_NOOP( "OSD improvements, patches, testing" ), "seb100@optusnet.com.au" );
    aboutData.addCredit( "Whitehawk Stormchaser", I18N_NOOP( "tester, patches" ), "zerokode@gmx.net" );


    KApplication::disableAutoDcopRegistration();

    //make amarokapp into amarok
    for( uint x = 1, len = strlen( argv[0] ); x & 3; ++x )
        argv[0][len-x] = '\0';

    App::initCliArgs( argc, argv );
    App app;

    return app.exec();
}
