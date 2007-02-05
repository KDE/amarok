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
#include "crashhandler.h"
#include <kaboutdata.h>
#include <kcrash.h>

#include "metadata/tplugins.h"

//#define AMAROK_USE_DRKONQI

extern class KAboutData aboutData; //defined in amarokcore/app.cpp

int main( int argc, char *argv[] )
{
    aboutData.addAuthor( "Alexandre 'Ain't afraid of no bugs' Oliveira",
            ( "Developer (Untouchable)" ), "aleprj@gmail.com" );
    aboutData.addAuthor( "Christian 'Babe-Magnet' Muehlhaeuser",
            ( "Stud (muesli)" ), "chris@chris.de", "http://www.chris.de" );
    aboutData.addAuthor( "Frederik 'Ich bin kein Deustcher!' Holljen",
            ( "733t code, OSD improvement, patches (Larson)" ), "fh@ez.no" );
    aboutData.addAuthor( "Gábor 'Opera owns your mom' Lehel",
            ( "Developer (illissius)" ), "illissius@gmail.com" );
    aboutData.addAuthor( "Ian 'The Beard' Monroe",
            ( "Developer (eean)" ), "ian@monroe.nu", "http://www.monroe.nu/" );
    aboutData.addAuthor( "Jeff 'IROCKSOHARD' Mitchell",
            ( "Developer (jefferai)" ), "kde-dev@emailgoeshere.com" );
    aboutData.addAuthor( "Mark 'It's good, but it's not irssi' Kretschmann",
            ( "Project founder (markey)" ), "markey@web.de" );
    aboutData.addAuthor( "Martin 'Easily the most compile-breaks ever!' Aumueller",
            ( "Developer (aumuell)" ), "aumuell@reserv.at" );
    aboutData.addAuthor( "Max 'Turtle-Power' Howell",
            ( "Cowboy mxcl" ), "max.howell@methylblue.com", "http://www.methylblue.com" );
    aboutData.addAuthor( "Mike 'Purple is not girly!' Diehl",
            ( "DCOP, improvements, Preci-i-o-u-u-s handbook maintainer (madpenguin8)" ), "madpenguin8@yahoo.com" );
    aboutData.addAuthor( "Paul 'Meet me at the Amarok Bar!' Cifarelli",
            ( "Developer (foreboy)" ), "paul@cifarelli.net" );
    aboutData.addAuthor( "Pierpaolo 'Spaghetti Coder' Di Panfilo",
            ( "Playlist-browser, cover-manager (teax)" ), "pippo_dp@libero.it" );
    aboutData.addAuthor( "Roman 'And God said, let there be Mac' Becker",
            ( "Amarok logo, splash screen, icons" ), "roman@formmorf.de", "http://www.formmorf.de" );
    aboutData.addAuthor( "Seb 'Surfin' down under' Ruiz",
            ( "Developer (sebr)" ), "me@sebruiz.net", "http://www.sebruiz.net" );
    aboutData.addAuthor( "Stanislav 'All you need is DCOP' Karchebny",
            ( "DCOP, improvements, cleanups, i18n (berkus)" ), "berkus@madfire.net" );


    aboutData.addCredit( "Adam Pigg", ( "Analyzers, patches, shoutcast" ), "adam@piggz.co.uk" );
    aboutData.addCredit( "Adeodato Simó", ( "Patches" ), "asp16@alu.ua.es" );
    aboutData.addCredit( "Andreas Mair", ( "MySQL support" ), "am_ml@linogate.com" );
    aboutData.addCredit( "Andrew de Quincey", ( "Postgresql support" ), "adq_dvb@lidskialf.net" );
    aboutData.addCredit( "Andrew Turner", ( "Patches" ), "andrewturner512@googlemail.com" );
    aboutData.addCredit( "Bart Cerneels", ( "podcast code improvements" ), "shanachie@yucom.be" );
    aboutData.addCredit( "Christie Harris", ( "roKymoter (dangle)" ), "dangle.baby@gmail.com" );
    aboutData.addCredit( "Dan Leinir Turthra Jensen", ( "First-run wizard, usability" ), "admin@REMOVEleinir.dk" );
    aboutData.addCredit( "Dan Meltzer", ( "roKymoter (hydrogen)" ), "hydrogen@notyetimplemented.com" );
    aboutData.addCredit( "Derek Nelson", ( "graphics, splash-screen" ), "admrla@gmail.com" );
    aboutData.addCredit( "Enrico Ros", ( "Analyzers, Context Browser and systray eye-candy" ), "eros.kde@email.it" );
    aboutData.addCredit( "Gérard Dürrmeyer", ( "icons and image work" ), "gerard@randomtree.com" );
    aboutData.addCredit( "Giovanni Venturi", ( "dialog to filter the collection titles" ), "giovanni@ksniffer.org" );
    aboutData.addCredit( "Greg Meyer", ( "Live CD, Bug squashing (oggb4mp3)" ), "greg@gkmweb.com" );
    aboutData.addCredit( "Harald Sitter", ( "handbook enhancements, translations, bug fixes, screenshots, roKymoter (apachelogger)" ), "harald.sitter@kdemail.net" );
    aboutData.addCredit( "Jarkko Lehti", ( "Tester, IRC channel operator, whipping" ), "grue@iki.fi" );
    aboutData.addCredit( "Jocke Andersson", ( "roKymoter, bug fixer and Swedish Bitch (Firetech)" ), "ajocke@gmail.com" );
    aboutData.addCredit( "Kenneth Wesley Wimer II", ( "Icons" ), "kwwii@bootsplash.org" );
    aboutData.addCredit( "Marco Gulino", ( "Konqueror Sidebar, some DCOP methods" ), "marco@kmobiletools.org" );
    aboutData.addCredit( "Maximilian Kossick", ( "Dynamic Collection, label support, patches" ), "maximilian.kossick@gmail.com" );
    aboutData.addCredit( "Melchior Franz", ( "FHT routine, bugfixes" ), "mfranz@kde.org" );
    aboutData.addCredit( "Michael Pyne", ( "K3B export code" ), "michael.pyne@kdemail.net" );
    aboutData.addCredit( "Nenad Grujicic", ( "Splash screen" ), "mchitman@neobee.net" );
    aboutData.addCredit( "Nikolaj Hald Nielsen", ( "Magnatune.com store integration (nhnFreespirit)" ), "nhnFreespirit@gmail.com" );
    aboutData.addCredit( "Olivier Bédard", ( "Website hosting" ), "paleo@pwsp.net" );
    aboutData.addCredit( "Peter C. Ndikuwera", ( "Bugfixes, PostgreSQL support" ), "pndiku@gmail.com" );
    aboutData.addCredit( "Reigo Reinmets", ( "Wikipedia support, patches" ), "xatax@hot.ee" );
    aboutData.addCredit( "Roland Gigler", ( "MAS engine" ), "rolandg@web.de" );
    aboutData.addCredit( "Sami Nieminen", ( "Audioscrobbler support" ), "sami.nieminen@iki.fi" );
    aboutData.addCredit( "Scott Wheeler", ( "TagLib & ktrm code" ), "wheeler@kde.org" );
    aboutData.addCredit( "Shane King", ( "Patches" ), "kde@dontletsstart.com" );
    aboutData.addCredit( "Stefan Bogner", ( "Loadsa stuff" ), "bochi@online.ms" );
    aboutData.addCredit( "Stefan Siegel", ( "Patches, Bugfixes" ), "kde@sdas.de" );
    aboutData.addCredit( "Sven Krohlas", ( "roKymoter (sven423)" ), "sven@asbest-online.de" );
    aboutData.addCredit( "Vadim Petrunin", ( "Graphics, splash-screen (vnizzz)" ), "vnizzz@list.ru" );
    aboutData.addCredit( "Whitehawk Stormchaser", ( "Tester, patches" ), "zerokode@gmx.net" );

    registerTaglibPlugins();

    App::initCliArgs( argc, argv );
    App app;

#ifdef Q_WS_X11
    #ifndef AMAROK_USE_DRKONQI
    KCrash::setCrashHandler( Amarok::Crash::crashHandler );
    #endif
#endif


    return app.exec();
}
