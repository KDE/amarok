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

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "App.h"
#include "aboutdialog/OcsData.h"

#include <KAboutData>
#include <KCmdLineArgs>
#include <KDebug>

#include <qglobal.h>

#ifdef Q_WS_X11
    #include <X11/Xlib.h>
#endif

#include <csignal>

//#define AMAROK_USE_DRKONQI
#ifdef Q_OS_WIN
AMAROK_EXPORT OcsData ocsData;
#endif

int main( int argc, char *argv[] )
{
    KAboutData aboutData(
        "amarok",
        ki18n( "Amarok" ).toString(),
        AMAROK_VERSION,
        ki18n( "The audio player for KDE" ).toString(),
        KAboutLicense::GPL,
        ki18n( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2013, The Amarok Development Squad" ).toString(),
        ki18n( "IRC:\nirc.freenode.net - #amarok, #amarok.de, #amarok.es, #amarok.fr\n\nFeedback:\namarok@kde.org\n\n(Build Date: %1)" ).subs( __DATE__ ).toString(),
        ( "http://amarok.kde.org" ) );

    //Currently active Authors
    extern OcsData ocsData;
    aboutData.addAuthor( ki18n("Bart 'Where are my toothpicks' Cerneels").toString(),
            ki18n("Developer (Stecchino)").toString(), "bart.cerneels@kde.org", "http://commonideas.blogspot.com" );
    ocsData.addAuthor( "Stecchino", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Edward \"Hades\" Toroshchin").toString(),
            ki18n("Developer (dr_lepper)").toString(), "edward.hades@gmail.com" );
    ocsData.addAuthor( "hadeschief", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Mark Kretschmann" ).toString(),
            ki18n("Project founder (markey)").toString(), "kretschmann@kde.org", "https://plus.google.com/102602725322221030250/posts" );
        ocsData.addAuthor( "MarkKretschmann", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Matěj Laitl").toString(),
            ki18n("iPod collection rewrite & more (strohel)").toString(), "matej@laitl.cz", "http://strohel.blogspot.com/" );
    ocsData.addAuthor( "strohel", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Myriam Schweingruber").toString(), ki18n("Rokymoter, Bug triaging (Mamarok)").toString(), "myriam@kde.org", "http://blogs.fsfe.org/myriam" );
    ocsData.addAuthor( "Mamarok", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Ralf 'SalsaMaster' Engels").toString(),
            ki18n("Developer (rengels)").toString(), "ralf.engels@nokia.com" );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Patrick von Reth").toString(), ki18n("Windows build (TheOneRing)").toString(),
            "patrick.vonreth@gmail.com" );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Rick W. Chen").toString(),
            ki18n("Developer (stuffcorpse)").toString(), "stuffcorpse@archlinux.us" );
    ocsData.addAuthor( "stuffcorpse", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Sam Lade").toString(), ki18n("Developer (Sentynel)").toString(),
            "sam@sentynel.com" );
    ocsData.addAuthor( "Sentynel", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Sven Krohlas").toString(), ki18n("Rokymoter, Developer (sven423)").toString(), "sven@asbest-online.de" );
    ocsData.addAuthor( "krohlas", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Téo Mrnjavac").toString(),
            ki18n("Developer (Teo`)").toString(), "teo@kde.org", "http://teom.wordpress.com/" );
    ocsData.addAuthor( "teom", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Valorie Zimmerman").toString(),
            ki18n("Rokymoter, Handbook (valorie)").toString(), "valorie@kde.org" );
    ocsData.addAuthor( "valorie", aboutData.authors().last() );

    //Inactive authors
    /* This list should contain people who still hold major copyright on the current code
     * For instance: does not include authors of 1.4 who have not contributed to 2.x */
    aboutData.addAuthor( ki18n("<i>Inactive authors</i>").toString(),
                         ki18n("Amarok authorship is not a hobby, it's a lifestyle. "
                               "But when people move on we want to keep respecting "
                               "them by mentioning them here:").toString(), "" );
        ocsData.addAuthor( "%%category%%", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Ian 'The Beard' Monroe").toString(), ki18n("Developer (eean)").toString(), "ian@monroe.nu" );
        ocsData.addAuthor( "eean", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Jeff 'IROKSOHARD' Mitchell").toString(), ki18n("Developer (jefferai)").toString(), "mitchell@kde.org" );
        ocsData.addAuthor( "jefferai", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Leo Franchi").toString(), ki18n("Developer (lfranchi)").toString(), "lfranchi@kde.org" );
        ocsData.addAuthor( "lfranchi", aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Lydia 'is wrong(TM)' Pintscher").toString(), ki18n("Release Vixen (Nightrose)").toString(), "lydia@kde.org" );
        ocsData.addAuthor( "nightrose", aboutData.authors().last() );

    aboutData.addCredit( ki18n("Max Howell").toString(), ki18n("Developer, Vision").toString(), "max.howell@methylblue.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );

    aboutData.addAuthor( ki18n("Maximilian Kossick").toString(), ki18n("Developer (maxx_k)").toString(), "maximilian.kossick@gmail.com" );
        ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( ki18n("Nikolaj Hald 'Also very hot' Nielsen").toString(), ki18n("Developer (nhn)").toString(), "nhn@kde.org" );
        ocsData.addAuthor( "nhnFreespirit", aboutData.authors().last() );

    aboutData.addCredit( ki18n("Seb 'Surfin' down under' Ruiz").toString(), ki18n("Developer (sebr)").toString(), "ruiz@kde.org" );
           ocsData.addCredit( "seb", aboutData.credits().last() );


    //Contributors
    aboutData.addCredit( ki18n("Alejandro Wainzinger").toString(), ki18n("Developer (xevix)").toString(), "aikawarazuni@gmail.com" );
        ocsData.addCredit( "xevix", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Alex Merry").toString(), ki18n("Developer, Replay Gain support").toString(), "kde@randomguy3.me.uk" );
        ocsData.addCredit( "randomguy3", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Casey Link").toString(), ki18n("MP3tunes integration").toString(), "unnamedrambler@gmail.com" );
        ocsData.addCredit( "Ramblurr", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Casper van Donderen").toString(), ki18n("Windows porting").toString(), "casper.vandonderen@gmail.com" );
        ocsData.addCredit( "cvandonderen", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Christie Harris").toString(), ki18n("Rokymoter (dangle)").toString(), "dangle.baby@gmail.com" );
        ocsData.addCredit( "dangle", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Dan Leinir Turthra Jensen").toString(), ki18n("Usability").toString(), "admin@leinir.dk" );
        ocsData.addCredit( "leinir", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Dan 'Hey, it compiled...' Meltzer").toString(), ki18n("Developer (hydrogen)").toString(), "parallelgrapefruit@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Daniel Caleb Jones").toString(), ki18n("Biased playlists").toString(), "danielcjones@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Daniel Dewald").toString(), ki18n("Tag Guesser, Labels, Spectrum Analyzer").toString(), "Daniel.Dewald@time-shift.de" );
        ocsData.addCredit( "TheCrasher", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Daniel Winter").toString(), ki18n("Nepomuk integration").toString(), "dw@danielwinter.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
      aboutData.addCredit( ki18n("Frank Meerkötter").toString(), ki18n("Podcast improvements").toString(), "frank@meerkoetter.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Greg Meyer").toString(), ki18n("Live CD, Bug squashing (oggb4mp3)").toString(), "greg@gkmweb.com" );
        ocsData.addCredit( "oggb4mp3", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Harald Sitter").toString(), ki18n("Phonon, Lord-President of KDE Multimedia (apachelogger)").toString(), "harald.sitter@kdemail.net" );
        ocsData.addCredit( "apachelogger", aboutData.credits().last() );
    aboutData.addCredit( ki18n("John Atkinson").toString(), ki18n("Assorted patches").toString(), "john@fauxnetic.co.uk" );
        ocsData.addCredit( "fauxnetic", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Kenneth Wesley Wimer II").toString(), ki18n("Icons").toString(), "kwwii@bootsplash.org" );
        ocsData.addCredit( "kwwii", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Kevin Funk").toString(), ki18n("Developer, Website theme (KRF)").toString(), "krf@electrostorm.net" );
        ocsData.addCredit( "krf", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Kuba Serafinowski").toString(), ki18n("Rokymoter").toString(), "zizzfizzix@gmail.com" );
        ocsData.addCredit( "zizzfizzix", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Lee Olson").toString(), ki18n("Artwork").toString(), "leetolson@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Ljubomir Simin").toString(), ki18n("Rokymoter (ljubomir)").toString(), "ljubomir.simin@gmail.com" );
        ocsData.addCredit( "ljubomir", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Lucas Gomes").toString(), ki18n("Developer (MaskMaster)").toString(), "x8lucas8x@gmail.com" );
        ocsData.addCredit( "x8lucas8x", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Mathias Panzenböck").toString(), ki18n("Podcast improvements").toString(), "grosser.meister.morti@gmx.net" );
        ocsData.addCredit( "panzi", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Mikko Caldara").toString(), ki18n("Bug triaging and sanitizing").toString(), "mikko.cal@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Nikhil Marathe").toString(), ki18n("UPnP support and patches (nsm)").toString(), "nsm.nikhil@gmail.com" );
        ocsData.addCredit( "nikhilm", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Nuno Pinheiro").toString(), ki18n("Artwork").toString(), "nuno@oxygen-icons.org" );
        ocsData.addCredit( "nunopinheirokde", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Olivier Bédard").toString(), ki18n("Website hosting").toString(), "paleo@pwsp.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Pasi Lalinaho").toString(), ki18n("Rokymoter (emunkki)").toString(), "pasi@getamarok.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Peter Zhou Lei").toString(), ki18n("Scripting interface").toString(), "peterzhoulei@gmail.com" );
        ocsData.addCredit( "peterzl", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Phalgun Guduthur").toString(), ki18n("Nepomuk Collection (phalgun)").toString(), "me@phalgun.in" );
        ocsData.addCredit( "phalgun", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Scott Wheeler").toString(), ki18n("TagLib & ktrm code").toString(), "wheeler@kde.org" );
        ocsData.addCredit( "wheels", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Shane King").toString(), ki18n("Patches & Windows porting (shakes)").toString(), "kde@dontletsstart.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Simon Esneault").toString(), ki18n("Photos & Videos applets, Context View").toString(), "simon.esneault@gmail.com" );
        ocsData.addCredit( "Takahani", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Soren Harward").toString(), ki18n("Developer, Automated Playlist Generator").toString(), "stharward@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Thomas Lübking").toString(), ki18n("Developer").toString(), "thomas.luebking@web.de" );
        ocsData.addCredit( "thomas12777", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Valentin Rouet").toString(), ki18n("Developer").toString(), "v.rouet@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Wade Olson").toString(), ki18n("Splash screen artist").toString(), "wade@corefunction.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("William Viana Soares").toString(), ki18n("Context view").toString(), "vianasw@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );

    //Former Contributors
    aboutData.addCredit( ki18n("Former contributors").toString(), ki18n("People listed below have contributed to Amarok in the past. Thank you!").toString(), "" );
        ocsData.addCredit( "%%category%%", aboutData.credits().last() );
    aboutData.addCredit( ki18n("Adam Pigg").toString(), ki18n("Analyzers, patches, shoutcast").toString(), "adam@piggz.co.uk" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Adeodato Simó").toString(), ki18n("Patches").toString(), "asp16@alu.ua.es" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Alexandre Oliveira").toString(), ki18n("Developer").toString(), "aleprj@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andreas Mair").toString(), ki18n("MySQL support").toString(), "am_ml@linogate.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andrew de Quincey").toString(), ki18n("Postgresql support").toString(), "adq_dvb@lidskialf.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andrew Turner").toString(), ki18n("Patches").toString(), "andrewturner512@googlemail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Andy Kelk").toString(), ki18n("MTP and Rio Karma media devices, patches").toString(), "andy@mopoke.co.uk" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Christian Muehlhaeuser").toString(), ki18n("Developer").toString(), "chris@chris.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Derek Nelson").toString(), ki18n("Graphics, splash-screen").toString(), "admrla@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Enrico Ros").toString(), ki18n("Analyzers, Context Browser and systray eye-candy").toString(), "eros.kde@email.it" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Frederik Holljen").toString(), ki18n("Developer").toString(), "fh@ez.no" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Gábor Lehel").toString(), ki18n("Developer").toString(), "illissius@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Gérard Dürrmeyer").toString(), ki18n("Icons and image work").toString(), "gerard@randomtree.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Giovanni Venturi").toString(), ki18n("Dialog to filter the collection titles").toString(), "giovanni@ksniffer.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Jarkko Lehti").toString(), ki18n("Tester, IRC channel operator, whipping").toString(), "grue@iki.fi" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Jocke Andersson").toString(), ki18n("Rokymoter, bug fixer (Firetech)").toString(), "ajocke@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Marco Gulino").toString(), ki18n("Konqueror Sidebar, some DCOP methods").toString(), "marco@kmobiletools.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Martin Aumueller").toString(), ki18n("Developer").toString(), "aumuell@reserv.at" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Melchior Franz").toString(), ki18n("FHT routine, bugfixes").toString(), "mfranz@kde.org" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Michael Pyne").toString(), ki18n("K3b export code").toString(), "michael.pyne@kdemail.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Mike Diehl").toString(), ki18n("Developer").toString(), "madpenguin8@yahoo.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Paul Cifarelli").toString(), ki18n("Developer").toString(), "paul@cifarelli.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Peter C. Ndikuwera").toString(), ki18n("Bugfixes, PostgreSQL support").toString(), "pndiku@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Pierpaolo Panfilo").toString(), ki18n("Developer").toString(), "pippo_dp@libero.it" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Reigo Reinmets").toString(), ki18n("Wikipedia support, patches").toString(), "xatax@hot.ee" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Roman Becker").toString(), ki18n("Former Amarok logo, former splash screen, former icons").toString(), "roman@formmorf.de" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Sami Nieminen").toString(), ki18n("Audioscrobbler support").toString(), "sami.nieminen@iki.fi" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Stanislav Karchebny").toString(), ki18n("Developer").toString(), "berkus@madfire.net" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Stefan Bogner").toString(), ki18n("Loads of stuff").toString(), "bochi@online.ms" );
        ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( ki18n("Tomasz Dudzik").toString(), ki18n("Splash screen").toString(), "madsheytan@gmail.com" );
        ocsData.addCredit( QString(), aboutData.credits().last() );

    //Donors:
    //Last update: 2012/11/07, post Roktober 2012
    ocsData.addDonor( "ayleph", KAboutPerson( ki18n( "Andrew Browning" ).toString() ) );
    ocsData.addDonor( QString(), KAboutPerson( ki18n( "Chris Wales" ).toString() ) );
    ocsData.addDonor( QString(), KAboutPerson( ki18n( "ZImin Stanislav" ).toString() ) );

    KCmdLineArgs::reset();
    KCmdLineArgs::init( argc, argv, &aboutData ); //calls KCmdLineArgs::addStdCmdLineOptions()

    App::initCliArgs();
    KUniqueApplication::addCmdLineOptions();

    KCmdLineArgs* const args = KCmdLineArgs::parsedArgs();

    KUniqueApplication::StartFlag startFlag;
    startFlag = args->isSet( "multipleinstances" )
                ? KUniqueApplication::NonUniqueInstance
                : KUniqueApplication::StartFlag( 0 );

    const bool debugColorsEnabled = !args->isSet( "coloroff" );
    const bool debugEnabled = args->isSet( "debug" );

    Debug::setDebugEnabled( debugEnabled );
    Debug::setColoredDebug( debugColorsEnabled );

    if ( args->isSet( "debug-audio" ) ) {
        qputenv( "PHONON_DEBUG", QByteArray( "3" ) );
        qputenv( "PHONON_BACKEND_DEBUG", QByteArray( "3" ) );
        qputenv( "PHONON_PULSEAUDIO_DEBUG", QByteArray( "3" ) );
    }

    if( !KUniqueApplication::start( startFlag ) ) {
        QList<QByteArray> instanceOptions;
        instanceOptions << "previous" << "play" << "play-pause" << "stop" << "next"
                << "append" << "queue" << "load";

        // Check if an option for a running instance is set
        bool isSet = false;
        for( int i = 0; i < instanceOptions.size(); ++i )
            if( args->isSet( instanceOptions[ i ] ) )
                isSet = true;

        if ( !isSet )
            fprintf( stderr, "Amarok is already running!\n" );
        return 0;
    }

    // Rewrite default SIGINT and SIGTERM handlers
    // to make amarok save current playlists during forced
    // application termination (logout, Ctr+C in console etc.)
    signal( SIGINT, &QCoreApplication::exit );
    signal( SIGTERM, &QCoreApplication::exit );

    // This call is needed to prevent a crash on exit with Phonon-VLC and LibPulse
#ifdef Q_WS_X11
    XInitThreads();
#endif

    App app;
    app.setUniqueInstance( startFlag == KUniqueApplication::NonUniqueInstance );
    return app.exec();
}

