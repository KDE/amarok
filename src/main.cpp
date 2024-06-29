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
#include <KCrash>
#include <KDBusService>
#include <Kdelibs4ConfigMigrator>
#include <Kdelibs4Migration>

#include <KLocalizedString>

#include <QCommandLineParser>
#include <QStandardPaths>
#include <QtGlobal>

#ifdef WITH_QT_WEBENGINE
#include <QtWebEngine>
#endif

#include <csignal>

#ifdef Q_OS_WIN
AMAROK_EXPORT OcsData ocsData;
#endif

int main( int argc, char *argv[] )
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    App app(argc, argv);

    app.setApplicationDisplayName(i18n("Amarok"));

    QCoreApplication::setApplicationName("amarok");
    QCoreApplication::setOrganizationDomain("kde.org");
    QCoreApplication::setApplicationVersion(AMAROK_VERSION);

    KCrash::initialize();

    Kdelibs4ConfigMigrator configMigrator(QStringLiteral("amarok"));
    configMigrator.setConfigFiles(QStringList()
                                  << QStringLiteral("amarokrc")
                                  << QStringLiteral("amarok_homerc")
                                  << QStringLiteral("amarok-appletsrc")
                                  );
    configMigrator.migrate();

    if (configMigrator.migrate()) {
        Kdelibs4Migration dataMigrator;
        const QString sourceBasePath = dataMigrator.saveLocation("data", QStringLiteral("amarok"));
        const QString targetBasePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/amarok/");
        QString targetFilePath;

        QDir sourceDir(sourceBasePath);
        QDir targetDir(targetBasePath);

        if (sourceDir.exists()) {
            if (!targetDir.exists()) {
                QDir().mkpath(targetBasePath);
            }
            QStringList fileNames = sourceDir.entryList(
                        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
            for(const QString &fileName : fileNames) {
                targetFilePath = targetBasePath + fileName;
                if (!QFile::exists(targetFilePath)) {
                    QFile::copy(sourceBasePath + fileName, targetFilePath);
                }
            }
        }
    }

    KAboutData aboutData( "amarok",
                          i18n( "Amarok" ),
                          AMAROK_VERSION,
                          i18n( "The audio player by KDE" ),
                          KAboutLicense::GPL,
                          i18n( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2024, The Amarok Development Squad" ),
                          ki18n( "IRC:\nirc.libera.chat - #amarok, #amarok-de, #amarok-es, #amarok-fr\n\nFeedback:\namarok@kde.org\n\n(Build Date: %1)" ).subs( __DATE__ ).toString(),
                          ( "http://amarok.kde.org" ) );

    //------------ About data ----------------------
    //Currently active Authors
    extern OcsData ocsData;

    //Inactive authors
    /* This list should contain people who still hold major copyright on the current code
     * For instance: does not include authors of 1.4 who have not contributed to 2.x */
    aboutData.addAuthor( i18n("<i>Inactive authors</i>"),
                         i18n("Amarok authorship is not a hobby, it's a lifestyle. "
                               "But when people move on we want to keep respecting "
                               "them by mentioning them here:"), "" );
    ocsData.addAuthor( "%%category%%", aboutData.authors().last() );
    // NOTE 2024: Moved Inactive author header up here to reflect the fact that this list hasn't been updated lately

    aboutData.addAuthor( i18n("Bart 'Where are my toothpicks' Cerneels"),
                         i18n("Developer (Stecchino)"), "bart.cerneels@kde.org", "http://commonideas.blogspot.com" );
    ocsData.addAuthor( "Stecchino", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Edward \"Hades\" Toroshchin"),
                         i18n("Developer (dr_lepper)"), "edward.hades@gmail.com" );
    ocsData.addAuthor( "hadeschief", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Mark Kretschmann" ),
                         i18n("Project founder (markey)"), "kretschmann@kde.org", "https://plus.google.com/102602725322221030250/posts" );
    ocsData.addAuthor( "MarkKretschmann", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Matěj Laitl"),
                         i18n("iPod collection rewrite & more (strohel)"), "matej@laitl.cz", "http://strohel.blogspot.com/" );
    ocsData.addAuthor( "strohel", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Myriam Schweingruber"), i18n("Rokymoter, Bug triaging (Mamarok)"), "myriam@kde.org", "http://blogs.fsfe.org/myriam" );
    ocsData.addAuthor( "Mamarok", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Ralf 'SalsaMaster' Engels"),
                         i18n("Developer (rengels)"), "ralf.engels@nokia.com" );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Patrick von Reth"), i18n("Windows build (TheOneRing)"),
                         "patrick.vonreth@gmail.com" );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Rick W. Chen"),
                         i18n("Developer (stuffcorpse)"), "stuffcorpse@archlinux.us" );
    ocsData.addAuthor( "stuffcorpse", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Sam Lade"), i18n("Developer (Sentynel)"),
                         "sam@sentynel.com" );
    ocsData.addAuthor( "Sentynel", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Sven Krohlas"), i18n("Rokymoter, Developer (sven423)"), "sven@asbest-online.de" );
    ocsData.addAuthor( "krohlas", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Téo Mrnjavac"),
                         i18n("Developer (Teo`)"), "teo@kde.org", "http://teom.wordpress.com/" );
    ocsData.addAuthor( "teom", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Valorie Zimmerman"),
                         i18n("Rokymoter, Handbook (valorie)"), "valorie@kde.org" );
    ocsData.addAuthor( "valorie", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Ian 'The Beard' Monroe"), i18n("Developer (eean)"), "ian@monroe.nu" );
    ocsData.addAuthor( "eean", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Jeff 'IROKSOHARD' Mitchell"), i18n("Developer (jefferai)"), "mitchell@kde.org" );
    ocsData.addAuthor( "jefferai", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Leo Franchi"), i18n("Developer (lfranchi)"), "lfranchi@kde.org" );
    ocsData.addAuthor( "lfranchi", aboutData.authors().last() );

    aboutData.addAuthor( i18n("Lydia 'is wrong(TM)' Pintscher"), i18n("Release Vixen (Nightrose)"), "lydia@kde.org" );
    ocsData.addAuthor( "nightrose", aboutData.authors().last() );

    aboutData.addCredit( i18n("Max Howell"), i18n("Developer, Vision"), "max.howell@methylblue.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );

    aboutData.addAuthor( i18n("Maximilian Kossick"), i18n("Developer (maxx_k)"), "maximilian.kossick@gmail.com" );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Nikolaj Hald 'Also very hot' Nielsen"), i18n("Developer (nhn)"), "nhn@kde.org" );
    ocsData.addAuthor( "nhnFreespirit", aboutData.authors().last() );

    aboutData.addCredit( i18n("Seb 'Surfin' down under' Ruiz"), i18n("Developer (sebr)"), "ruiz@kde.org" );
    ocsData.addCredit( "seb", aboutData.credits().last() );


    //Contributors
    aboutData.addCredit( i18n("Alejandro Wainzinger"), i18n("Developer (xevix)"), "aikawarazuni@gmail.com" );
    ocsData.addCredit( "xevix", aboutData.credits().last() );
    aboutData.addCredit( i18n("Alex Merry"), i18n("Developer, Replay Gain support"), "kde@randomguy3.me.uk" );
    ocsData.addCredit( "randomguy3", aboutData.credits().last() );
    aboutData.addCredit( i18n("Casey Link"), i18n("MP3tunes integration"), "unnamedrambler@gmail.com" );
    ocsData.addCredit( "Ramblurr", aboutData.credits().last() );
    aboutData.addCredit( i18n("Casper van Donderen"), i18n("Windows porting"), "casper.vandonderen@gmail.com" );
    ocsData.addCredit( "cvandonderen", aboutData.credits().last() );
    aboutData.addCredit( i18n("Christie Harris"), i18n("Rokymoter (dangle)"), "dangle.baby@gmail.com" );
    ocsData.addCredit( "dangle", aboutData.credits().last() );
    aboutData.addCredit( i18n("Dan Leinir Turthra Jensen"), i18n("Usability"), "admin@leinir.dk" );
    ocsData.addCredit( "leinir", aboutData.credits().last() );
    aboutData.addCredit( i18n("Dan 'Hey, it compiled...' Meltzer"), i18n("Developer (hydrogen)"), "parallelgrapefruit@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Daniel Caleb Jones"), i18n("Biased playlists"), "danielcjones@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Daniel Dewald"), i18n("Tag Guesser, Labels, Spectrum Analyzer"), "Daniel.Dewald@time-shift.de" );
    ocsData.addCredit( "TheCrasher", aboutData.credits().last() );
    aboutData.addCredit( i18n("Daniel Winter"), i18n("Nepomuk integration"), "dw@danielwinter.de" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Frank Meerkötter"), i18n("Podcast improvements"), "frank@meerkoetter.org" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Greg Meyer"), i18n("Live CD, Bug squashing (oggb4mp3)"), "greg@gkmweb.com" );
    ocsData.addCredit( "oggb4mp3", aboutData.credits().last() );
    aboutData.addCredit( i18n("Harald Sitter"), i18n("Phonon, Lord-President of KDE Multimedia (apachelogger)"), "harald.sitter@kdemail.net" );
    ocsData.addCredit( "apachelogger", aboutData.credits().last() );
    aboutData.addCredit( i18n("John Atkinson"), i18n("Assorted patches"), "john@fauxnetic.co.uk" );
    ocsData.addCredit( "fauxnetic", aboutData.credits().last() );
    aboutData.addCredit( i18n("Kenneth Wesley Wimer II"), i18n("Icons"), "kwwii@bootsplash.org" );
    ocsData.addCredit( "kwwii", aboutData.credits().last() );
    aboutData.addCredit( i18n("Kevin Funk"), i18n("Developer, Website theme (KRF)"), "krf@electrostorm.net" );
    ocsData.addCredit( "krf", aboutData.credits().last() );
    aboutData.addCredit( i18n("Kuba Serafinowski"), i18n("Rokymoter"), "zizzfizzix@gmail.com" );
    ocsData.addCredit( "zizzfizzix", aboutData.credits().last() );
    aboutData.addCredit( i18n("Lee Olson"), i18n("Artwork"), "leetolson@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Ljubomir Simin"), i18n("Rokymoter (ljubomir)"), "ljubomir.simin@gmail.com" );
    ocsData.addCredit( "ljubomir", aboutData.credits().last() );
    aboutData.addCredit( i18n("Lucas Gomes"), i18n("Developer (MaskMaster)"), "x8lucas8x@gmail.com" );
    ocsData.addCredit( "x8lucas8x", aboutData.credits().last() );
    aboutData.addCredit( i18n("Mathias Panzenböck"), i18n("Podcast improvements"), "grosser.meister.morti@gmx.net" );
    ocsData.addCredit( "panzi", aboutData.credits().last() );
    aboutData.addCredit( i18n("Mikko Caldara"), i18n("Bug triaging and sanitizing"), "mikko.cal@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Nikhil Marathe"), i18n("UPnP support and patches (nsm)"), "nsm.nikhil@gmail.com" );
    ocsData.addCredit( "nikhilm", aboutData.credits().last() );
    aboutData.addCredit( i18n("Nuno Pinheiro"), i18n("Artwork"), "nuno@oxygen-icons.org" );
    ocsData.addCredit( "nunopinheirokde", aboutData.credits().last() );
    aboutData.addCredit( i18n("Olivier Bédard"), i18n("Website hosting"), "paleo@pwsp.net" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Pasi Lalinaho"), i18n("Rokymoter (emunkki)"), "pasi@getamarok.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Peter Zhou Lei"), i18n("Scripting interface"), "peterzhoulei@gmail.com" );
    ocsData.addCredit( "peterzl", aboutData.credits().last() );
    aboutData.addCredit( i18n("Phalgun Guduthur"), i18n("Nepomuk Collection (phalgun)"), "me@phalgun.in" );
    ocsData.addCredit( "phalgun", aboutData.credits().last() );
    aboutData.addCredit( i18n("Scott Wheeler"), i18n("TagLib & ktrm code"), "wheeler@kde.org" );
    ocsData.addCredit( "wheels", aboutData.credits().last() );
    aboutData.addCredit( i18n("Shane King"), i18n("Patches & Windows porting (shakes)"), "kde@dontletsstart.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Simon Esneault"), i18n("Photos & Videos applets, Context View"), "simon.esneault@gmail.com" );
    ocsData.addCredit( "Takahani", aboutData.credits().last() );
    aboutData.addCredit( i18n("Soren Harward"), i18n("Developer, Automated Playlist Generator"), "stharward@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Thomas Lübking"), i18n("Developer"), "thomas.luebking@web.de" );
    ocsData.addCredit( "thomas12777", aboutData.credits().last() );
    aboutData.addCredit( i18n("Valentin Rouet"), i18n("Developer"), "v.rouet@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Wade Olson"), i18n("Splash screen artist"), "wade@corefunction.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("William Viana Soares"), i18n("Context view"), "vianasw@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );

    //Former Contributors
    aboutData.addCredit( i18n("Former contributors"), i18n("People listed below have contributed to Amarok in the past. Thank you!"), "" );
    ocsData.addCredit( "%%category%%", aboutData.credits().last() );
    aboutData.addCredit( i18n("Adam Pigg"), i18n("Analyzers, patches, shoutcast"), "adam@piggz.co.uk" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Adeodato Simó"), i18n("Patches"), "asp16@alu.ua.es" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Alexandre Oliveira"), i18n("Developer"), "aleprj@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andreas Mair"), i18n("MySQL support"), "am_ml@linogate.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andrew de Quincey"), i18n("Postgresql support"), "adq_dvb@lidskialf.net" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andrew Turner"), i18n("Patches"), "andrewturner512@googlemail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andy Kelk"), i18n("MTP and Rio Karma media devices, patches"), "andy@mopoke.co.uk" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Christian Muehlhaeuser"), i18n("Developer"), "chris@chris.de" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Derek Nelson"), i18n("Graphics, splash-screen"), "admrla@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Enrico Ros"), i18n("Analyzers, Context Browser and systray eye-candy"), "eros.kde@email.it" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Frederik Holljen"), i18n("Developer"), "fh@ez.no" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Gábor Lehel"), i18n("Developer"), "illissius@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Gérard Dürrmeyer"), i18n("Icons and image work"), "gerard@randomtree.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Giovanni Venturi"), i18n("Dialog to filter the collection titles"), "giovanni@ksniffer.org" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Jarkko Lehti"), i18n("Tester, IRC channel operator, whipping"), "grue@iki.fi" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Jocke Andersson"), i18n("Rokymoter, bug fixer (Firetech)"), "ajocke@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Marco Gulino"), i18n("Konqueror Sidebar, some DCOP methods"), "marco@kmobiletools.org" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Martin Aumueller"), i18n("Developer"), "aumuell@reserv.at" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Melchior Franz"), i18n("FHT routine, bugfixes"), "mfranz@kde.org" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Michael Pyne"), i18n("K3b export code"), "michael.pyne@kdemail.net" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Mike Diehl"), i18n("Developer"), "madpenguin8@yahoo.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Paul Cifarelli"), i18n("Developer"), "paul@cifarelli.net" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Peter C. Ndikuwera"), i18n("Bugfixes, PostgreSQL support"), "pndiku@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Pierpaolo Panfilo"), i18n("Developer"), "pippo_dp@libero.it" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Reigo Reinmets"), i18n("Wikipedia support, patches"), "xatax@hot.ee" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Roman Becker"), i18n("Former Amarok logo, former splash screen, former icons"), "roman@formmorf.de" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Sami Nieminen"), i18n("Audioscrobbler support"), "sami.nieminen@iki.fi" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Stanislav Karchebny"), i18n("Developer"), "berkus@madfire.net" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Stefan Bogner"), i18n("Loads of stuff"), "bochi@online.ms" );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Tomasz Dudzik"), i18n("Splash screen"), "madsheytan@gmail.com" );
    ocsData.addCredit( QString(), aboutData.credits().last() );

    //Donors:
    //Last update: 2012/11/07, post Roktober 2012 //TODO possibly remove in Amarok 4
    ocsData.addDonor( "ayleph", KAboutPerson( i18n( "Andrew Browning" ) ) );
    ocsData.addDonor( QString(), KAboutPerson( i18n( "Chris Wales" ) ) );
    ocsData.addDonor( QString(), KAboutPerson( i18n( "ZImin Stanislav" ) ) );

    KAboutData::setApplicationData(aboutData);

    // Command line parser
    QCommandLineParser parser;

    aboutData.setupCommandLine(&parser);
    app.initCliArgs(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService::StartupOptions startOptions = parser.isSet( "multipleinstances" ) ? KDBusService::Multiple
                                                                                    : KDBusService::Unique ;
    // register  the app  to dbus
    KDBusService dbusService( startOptions );

    QObject::connect(&dbusService, &KDBusService::activateRequested,
                     &app, &App::activateRequested);

    const bool debugColorsEnabled = !parser.isSet( "coloroff" );
    const bool debugEnabled = parser.isSet( "debug" ) || parser.isSet( "debug-with-lastfm" ); // HACK see App::initCliArgs

    Debug::setDebugEnabled( debugEnabled );
    Debug::setColoredDebug( debugColorsEnabled );

    if ( parser.isSet( "debug-audio" ) ) {
        qputenv( "PHONON_DEBUG", QByteArray( "3" ) );
        qputenv( "PHONON_BACKEND_DEBUG", QByteArray( "3" ) );
        qputenv( "PHONON_PULSEAUDIO_DEBUG", QByteArray( "3" ) );
    }

#pragma message("PORT KF5: This *if* should be moved to activateRequested() slot")
    if( !dbusService.isRegistered() ) {
        QList<QByteArray> instanceOptions;
        instanceOptions << "previous" << "play" << "play-pause" << "stop" << "next"
                        << "append" << "queue" << "load";
        // Check if an option for a running instance is set
        bool isSet = false;
        for( int i = 0; i < instanceOptions.size(); ++i )
            if( parser.isSet( instanceOptions[ i ] ) )
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

    app.continueInit();
    return app.exec();
}

