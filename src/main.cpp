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

    QCoreApplication::setApplicationName(QStringLiteral("amarok"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QCoreApplication::setApplicationVersion(QStringLiteral(AMAROK_VERSION));

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

    KAboutData aboutData( QStringLiteral("amarok"),
                          i18n( "Amarok" ),
                          QStringLiteral(AMAROK_VERSION),
                          i18n( "The audio player by KDE" ),
                          KAboutLicense::GPL,
                          i18n( "(C) 2002-2003, Mark Kretschmann\n(C) 2003-2024, The Amarok Development Squad" ),
                          ki18n( "IRC:\nirc.libera.chat - #amarok, #amarok-de, #amarok-es, #amarok-fr\n\nFeedback:\namarok@kde.org\n\n(Build Date: %1)" ).subs( QStringLiteral(__DATE__) ).toString(),
                          QStringLiteral( "http://amarok.kde.org" ) );

    //------------ About data ----------------------
    //Currently active Authors
    extern OcsData ocsData;

    //Inactive authors
    /* This list should contain people who still hold major copyright on the current code
     * For instance: does not include authors of 1.4 who have not contributed to 2.x */
    aboutData.addAuthor( i18n("<i>Inactive authors</i>"),
                         i18n("Amarok authorship is not a hobby, it's a lifestyle. "
                               "But when people move on we want to keep respecting "
                               "them by mentioning them here:"), QStringLiteral("") );
    ocsData.addAuthor( QStringLiteral("%%category%%"), aboutData.authors().last() );
    // NOTE 2024: Moved Inactive author header up here to reflect the fact that this list hasn't been updated lately

    aboutData.addAuthor( i18n("Bart 'Where are my toothpicks' Cerneels"),
                         i18n("Developer (Stecchino)"), QStringLiteral("bart.cerneels@kde.org"), QStringLiteral("http://commonideas.blogspot.com") );
    ocsData.addAuthor( QStringLiteral("Stecchino"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Edward \"Hades\" Toroshchin"),
                         i18n("Developer (dr_lepper)"), QStringLiteral("edward.hades@gmail.com") );
    ocsData.addAuthor( QStringLiteral("hadeschief"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Mark Kretschmann" ),
                         i18n("Project founder (markey)"), QStringLiteral("kretschmann@kde.org"), QStringLiteral("https://plus.google.com/102602725322221030250/posts") );
    ocsData.addAuthor( QStringLiteral("MarkKretschmann"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Matěj Laitl"),
                         i18n("iPod collection rewrite & more (strohel)"), QStringLiteral("matej@laitl.cz"), QStringLiteral("http://strohel.blogspot.com/") );
    ocsData.addAuthor( QStringLiteral("strohel"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Myriam Schweingruber"), i18n("Rokymoter, Bug triaging (Mamarok)"), QStringLiteral("myriam@kde.org"), QStringLiteral("http://blogs.fsfe.org/myriam") );
    ocsData.addAuthor( QStringLiteral("Mamarok"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Ralf 'SalsaMaster' Engels"),
                         i18n("Developer (rengels)"), QStringLiteral("ralf.engels@nokia.com") );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Patrick von Reth"), i18n("Windows build (TheOneRing)"),
                         QStringLiteral("patrick.vonreth@gmail.com") );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Rick W. Chen"),
                         i18n("Developer (stuffcorpse)"), QStringLiteral("stuffcorpse@archlinux.us") );
    ocsData.addAuthor( QStringLiteral("stuffcorpse"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Sam Lade"), i18n("Developer (Sentynel)"),
                         QStringLiteral("sam@sentynel.com") );
    ocsData.addAuthor( QStringLiteral("Sentynel"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Sven Krohlas"), i18n("Rokymoter, Developer (sven423)"), QStringLiteral("sven@asbest-online.de") );
    ocsData.addAuthor( QStringLiteral("krohlas"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Téo Mrnjavac"),
                         i18n("Developer (Teo`)"), QStringLiteral("teo@kde.org"), QStringLiteral("http://teom.wordpress.com/") );
    ocsData.addAuthor( QStringLiteral("teom"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Valorie Zimmerman"),
                         i18n("Rokymoter, Handbook (valorie)"), QStringLiteral("valorie@kde.org") );
    ocsData.addAuthor( QStringLiteral("valorie"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Ian 'The Beard' Monroe"), i18n("Developer (eean)"), QStringLiteral("ian@monroe.nu") );
    ocsData.addAuthor( QStringLiteral("eean"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Jeff 'IROKSOHARD' Mitchell"), i18n("Developer (jefferai)"), QStringLiteral("mitchell@kde.org") );
    ocsData.addAuthor( QStringLiteral("jefferai"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Leo Franchi"), i18n("Developer (lfranchi)"), QStringLiteral("lfranchi@kde.org") );
    ocsData.addAuthor( QStringLiteral("lfranchi"), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Lydia 'is wrong(TM)' Pintscher"), i18n("Release Vixen (Nightrose)"), QStringLiteral("lydia@kde.org") );
    ocsData.addAuthor( QStringLiteral("nightrose"), aboutData.authors().last() );

    aboutData.addCredit( i18n("Max Howell"), i18n("Developer, Vision"), QStringLiteral("max.howell@methylblue.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );

    aboutData.addAuthor( i18n("Maximilian Kossick"), i18n("Developer (maxx_k)"), QStringLiteral("maximilian.kossick@gmail.com") );
    ocsData.addAuthor( QString(), aboutData.authors().last() );

    aboutData.addAuthor( i18n("Nikolaj Hald 'Also very hot' Nielsen"), i18n("Developer (nhn)"), QStringLiteral("nhn@kde.org") );
    ocsData.addAuthor( QStringLiteral("nhnFreespirit"), aboutData.authors().last() );

    aboutData.addCredit( i18n("Seb 'Surfin' down under' Ruiz"), i18n("Developer (sebr)"), QStringLiteral("ruiz@kde.org") );
    ocsData.addCredit( QStringLiteral("seb"), aboutData.credits().last() );


    //Contributors
    aboutData.addCredit( i18n("Alejandro Wainzinger"), i18n("Developer (xevix)"), QStringLiteral("aikawarazuni@gmail.com") );
    ocsData.addCredit( QStringLiteral("xevix"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Alex Merry"), i18n("Developer, Replay Gain support"), QStringLiteral("kde@randomguy3.me.uk") );
    ocsData.addCredit( QStringLiteral("randomguy3"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Casey Link"), i18n("MP3tunes integration"), QStringLiteral("unnamedrambler@gmail.com") );
    ocsData.addCredit( QStringLiteral("Ramblurr"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Casper van Donderen"), i18n("Windows porting"), QStringLiteral("casper.vandonderen@gmail.com") );
    ocsData.addCredit( QStringLiteral("cvandonderen"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Christie Harris"), i18n("Rokymoter (dangle)"), QStringLiteral("dangle.baby@gmail.com") );
    ocsData.addCredit( QStringLiteral("dangle"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Dan Leinir Turthra Jensen"), i18n("Usability"), QStringLiteral("admin@leinir.dk") );
    ocsData.addCredit( QStringLiteral("leinir"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Dan 'Hey, it compiled...' Meltzer"), i18n("Developer (hydrogen)"), QStringLiteral("parallelgrapefruit@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Daniel Caleb Jones"), i18n("Biased playlists"), QStringLiteral("danielcjones@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Daniel Dewald"), i18n("Tag Guesser, Labels, Spectrum Analyzer"), QStringLiteral("Daniel.Dewald@time-shift.de") );
    ocsData.addCredit( QStringLiteral("TheCrasher"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Daniel Winter"), i18n("Nepomuk integration"), QStringLiteral("dw@danielwinter.de") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Frank Meerkötter"), i18n("Podcast improvements"), QStringLiteral("frank@meerkoetter.org") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Greg Meyer"), i18n("Live CD, Bug squashing (oggb4mp3)"), QStringLiteral("greg@gkmweb.com") );
    ocsData.addCredit( QStringLiteral("oggb4mp3"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Harald Sitter"), i18n("Phonon, Lord-President of KDE Multimedia (apachelogger)"), QStringLiteral("harald.sitter@kdemail.net") );
    ocsData.addCredit( QStringLiteral("apachelogger"), aboutData.credits().last() );
    aboutData.addCredit( i18n("John Atkinson"), i18n("Assorted patches"), QStringLiteral("john@fauxnetic.co.uk") );
    ocsData.addCredit( QStringLiteral("fauxnetic"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Kenneth Wesley Wimer II"), i18n("Icons"), QStringLiteral("kwwii@bootsplash.org") );
    ocsData.addCredit( QStringLiteral("kwwii"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Kevin Funk"), i18n("Developer, Website theme (KRF)"), QStringLiteral("krf@electrostorm.net") );
    ocsData.addCredit( QStringLiteral("krf"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Kuba Serafinowski"), i18n("Rokymoter"), QStringLiteral("zizzfizzix@gmail.com") );
    ocsData.addCredit( QStringLiteral("zizzfizzix"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Lee Olson"), i18n("Artwork"), QStringLiteral("leetolson@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Ljubomir Simin"), i18n("Rokymoter (ljubomir)"), QStringLiteral("ljubomir.simin@gmail.com") );
    ocsData.addCredit( QStringLiteral("ljubomir"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Lucas Gomes"), i18n("Developer (MaskMaster)"), QStringLiteral("x8lucas8x@gmail.com") );
    ocsData.addCredit( QStringLiteral("x8lucas8x"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Mathias Panzenböck"), i18n("Podcast improvements"), QStringLiteral("grosser.meister.morti@gmx.net") );
    ocsData.addCredit( QStringLiteral("panzi"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Mikko Caldara"), i18n("Bug triaging and sanitizing"), QStringLiteral("mikko.cal@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Nikhil Marathe"), i18n("UPnP support and patches (nsm)"), QStringLiteral("nsm.nikhil@gmail.com") );
    ocsData.addCredit( QStringLiteral("nikhilm"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Nuno Pinheiro"), i18n("Artwork"), QStringLiteral("nuno@oxygen-icons.org") );
    ocsData.addCredit( QStringLiteral("nunopinheirokde"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Olivier Bédard"), i18n("Website hosting"), QStringLiteral("paleo@pwsp.net") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Pasi Lalinaho"), i18n("Rokymoter (emunkki)"), QStringLiteral("pasi@getamarok.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Peter Zhou Lei"), i18n("Scripting interface"), QStringLiteral("peterzhoulei@gmail.com") );
    ocsData.addCredit( QStringLiteral("peterzl"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Phalgun Guduthur"), i18n("Nepomuk Collection (phalgun)"), QStringLiteral("me@phalgun.in") );
    ocsData.addCredit( QStringLiteral("phalgun"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Scott Wheeler"), i18n("TagLib & ktrm code"), QStringLiteral("wheeler@kde.org") );
    ocsData.addCredit( QStringLiteral("wheels"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Shane King"), i18n("Patches & Windows porting (shakes)"), QStringLiteral("kde@dontletsstart.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Simon Esneault"), i18n("Photos & Videos applets, Context View"), QStringLiteral("simon.esneault@gmail.com") );
    ocsData.addCredit( QStringLiteral("Takahani"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Soren Harward"), i18n("Developer, Automated Playlist Generator"), QStringLiteral("stharward@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Thomas Lübking"), i18n("Developer"), QStringLiteral("thomas.luebking@web.de") );
    ocsData.addCredit( QStringLiteral("thomas12777"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Valentin Rouet"), i18n("Developer"), QStringLiteral("v.rouet@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Wade Olson"), i18n("Splash screen artist"), QStringLiteral("wade@corefunction.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("William Viana Soares"), i18n("Context view"), QStringLiteral("vianasw@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );

    //Former Contributors
    aboutData.addCredit( i18n("Former contributors"), i18n("People listed below have contributed to Amarok in the past. Thank you!"), QStringLiteral("") );
    ocsData.addCredit( QStringLiteral("%%category%%"), aboutData.credits().last() );
    aboutData.addCredit( i18n("Adam Pigg"), i18n("Analyzers, patches, shoutcast"), QStringLiteral("adam@piggz.co.uk") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Adeodato Simó"), i18n("Patches"), QStringLiteral("asp16@alu.ua.es") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Alexandre Oliveira"), i18n("Developer"), QStringLiteral("aleprj@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andreas Mair"), i18n("MySQL support"), QStringLiteral("am_ml@linogate.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andrew de Quincey"), i18n("Postgresql support"), QStringLiteral("adq_dvb@lidskialf.net") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andrew Turner"), i18n("Patches"), QStringLiteral("andrewturner512@googlemail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Andy Kelk"), i18n("MTP and Rio Karma media devices, patches"), QStringLiteral("andy@mopoke.co.uk") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Christian Muehlhaeuser"), i18n("Developer"), QStringLiteral("chris@chris.de") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Derek Nelson"), i18n("Graphics, splash-screen"), QStringLiteral("admrla@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Enrico Ros"), i18n("Analyzers, Context Browser and systray eye-candy"), QStringLiteral("eros.kde@email.it") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Frederik Holljen"), i18n("Developer"), QStringLiteral("fh@ez.no") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Gábor Lehel"), i18n("Developer"), QStringLiteral("illissius@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Gérard Dürrmeyer"), i18n("Icons and image work"), QStringLiteral("gerard@randomtree.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Giovanni Venturi"), i18n("Dialog to filter the collection titles"), QStringLiteral("giovanni@ksniffer.org") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Jarkko Lehti"), i18n("Tester, IRC channel operator, whipping"), QStringLiteral("grue@iki.fi") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Jocke Andersson"), i18n("Rokymoter, bug fixer (Firetech)"), QStringLiteral("ajocke@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Marco Gulino"), i18n("Konqueror Sidebar, some DCOP methods"), QStringLiteral("marco@kmobiletools.org") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Martin Aumueller"), i18n("Developer"), QStringLiteral("aumuell@reserv.at") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Melchior Franz"), i18n("FHT routine, bugfixes"), QStringLiteral("mfranz@kde.org") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Michael Pyne"), i18n("K3b export code"), QStringLiteral("michael.pyne@kdemail.net") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Mike Diehl"), i18n("Developer"), QStringLiteral("madpenguin8@yahoo.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Paul Cifarelli"), i18n("Developer"), QStringLiteral("paul@cifarelli.net") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Peter C. Ndikuwera"), i18n("Bugfixes, PostgreSQL support"), QStringLiteral("pndiku@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Pierpaolo Panfilo"), i18n("Developer"), QStringLiteral("pippo_dp@libero.it") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Reigo Reinmets"), i18n("Wikipedia support, patches"), QStringLiteral("xatax@hot.ee") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Roman Becker"), i18n("Former Amarok logo, former splash screen, former icons"), QStringLiteral("roman@formmorf.de") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Sami Nieminen"), i18n("Audioscrobbler support"), QStringLiteral("sami.nieminen@iki.fi") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Stanislav Karchebny"), i18n("Developer"), QStringLiteral("berkus@madfire.net") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Stefan Bogner"), i18n("Loads of stuff"), QStringLiteral("bochi@online.ms") );
    ocsData.addCredit( QString(), aboutData.credits().last() );
    aboutData.addCredit( i18n("Tomasz Dudzik"), i18n("Splash screen"), QStringLiteral("madsheytan@gmail.com") );
    ocsData.addCredit( QString(), aboutData.credits().last() );

    //Donors:
    //Last update: 2012/11/07, post Roktober 2012 //TODO possibly remove in Amarok 4
    ocsData.addDonor( QStringLiteral("ayleph"), KAboutPerson( i18n( "Andrew Browning" ) ) );
    ocsData.addDonor( QString(), KAboutPerson( i18n( "Chris Wales" ) ) );
    ocsData.addDonor( QString(), KAboutPerson( i18n( "ZImin Stanislav" ) ) );

    KAboutData::setApplicationData(aboutData);

    // Command line parser
    QCommandLineParser parser;

    aboutData.setupCommandLine(&parser);
    app.initCliArgs(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService::StartupOptions startOptions = parser.isSet( QStringLiteral("multipleinstances") ) ? KDBusService::Multiple
                                                                                    : KDBusService::Unique ;
    // register  the app  to dbus
    KDBusService dbusService( startOptions );

    QObject::connect(&dbusService, &KDBusService::activateRequested,
                     &app, &App::activateRequested);

    const bool debugColorsEnabled = !parser.isSet( QStringLiteral("coloroff") );
    const bool debugEnabled = parser.isSet( QStringLiteral("debug") ) || parser.isSet( QStringLiteral("debug-with-lastfm") ); // HACK see App::initCliArgs

    Debug::setDebugEnabled( debugEnabled );
    Debug::setColoredDebug( debugColorsEnabled );

    if ( parser.isSet( QStringLiteral("debug-audio") ) ) {
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
            if( parser.isSet( QLatin1String(instanceOptions[ i ]) ) )
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

