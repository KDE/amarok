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
#include "metadata/tplugins.h"

#include <KAboutData>
#include <KCrash>
#include <KCmdLineArgs>

//#define AMAROK_USE_DRKONQI

extern class KAboutData aboutData; //defined in amarokcore/app.cpp


int main( int argc, char *argv[] )
{
    aboutData.addAuthor( ki18n("Alexandre 'Ain't afraid of no bugs' Oliveira"),
            ki18n(( "Developer (Untouchable)" )), "aleprj@gmail.com" );
    aboutData.addAuthor( ki18n("Christian 'Babe-Magnet' Muehlhaeuser"),
            ki18n(( "Stud (muesli)" )), "chris@chris.de", "http://www.chris.de" );
    aboutData.addAuthor( ki18n("Frederik 'Ich bin kein Deustcher!' Holljen"),
            ki18n(( "733t code, OSD improvement, patches (Larson)" )), "fh@ez.no" );
    aboutData.addAuthor( ki18n("Gábor 'Opera owns your mom' Lehel"),
            ki18n(( "Developer (illissius)" )), "illissius@gmail.com" );
    aboutData.addAuthor( ki18n("Ian 'The Beard' Monroe"),
            ki18n(( "Developer (eean)" )), "ian@monroe.nu", "http://www.monroe.nu/" );
    aboutData.addAuthor( ki18n("Jeff 'IROCKSOHARD' Mitchell"),
            ki18n(( "Developer (jefferai)" )), "kde-dev@emailgoeshere.com" );
    aboutData.addAuthor( ki18n("Mark 'It's good, but it's not irssi' Kretschmann"), //krazy:exclude=contractions
            ki18n(( "Project founder (markey)" )), "kretschmann@kde.org" );
    aboutData.addAuthor( ki18n("Martin 'Easily the most compile-breaks ever!' Aumueller"),
            ki18n(( "Developer (aumuell)" )), "aumuell@reserv.at" );
    aboutData.addAuthor( ki18n("Max 'Turtle-Power' Howell"),
            ki18n(( "Cowboy mxcl" )), "max.howell@methylblue.com", "http://www.methylblue.com" );
    aboutData.addAuthor( ki18n("Maximilian Kossick"),
            ki18n(( "Developer (maxx_k)" )), "maximilian.kossick@gmail.com" );
    aboutData.addAuthor( ki18n("Mike 'Purple is not girly!' Diehl"),
            ki18n(( "DCOP, improvements, Preci-i-o-u-u-s handbook maintainer (madpenguin8)" )), "madpenguin8@yahoo.com" );
    aboutData.addAuthor( ki18n("Paul 'Meet me at the Amarok Bar!' Cifarelli"),
            ki18n(( "Developer (foreboy)" )), "paul@cifarelli.net" );
    aboutData.addAuthor( ki18n("Pierpaolo 'Spaghetti Coder' Di Panfilo"),
            ki18n(( "Playlist-browser, cover-manager (teax)" )), "pippo_dp@libero.it" );
    aboutData.addAuthor( ki18n("Roman 'And God said, let there be Mac' Becker"),
            ki18n(( "Amarok logo, splash screen, icons" )), "roman@formmorf.de", "http://www.formmorf.de" );
    aboutData.addAuthor( ki18n("Seb 'Surfin' down under' Ruiz"),
            ki18n(( "Developer (sebr)" )), "ruiz@kde.org", "http://www.sebruiz.net" );
    aboutData.addAuthor( ki18n("Stanislav 'All you need is DCOP' Karchebny"),
            ki18n(( "DCOP, improvements, cleanups, i18n (berkus)" )), "berkus@madfire.net" );


    aboutData.addCredit( ki18n("Adam Pigg"), ki18n(( "Analyzers, patches, shoutcast" )), "adam@piggz.co.uk" );
    aboutData.addCredit( ki18n("Adeodato Simó"), ki18n(( "Patches" )), "asp16@alu.ua.es" );
    aboutData.addCredit( ki18n("Andreas Mair"), ki18n(( "MySQL support" )), "am_ml@linogate.com" );
    aboutData.addCredit( ki18n("Andrew de Quincey"), ki18n(( "Postgresql support" )), "adq_dvb@lidskialf.net" );
    aboutData.addCredit( ki18n("Andrew Turner"), ki18n(( "Patches" )), "andrewturner512@googlemail.com" );
    aboutData.addCredit( ki18n("Andy Kelk"), ki18n(( "MTP and Rio Karma media devices, patches" )), "andy@mopoke.co.uk" );
    aboutData.addCredit( ki18n("Bart Cerneels"), ki18n(( "podcast code improvements" )), "shanachie@yucom.be" );
    aboutData.addCredit( ki18n("Christie Harris"), ki18n(( "roKymoter (dangle)" )), "dangle.baby@gmail.com" );
    aboutData.addCredit( ki18n("Dan Leinir Turthra Jensen"), ki18n(( "First-run wizard, usability" )), "admin@REMOVEleinir.dk" );
    aboutData.addCredit( ki18n("Dan Meltzer"), ki18n(( "roKymoter (hydrogen)" )), "hydrogen@notyetimplemented.com" );
    aboutData.addCredit( ki18n("Derek Nelson"), ki18n(( "graphics, splash-screen" )), "admrla@gmail.com" );
    aboutData.addCredit( ki18n("Enrico Ros"), ki18n(( "Analyzers, Context Browser and systray eye-candy" )), "eros.kde@email.it" );
    aboutData.addCredit( ki18n("Gérard Dürrmeyer"), ki18n(( "icons and image work" )), "gerard@randomtree.com" );
    aboutData.addCredit( ki18n("Giovanni Venturi"), ki18n(( "dialog to filter the collection titles" )), "giovanni@ksniffer.org" );
    aboutData.addCredit( ki18n("Greg Meyer"), ki18n(( "Live CD, Bug squashing (oggb4mp3)" )), "greg@gkmweb.com" );
    aboutData.addCredit( ki18n("Harald Sitter"), ki18n(( "handbook enhancements, translations, bug fixes, screenshots, roKymoter (apachelogger)" )), "harald.sitter@kdemail.net" );
    aboutData.addCredit( ki18n("Jarkko Lehti"), ki18n(( "Tester, IRC channel operator, whipping" )), "grue@iki.fi" );
    aboutData.addCredit( ki18n("Jocke Andersson"), ki18n(( "roKymoter, bug fixer and Swedish Bitch (Firetech)" )), "ajocke@gmail.com" );
    aboutData.addCredit( ki18n("Kenneth Wesley Wimer II"), ki18n(( "Icons" )), "kwwii@bootsplash.org" );
    aboutData.addCredit( ki18n( "Leo Franchi" ), ki18n( ( "Context View work" )  ), "lfranchi@gmail.com" );
    aboutData.addCredit( ki18n("Marco Gulino"), ki18n(( "Konqueror Sidebar, some DCOP methods" )), "marco@kmobiletools.org" );
    aboutData.addCredit( ki18n("Melchior Franz"), ki18n(( "FHT routine, bugfixes" )), "mfranz@kde.org" );
    aboutData.addCredit( ki18n("Michael Pyne"), ki18n(( "K3B export code" )), "michael.pyne@kdemail.net" );
    aboutData.addCredit( ki18n("Nenad Grujicic"), ki18n(( "Splash screen" )), "mchitman@neobee.net" );
    aboutData.addCredit( ki18n("Nikolaj Hald Nielsen"), ki18n(( "Magnatune.com store integration (nhnFreespirit)" )), "nhnFreespirit@gmail.com" );
    aboutData.addCredit( ki18n("Olivier Bédard"), ki18n(( "Website hosting" )), "paleo@pwsp.net" );
    aboutData.addCredit( ki18n("Peter C. Ndikuwera"), ki18n(( "Bugfixes, PostgreSQL support" )), "pndiku@gmail.com" );
    aboutData.addCredit( ki18n("Reigo Reinmets"), ki18n(( "Wikipedia support, patches" )), "xatax@hot.ee" );
    aboutData.addCredit( ki18n("Roland Gigler"), ki18n(( "MAS engine" )), "rolandg@web.de" );
    aboutData.addCredit( ki18n("Sami Nieminen"), ki18n(( "Audioscrobbler support" )), "sami.nieminen@iki.fi" );
    aboutData.addCredit( ki18n("Scott Wheeler"), ki18n(( "TagLib & ktrm code" )), "wheeler@kde.org" );
    aboutData.addCredit( ki18n("Shane King"), ki18n(( "Patches" )), "kde@dontletsstart.com" );
    aboutData.addCredit( ki18n("Stefan Bogner"), ki18n(( "Loadsa stuff" )), "bochi@online.ms" );
    aboutData.addCredit( ki18n("Stefan Siegel"), ki18n(( "Patches, Bugfixes" )), "kde@sdas.de" );
    aboutData.addCredit( ki18n("Sven Krohlas"), ki18n(( "roKymoter (sven423)" )), "sven@asbest-online.de" );
    aboutData.addCredit( ki18n("Vadim Petrunin"), ki18n(( "Graphics, splash-screen (vnizzz)" )), "vnizzz@list.ru" );
    aboutData.addCredit( ki18n("Whitehawk Stormchaser"), ki18n(( "Tester, patches" )), "zerokode@gmx.net" );

    App::initCliArgs( argc, argv );
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start()) {
        fprintf(stderr, "Amarok is already running!\n");
        return 0;
    }

    App app;

#ifdef Q_WS_X11
    #ifndef AMAROK_USE_DRKONQI
    KCrash::setCrashHandler( Amarok::Crash::crashHandler );
    #endif
#endif

    return app.exec();
}
