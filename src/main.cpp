/****************************************************************************************
 * Copyright (c) 2002 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "Amarok.h"
#include "App.h"
#include "aboutdialog/OcsData.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>

//#define AMAROK_USE_DRKONQI

extern AMAROK_EXPORT class KAboutData aboutData; //defined in App.cpp
extern AMAROK_EXPORT class OcsData ocsData;


int main( int argc, char *argv[] )
{

    //Authors
    aboutData.addAuthor( ki18n("Alejandro Wainzinger"),
            ki18n("Developer (xevix)"), "aikawarazuni@gmail.com", "http://awainzin-foss.blogspot.com" );
    aboutData.addAuthor( ki18n("Bart 'Where are my toothpicks' Cerneels"),
            ki18n("Developer (Stecchino)"), "bart.cerneels@kde.org", "http://commonideas.blogspot.com" );
    aboutData.addAuthor( ki18n("Dan 'Hey, it compiled...' Meltzer"),
            ki18n("Developer (hydrogen)"), "parallelgrapefruit@gmail.com" );
    aboutData.addAuthor( ki18n("Ian 'The Beard' Monroe"),
            ki18n("Developer (eean)"), "ian@monroe.nu", "http://www.monroe.nu/" );
    aboutData.addAuthor( ki18n("Jeff 'IROKSOHARD' Mitchell"),
            ki18n("Developer (jefferai)"), "mitchell@kde.org", "http://amarok.kde.org/blog/categories/13-jefferai" );
    aboutData.addAuthor( ki18n("Leo Franchi"),
            ki18n("Developer (lfranchi)"), "lfranchi@kde.org" );
    aboutData.addAuthor( ki18n("Lydia 'is wrong(TM)' Pintscher"),
            ki18n("Release Vixen (Nightrose)"), "lydia@kde.org" );
    aboutData.addAuthor( ki18n("Mark 'It's good, but it's not irssi' Kretschmann"), //krazy:exclude=contractions
            ki18n("Project founder (markey)"), "kretschmann@kde.org" );
    aboutData.addAuthor( ki18n("Maximilian Kossick"),
            ki18n("Developer (maxx_k)"), "maximilian.kossick@gmail.com" );
    aboutData.addAuthor( ki18n("Nikolaj Hald 'Also very hot' Nielsen"),
            ki18n("Developer (nhn)"), "nhnfreespirit@gmail.com", "http://amarok.kde.org/blog/categories/18-freespirit" );
    aboutData.addAuthor( ki18n("Seb 'Surfin' down under' Ruiz"),
            ki18n("Developer (sebr)"), "ruiz@kde.org", "http://www.sebruiz.net" );
    aboutData.addAuthor( ki18n("Téo Mrnjavac"),
            ki18n("Developer (Teo`)"), "teo.mrnjavac@gmail.com", "http://teom.wordpress.com/" );

    ocsData.addAuthor( "xevix", aboutData.authors().at( 0 ) );
    ocsData.addAuthor( "Stecchino", aboutData.authors().at( 1 ) );
    ocsData.addAuthor( QString(), aboutData.authors().at( 2 ) );
    ocsData.addAuthor( "eean", aboutData.authors().at( 3 ) );
    ocsData.addAuthor( "jefferai", aboutData.authors().at( 4 ) );
    ocsData.addAuthor( "lfranchi", aboutData.authors().at( 5 ) );
    ocsData.addAuthor( "nightrose", aboutData.authors().at( 6 ) );
    ocsData.addAuthor( "MarkKretschmann", aboutData.authors().at( 7 ) );
    ocsData.addAuthor( QString(), aboutData.authors().at( 8 ) );
    ocsData.addAuthor( "nhnFreespirit", aboutData.authors().at( 9 ) );
    ocsData.addAuthor( "seb", aboutData.authors().at( 10 ) );
    ocsData.addAuthor( "teom", aboutData.authors().at( 11 ) );

    //Contributors
    aboutData.addCredit( ki18n("Alex Merry"), ki18n("Developer, Replay gain support"), "kde@randomguy3.me.uk" );
        ocsData.addCredit( "randomguy3", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Casey Link"), ki18n("MP3tunes integration"), "unnamedrambler@gmail.com" );
        ocsData.addCredit( "Ramblurr", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Casper van Donderen"), ki18n("Windows porting"), "casper.vandonderen@gmail.com" );
        ocsData.addCredit( "cvandonderen", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Christie Harris"), ki18n("Rokymoter (dangle)"), "dangle.baby@gmail.com" );
        ocsData.addCredit( "dangle", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Dan Leinir Turthra Jensen"), ki18n("Usability"), "admin@leinir.dk" );
        ocsData.addCredit( "leinir", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Daniel Caleb Jones"), ki18n("Biased playlists"), "danielcjones@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Daniel Dewald"), ki18n("Tag Guesser, Labels"), "Daniel.Dewald@time-shift.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Daniel Winter"), ki18n("Nepomuk integration"), "dw@danielwinter.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Edward \"Hades\" Toroshchin"), ki18n("Developer"), "edward.hades@gmail.com" );
        ocsData.addCredit( "hadeschief", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Greg Meyer"), ki18n("Live CD, Bug squashing (oggb4mp3)"), "greg@gkmweb.com" );
        ocsData.addCredit( "oggb4mp3", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Harald Sitter"), ki18n("Rokymoter (apachelogger)"), "harald.sitter@kdemail.net" );
        ocsData.addCredit( "apachelogger", aboutData.credits().last() );
    aboutData.addCredit( ki18n("John Atkinson"), ki18n(( "Assorted patches" )), "john@fauxnetic.co.uk" );
        ocsData.addCredit( "fauxnetic", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Kenneth Wesley Wimer II"), ki18n("Icons"), "kwwii@bootsplash.org" );
        ocsData.addCredit( "kwwii", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Kevin Funk"), ki18n("Developer, Website theme (KRF)"), "krf@electrostorm.net" );
        ocsData.addCredit( "krf", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Kuba Serafinowski"), ki18n("Rokymoter"), "zizzfizzix@gmail.com" );
        ocsData.addCredit( "zizzfizzix", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Lee Olson"), ki18n("Artwork"), "leetolson@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Ljubomir Simin"), ki18n("Rokymoter (ljubomir)"), "ljubomir.simin@gmail.com" );
        ocsData.addCredit( "ljubomir", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Max Howell"), ki18n("Developer, Vision"), "max.howell@methylblue.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Mikko Caldara"), ki18n("Bug triaging and sanitizing"), "mikko.cal@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Myriam Schweingruber"), ki18n("Rokymoter, bug squashing (Mamarok)"), "myriam@kde.org" );
        ocsData.addCredit( "Mamarok", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Nuno Pinheiro"), ki18n("Artwork"), "nuno@oxygen-icons.org" );
        ocsData.addCredit( "nunopinheirokde", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Olivier Bédard"), ki18n("Website hosting"), "paleo@pwsp.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Pasi Lalinaho"), ki18n("Rokymoter (emunkki)"), "pasi@getamarok.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Peter Zhou Lei"), ki18n("Scripting interface"), "peterzhoulei@gmail.com" );
        ocsData.addCredit( "peterzl", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Scott Wheeler"), ki18n("TagLib & ktrm code"), "wheeler@kde.org" );
        ocsData.addCredit( "wheels", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Shane King"), ki18n("Patches & Windows porting (shakes)"), "kde@dontletsstart.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Simon Esneault"), ki18n("Photos & Videos applets, Context View"), "simon.esneault@gmail.com" );
        ocsData.addCredit( "Takahani", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Soren Harward"), ki18n("Developer"), "stharward@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Sven Krohlas"), ki18n("Rokymoter, Developer (sven423)"), "sven@asbest-online.de" );
        ocsData.addCredit( "krohlas", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Thomas Lübking"), ki18n("Developer"), "thomas.luebking@web.de" );
        ocsData.addCredit( "thomas12777", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Valentin Rouet"), ki18n("Developer"), "v.rouet@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Wade Olson"), ki18n("Splash screen artist"), "wade@corefunction.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("William Viana Soares"), ki18n("Context view"), "vianasw@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Former contributors"), ki18n("People listed below have contributed to Amarok in the past. Thank you!"), "" );
        ocsData.addCredit( "%%category%%", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Adam Pigg"), ki18n("Analyzers, patches, shoutcast"), "adam@piggz.co.uk" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Adeodato Simó"), ki18n("Patches"), "asp16@alu.ua.es" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Alexandre Oliveira"), ki18n("Developer"), "aleprj@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andreas Mair"), ki18n("MySQL support"), "am_ml@linogate.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andrew de Quincey"), ki18n("Postgresql support"), "adq_dvb@lidskialf.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andrew Turner"), ki18n("Patches"), "andrewturner512@googlemail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andy Kelk"), ki18n("MTP and Rio Karma media devices, patches"), "andy@mopoke.co.uk" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Christian Muehlhaeuser"), ki18n("Developer"), "chris@chris.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Derek Nelson"), ki18n("Graphics, splash-screen"), "admrla@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Enrico Ros"), ki18n("Analyzers, Context Browser and systray eye-candy"), "eros.kde@email.it" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Frederik Holljen"), ki18n("Developer"), "fh@ez.no" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Gábor Lehel"), ki18n("Developer"), "illissius@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Gérard Dürrmeyer"), ki18n("Icons and image work"), "gerard@randomtree.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Giovanni Venturi"), ki18n("Dialog to filter the collection titles"), "giovanni@ksniffer.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Jarkko Lehti"), ki18n("Tester, IRC channel operator, whipping"), "grue@iki.fi" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Jocke Andersson"), ki18n("Rokymoter, bug fixer (Firetech)"), "ajocke@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Marco Gulino"), ki18n("Konqueror Sidebar, some DCOP methods"), "marco@kmobiletools.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Martin Aumueller"), ki18n("Developer"), "aumuell@reserv.at" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Melchior Franz"), ki18n("FHT routine, bugfixes"), "mfranz@kde.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Michael Pyne"), ki18n("K3b export code"), "michael.pyne@kdemail.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Mike Diehl"), ki18n("Developer"), "madpenguin8@yahoo.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Paul Cifarelli"), ki18n("Developer"), "paul@cifarelli.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Peter C. Ndikuwera"), ki18n("Bugfixes, PostgreSQL support"), "pndiku@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Pierpaolo Panfilo"), ki18n("Developer"), "pippo_dp@libero.it" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Reigo Reinmets"), ki18n("Wikipedia support, patches"), "xatax@hot.ee" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Roman Becker"), ki18n("Former Amarok logo, former splash screen, former icons"), "roman@formmorf.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Sami Nieminen"), ki18n("Audioscrobbler support"), "sami.nieminen@iki.fi" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Stanislav Karchebny"), ki18n("Developer"), "berkus@madfire.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Stefan Bogner"), ki18n("Loads of stuff"), "bochi@online.ms" );
        ocsData.addCredit( QString(), aboutData.credits().last() );

    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &::aboutData ); //calls KCmdLineArgs::addStdCmdLineOptions()

    App::initCliArgs();
    KUniqueApplication::addCmdLineOptions();

    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    KUniqueApplication::StartFlag startFlag;
    startFlag = args->isSet( "multipleinstances" ) ? KUniqueApplication::NonUniqueInstance : KUniqueApplication::StartFlag( 0 );

    if( !KUniqueApplication::start( startFlag ) ) {
        QList<QByteArray> instanceOptions;
        instanceOptions << "previous" << "play" << "play-pause" << "stop" << "next" << "append" << "queue" << "load" << "subscribe";

        // Check if an option for a running instance is set
        bool isSet = false;
        for( int i = 0; i < instanceOptions.size(); ++i )
            if( args->isSet( instanceOptions[ i ] ) )
                isSet = true;

        if ( !isSet )
            fprintf( stderr, "Amarok is already running!\n" );
        return 0;
    }

    App app;
    app.setUniqueInstance( startFlag == KUniqueApplication::NonUniqueInstance );
    return app.exec();
}

