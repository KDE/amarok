/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                 2005-2007 by Seb Ruiz <ruiz@kde.org>                    *
 *                      2006 by Alexandre Oliveira <aleprj@gmail.com>      *
 *                      2006 by Martin Ellis <martin.ellis@kdemail.net>    *
 *                      2007 by Leonardo Franchi <lfranchi@gmail.com>      *
 *                      2008 by Peter ZHOU <peterzhoulei@gmail.com>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#define DEBUG_PREFIX "ScriptManager"

#include "ScriptManager.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Debug.h"
#include "EngineController.h"
#include "AmarokProcess.h"
#include "StatusBar.h"
#include "Osd.h"
#include "scriptengine/AmarokCollectionScript.h"
#include "scriptengine/AmarokEngineScript.h"
#include "scriptengine/AmarokOSDScript.h"
#include "scriptengine/AmarokPlaylistScript.h"
#include "scriptengine/AmarokScript.h"
#include "scriptengine/AmarokScriptableServiceScript.h"
#include "scriptengine/AmarokServicePluginManagerScript.h"
#include "scriptengine/AmarokStatusbarScript.h"
#include "scriptengine/AmarokTrackInfoScript.h"
#include "scriptengine/AmarokWindowScript.h"
#include "scriptengine/ScriptImporter.h"
#include "servicebrowser/scriptableservice/ScriptableServiceManager.h"

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KApplication>
#include <KFileDialog>
#include <KIO/NetAccess>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <knewstuff2/engine.h>
#include <knewstuff2/core/entry.h>
#include <KProtocolManager>
#include <KPushButton>
#include <KRun>
#include <KStandardDirs>
#include <KTar>
#include <KTextEdit>
#include <KUrl>
#include <KWindowSystem>

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QPixmap>
#include <QSettings>
#include <QTextCodec>
#include <QTimer>
#include <QtScript>

#include <sys/stat.h>
#include <sys/types.h>

namespace Amarok {

    QString
    proxyForUrl(const QString& url)
    {
        KUrl kurl( url );

        QString proxy;

        if ( KProtocolManager::proxyForUrl( kurl ) !=
                QString::fromLatin1( "DIRECT" ) ) {
            KProtocolManager::slaveProtocol ( kurl, proxy );
        }

        return proxy;
    }

    QString
    proxyForProtocol(const QString& protocol)
    {
        return KProtocolManager::proxyFor( protocol );
    }


}

////////////////////////////////////////////////////////////////////////////////
// class ScriptManager
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;


ScriptManager::ScriptManager( QWidget* parent )
        : KDialog( parent )
        , EngineObserver( The::engineController() )
        , m_gui( new Ui::ScriptManagerBase() )
{
    DEBUG_BLOCK

    setObjectName( "ScriptManager" );
    setModal( false );
    setButtons( Close );
    setDefaultButton( Close );
    showButtonSeparator( true );

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Script Manager" ) ) );

    // Gives the window a small title bar, and skips a taskbar entry
#ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Utility );
    KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif

    QWidget* main = new QWidget( this );
    m_gui->setupUi( main );

    setMainWidget( main );

    /// Category items
    m_generalCategory    = new QTreeWidgetItem( m_gui->treeWidget );
    m_lyricsCategory     = new QTreeWidgetItem( m_gui->treeWidget );
    m_scoreCategory      = new QTreeWidgetItem( m_gui->treeWidget );
    m_transcodeCategory  = new QTreeWidgetItem( m_gui->treeWidget );
    m_contextCategory    = new QTreeWidgetItem( m_gui->treeWidget );
    m_servicesCategory   = new QTreeWidgetItem( m_gui->treeWidget );

    m_generalCategory  ->setText( 0, i18n( "General" ) );
    m_lyricsCategory   ->setText( 0, i18n( "Lyrics" ) );
    m_scoreCategory    ->setText( 0, i18n( "Score" ) );
    m_transcodeCategory->setText( 0, i18n( "Transcoding" ) );
    m_contextCategory  ->setText( 0, i18n( "Context Browser" ) );
    m_servicesCategory ->setText( 0, i18n( "Scripted Services" ) );

    m_generalCategory  ->setFlags( Qt::ItemIsEnabled );
    m_lyricsCategory   ->setFlags( Qt::ItemIsEnabled );
    m_scoreCategory    ->setFlags( Qt::ItemIsEnabled );
    m_transcodeCategory->setFlags( Qt::ItemIsEnabled );
    m_contextCategory  ->setFlags( Qt::ItemIsEnabled );
    m_servicesCategory ->setFlags( Qt::ItemIsEnabled );

    m_generalCategory  ->setIcon( 0, SmallIcon( "folder-amarok" ) );
    m_lyricsCategory   ->setIcon( 0, SmallIcon( "folder-amarok" ) );
    m_scoreCategory    ->setIcon( 0, SmallIcon( "folder-amarok" ) );
    m_transcodeCategory->setIcon( 0, SmallIcon( "folder-amarok" ) );
    m_contextCategory  ->setIcon( 0, SmallIcon( "folder-amarok" ) );
    m_servicesCategory ->setIcon( 0, SmallIcon( "folder-amarok" ) );


    // Restore the open/closed state of the category items
    KConfigGroup config = Amarok::config( "ScriptManager" );
    m_generalCategory  ->setExpanded( config.readEntry( "General category open", false ) );
    m_lyricsCategory   ->setExpanded( config.readEntry( "Lyrics category open", false ) );
    m_scoreCategory    ->setExpanded( config.readEntry( "Score category State", false ) );
    m_transcodeCategory->setExpanded( config.readEntry( "Transcode category open", false ) );
    m_contextCategory  ->setExpanded( config.readEntry( "Context category open", false ) );
    m_servicesCategory ->setExpanded( config.readEntry( "Service category open", false ) );

    connect( m_gui->treeWidget, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ), SLOT( slotCurrentChanged( QTreeWidgetItem* ) ) );
    connect( m_gui->treeWidget, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), SLOT( slotRunScript() ) );
    connect( m_gui->treeWidget, SIGNAL( customContextMenuRequested ( const QPoint& ) ), SLOT( slotShowContextMenu( const QPoint& ) ) );

    connect( m_gui->installButton,   SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( m_gui->retrieveButton,  SIGNAL( clicked() ), SLOT( slotRetrieveScript() ) );
    connect( m_gui->uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );
    connect( m_gui->runButton,       SIGNAL( clicked() ), SLOT( slotRunScript() ) );
    connect( m_gui->stopButton,      SIGNAL( clicked() ), SLOT( slotStopScript() ) );
    connect( m_gui->configureButton, SIGNAL( clicked() ), SLOT( slotConfigureScript() ) );
    connect( m_gui->aboutButton,     SIGNAL( clicked() ), SLOT( slotAboutScript() ) );

    m_gui->installButton  ->setIcon( KIcon( "folder-amarok" ) );
    m_gui->retrieveButton ->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    m_gui->uninstallButton->setIcon( KIcon( "edit-delete-amarok" ) );
    m_gui->runButton      ->setIcon( KIcon( "media-playback-start-amarok" ) );
    m_gui->stopButton     ->setIcon( KIcon( "media-playback-stop-amarok" ) );
    m_gui->configureButton->setIcon( KIcon( "configure-amarok" ) );
    m_gui->aboutButton    ->setIcon( KIcon( "help-about-amarok" ) );

    QSize sz = sizeHint();
    setMinimumSize( qMax( 350, sz.width() ), qMax( 250, sz.height() ) );
    resize( sizeHint() );

    // Center the dialog in the middle of the mainwindow
    const int x = parentWidget()->width() / 2 - width() / 2;
    const int y = parentWidget()->height() / 2 - height() / 2;
    move( x, y );

//FIXME: contex tbrowser changes
//     connect( this, SIGNAL(lyricsScriptChanged()), ContextBrowser::instance(), SLOT( lyricsScriptChanged() ) );

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, SLOT( findScripts() ) );

}


ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK

    QStringList runningScripts;
    foreach( const QString &key, m_scripts.keys() )
        if( m_scripts[key].running )
            runningScripts << key;

    // Save config
    KConfigGroup config = Amarok::config( "ScriptManager" );
    config.writeEntry( "Running Scripts", runningScripts );

    // Save the open/closed state of the category items
    config.writeEntry( "General category open", m_generalCategory->isExpanded() );
    config.writeEntry( "Lyrics category open", m_lyricsCategory->isExpanded() );
    config.writeEntry( "Score category open", m_scoreCategory->isExpanded() );
    config.writeEntry( "Transcode category open", m_transcodeCategory->isExpanded() );
    config.writeEntry( "Context category open", m_contextCategory->isExpanded() );
    config.writeEntry( "Service category open", m_servicesCategory->isExpanded() );

    config.sync();

    s_instance = 0;
}


////////////////////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::runScript( const QString& name, bool silent )
{
    if( !m_scripts.contains( name ) )
        return false;

    m_gui->treeWidget->setCurrentItem( m_scripts[name].li );
    return slotRunScript( silent );
}


bool
ScriptManager::stopScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return false;

    m_gui->treeWidget->setCurrentItem( m_scripts[name].li );
    slotStopScript();

    return true;
}


QStringList
ScriptManager::listRunningScripts()
{
    QStringList runningScripts;
    foreach( const QString &key, m_scripts.keys() )
        if( m_scripts[key].running )
            runningScripts << key;

    return runningScripts;
}


QString
ScriptManager::specForScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return QString();
    QFileInfo info( m_scripts[name].url.path() );
    const QString specPath = info.path() + '/' + info.completeBaseName() + ".spec";

    return specPath;
}


void
ScriptManager::notifyFetchLyrics( const QString& artist, const QString& title )
{
    const QString args = QUrl::toPercentEncoding( artist ) + ' ' + QUrl::toPercentEncoding( title ); //krazy:exclude=qclasses
//    notifyScripts( "fetchLyrics " + args );
}


void
ScriptManager::notifyFetchLyricsByUrl( const QString& url )
{
//    notifyScripts( "fetchLyricsByUrl " + url );
}


void ScriptManager::notifyTranscode( const QString& srcUrl, const QString& filetype )
{
//    notifyScripts( "transcode " + srcUrl + ' ' + filetype );
}


void
ScriptManager::requestNewScore( const QString &url, double prevscore, int playcount, int length, float percentage, const QString &reason )
{
    const QString script = ensureScoreScriptRunning();
    if( script.isNull() )
    {
    // Scoring is currently disabled, so don't show warning
#if 0
        The::statusBar()->longMessage(
            i18n( "No score scripts were found, or none of them worked. Automatic scoring will be disabled. Sorry." ),
            KDE::StatusBar::Sorry );
#endif
        return;
    }
/*
    m_scripts[script].process->writeStdin(
        QString( "requestNewScore %6 %1 %2 %3 %4 %5" )
        .arg( prevscore )
        .arg( playcount )
        .arg( length )
        .arg( percentage )
        .arg( reason )
        .arg( QString( QUrl::toPercentEncoding( url ) ) ) ); //last because it might have %s
*/
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::findScripts() //SLOT
{
    const QStringList allFiles = KGlobal::dirs()->findAllResources( "data", "amarok/scripts/*/main.js",KStandardDirs::Recursive );

    // Add found scripts to treeWidget:
    foreach( const QString &str, allFiles )
        loadScript( str );

    // Handle auto-run:

    KConfigGroup config = Amarok::config( "ScriptManager" );
    const QStringList runningScripts = config.readEntry( "Running Scripts", QStringList() );

    foreach( const QString &str, runningScripts )
        if( m_scripts.contains( str ) )
        {
            m_gui->treeWidget->setCurrentItem( m_scripts[str].li );
            slotRunScript();
        }

//FIXME    m_gui->treeWidget->setCurrentItem( m_gui->treeWidget->firstChild() );
    slotCurrentChanged( m_gui->treeWidget->currentItem() );
}


void
ScriptManager::slotCurrentChanged( QTreeWidgetItem* item )
{
    const bool isCategory = item == m_generalCategory ||
                            item == m_lyricsCategory ||
                            item == m_scoreCategory ||
                            item == m_transcodeCategory ||
                            item == m_servicesCategory;

    if( item && !isCategory )
    {
        const QString name = item->text( 0 );
        m_gui->uninstallButton->setEnabled( true );
        m_gui->runButton->setEnabled( !m_scripts[name].running );
        m_gui->stopButton->setEnabled( m_scripts[name].running );
        m_gui->configureButton->setEnabled( m_scripts[name].running );
        m_gui->aboutButton->setEnabled( true );
    }
    else
    {
        m_gui->uninstallButton->setEnabled( false );
        m_gui->runButton->setEnabled( false );
        m_gui->stopButton->setEnabled( false );
        m_gui->configureButton->setEnabled( false );
        m_gui->aboutButton->setEnabled( false );
    }
}


bool
ScriptManager::slotInstallScript( const QString& path )
{
    QString _path = path;

    if( path.isNull() )
    {
        _path = KFileDialog::getOpenFileName( KUrl(),
            "*.amarokscript.tar *.amarokscript.tar.bz2 *.amarokscript.tar.gz|"
            + i18n( "Script Packages (*.amarokscript.tar, *.amarokscript.tar.bz2, *.amarokscript.tar.gz)" )
            , this );
        if( _path.isNull() ) return false;
    }

    KTar archive( _path );
    if( !archive.open( QIODevice::ReadOnly ) )
    {
        KMessageBox::sorry( 0, i18n( "Could not read this package." ) );
        return false;
    }

    QString destination = Amarok::saveLocation( "scripts/" );
    const KArchiveDirectory* const archiveDir = archive.directory();

    // Prevent installing a script that's already installed
    const QString scriptFolder = destination + archiveDir->entries().first();
    if( QFile::exists( scriptFolder ) )
    {
        KMessageBox::error( 0, i18n( "A script with the name '%1' is already installed. "
                                     "Please uninstall it first.", archiveDir->entries().first() ) );
        return false;
    }

    archiveDir->copyTo( destination );
    m_installSuccess = false;
    recurseInstall( archiveDir, destination );

    if( m_installSuccess )
    {
        KMessageBox::information( 0, i18n( "Script successfully installed." ) );
        return true;
    }
    else
    {
        KMessageBox::sorry( 0, i18n( "<p>Script installation failed.</p>"
                                     "<p>The package did not contain an executable file. "
                                     "Please inform the package maintainer about this error.</p>" ) );

        // Delete directory recursively
        KIO::NetAccess::del( KUrl( scriptFolder ), 0 );
    }

    return false;
}


void
ScriptManager::recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination )
{
    const QStringList entries = archiveDir->entries();

    foreach( const QString &entry, entries )
    {
        const KArchiveEntry* const archEntry = archiveDir->entry( entry );

        if( archEntry->isDirectory() )
        {
            const KArchiveDirectory* const dir = static_cast<const KArchiveDirectory*>( archEntry );
            recurseInstall( dir, destination + entry + '/' );
        }
        else
        {
            ::chmod( QFile::encodeName( destination + entry ), archEntry->permissions() );

            if( QFileInfo( destination + entry ).isExecutable() )
            {
                loadScript( destination + entry );
                m_installSuccess = true;
            }
        }
    }
}


void
ScriptManager::slotRetrieveScript()
{
    KNS::Engine *engine = new KNS::Engine();
    engine->init( "amarok.knsrc" );
    KNS::Entry::List entries = engine->downloadDialogModal();
    debug() << "scripts status:" << endl;
    foreach ( KNS::Entry* entry, entries )
    {
//        if ( entry->status() == KNS::Entry::Downloadable )
            debug() << "script downloadable!" << endl;
    }
    delete engine;
}


void
ScriptManager::slotUninstallScript()
{
    const QString name = m_gui->treeWidget->currentItem()->text( 0 );

    if( KMessageBox::warningContinueCancel( this, i18n( "Are you sure you want to uninstall the script '%1'?", name ), i18n("Uninstall Script"), KGuiItem( i18n("Uninstall") ) ) == KMessageBox::Cancel )
        return;

    if( m_scripts.find( name ) == m_scripts.end() )
        return;

    const QString directory = m_scripts[name].url.directory();

    // Delete directory recursively
    const KUrl url = KUrl( directory );
    if( !KIO::NetAccess::del( url, 0 ) )
    {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this script.</p><p>The ScriptManager can only uninstall scripts which have been installed as packages.</p>" ) );
        return;
    }

    QStringList keys;

    // Find all scripts that were in the uninstalled folder
    foreach( const QString &key, m_scripts.keys() )
        if( m_scripts[key].url.directory() == directory )
            keys << key;

    // Terminate script, remove entries from script list
    foreach( const QString &key, keys )
    {
        scriptFinished( key );
        delete m_scripts[key].li;
        delete m_scripts[key].engine;
        m_scripts.remove( key );
    }
}


bool
ScriptManager::slotRunScript( bool silent )
{
    DEBUG_BLOCK
    if( !m_gui->runButton->isEnabled() )
        return false;

    QTreeWidgetItem* const li = m_gui->treeWidget->currentItem();
    const QString name = li->text( 0 );

    if( m_scripts[name].type == "lyrics" && !lyricsScriptRunning().isEmpty() )
    {
        if( !silent )
            KMessageBox::sorry( 0, i18n( "Another lyrics script is already running. "
                                         "You may only run one lyrics script at a time." ) );
        return false;
    }

    if( m_scripts[name].type == "transcode" && !transcodeScriptRunning().isEmpty() )
    {
        if( !silent )
            KMessageBox::sorry( 0, i18n( "Another transcode script is already running. "
                                         "You may only run one transcode script at a time." ) );
        return false;
    }
    const KUrl url = m_scripts[name].url;
    QTime time;
    //load the wrapper classes
    startScriptEngine( name );
    QFile scriptFile( url.path() );
    scriptFile.open( QIODevice::ReadOnly );
    m_scripts[name].running = true;
    //todo: setProcessEventsInterval?
    li->setIcon( 0, SmallIcon( "media-playback-start-amarok" ) );
    slotCurrentChanged( m_gui->treeWidget->currentItem() );

    m_scripts[name].log += time.currentTime().toString() + " Script Started!" + '\n';
    m_scripts[name].engine->evaluate( scriptFile.readAll() );
    scriptFile.close();
    //FIXME: '\n' doesen't work?
    if ( m_scripts[name].engine->hasUncaughtException() )
    {
        m_scripts[name].log += time.currentTime().toString() + " " + m_scripts[name].engine->uncaughtException().toString() + " on Line: " + QString::number( m_scripts[name].engine->uncaughtExceptionLineNumber() ) + '\n';
        m_scripts[name].engine->clearExceptions();
        KMessageBox::sorry( 0, i18n( "There are exceptions caught in the script. Please refer to the log!" ) );
        scriptFinished( name );
    }

    return true;
}


void
ScriptManager::slotStopScript()
{
    DEBUG_BLOCK

    QTreeWidgetItem* const li = m_gui->treeWidget->currentItem();
    const QString name = li->text( 0 );

    m_scripts[name].engine->abortEvaluation();
    if( m_scripts.value( name ).type == "service" )
        The::scriptableServiceManager()->removeRunningScript( name );

    scriptFinished( name );
}


void
ScriptManager::slotConfigureScript()
{
    const QString name = m_gui->treeWidget->currentItem()->text( 0 );
      if( !m_scripts[name].running ) return;
    m_scripts[name].globalPtr->slotConfigured();
}

void
ScriptManager::ServiceScriptPopulate( QString name, int level, int parent_id, QString path, QString filter )
{
    m_scripts[name].servicePtr->slotPopulate( level, parent_id, path, filter );
}

void
ScriptManager::slotAboutScript()
{
    //TODO: rewrite this
    const QString name = m_gui->treeWidget->currentItem()->text( 0 );
    QFile readme( m_scripts[name].url.directory( KUrl::AppendTrailingSlash ) + "README" );
    QFile license( m_scripts[name].url.directory( KUrl::AppendTrailingSlash) + "COPYING" );

    if( !readme.open( QIODevice::ReadOnly ) )
    {
        KMessageBox::sorry( 0, i18n( "There is no information available for this script." ) );
        return;
    }

    KAboutData aboutData( name.toLatin1(), 0, ki18n(name.toLatin1()), "1.0", ki18n(readme.readAll()) );

    KAboutApplicationDialog* about = new KAboutApplicationDialog( &aboutData, this );
    about->setButtons( KDialog::Ok );
    about->setDefaultButton( KDialog::Ok );

    kapp->setTopWidget( about );
    about->setCaption( KDialog::makeStandardCaption( i18n( "About %1", name ) ) );

    about->setInitialSize( QSize( 500, 350 ) );
    about->show();
}


void
ScriptManager::slotShowContextMenu( const QPoint& pos )
{
    QTreeWidgetItem* item = m_gui->treeWidget->itemAt( pos );

    const bool isCategory = item == m_generalCategory ||
                            item == m_lyricsCategory ||
                            item == m_scoreCategory ||
                            item == m_transcodeCategory ||
                            item == m_contextCategory ||
                            item == m_servicesCategory;

    if( !item || isCategory )
        return;

    // Find the script entry in our map
    QString key;
    foreach( key, m_scripts.keys() )
        if( m_scripts[key].li == item ) break;

    enum { SHOW_LOG, EDIT };
    KMenu menu;
    menu.addTitle( i18n( "Debugging" ) );
    QAction* logAction = menu.addAction( KIcon( "view-history-amarok" ), i18n( "Show Output &Log" ) );
    QAction* editAction = menu.addAction( KIcon( "document-properties-amarok" ), i18n( "&Edit" ) );
    logAction->setData( SHOW_LOG );
    editAction->setData( EDIT );

    QAction* choice = menu.exec( mapToGlobal( pos ) );
    if( !choice ) return;
    const int id = choice->data().toInt();

    switch( id )
    {
        case EDIT:
            KRun::runCommand( "kwrite " + m_scripts[key].url.path(), 0 );
            break;

        case SHOW_LOG:
            KTextEdit* editor = new KTextEdit( m_scripts[key].log );
            kapp->setTopWidget( editor );
            editor->setWindowTitle( KDialog::makeStandardCaption( i18n( "Output Log for %1", key ) ) );
            editor->setReadOnly( true );

            QFont font( "fixed" );
            font.setFixedPitch( true );
            font.setStyleHint( QFont::TypeWriter );
            editor->setFont( font );

            editor->resize( 400, 350 );
            editor->show();
            break;
    }
}

void
ScriptManager::scriptFinished( QString name ) //SLOT
{
    DEBUG_BLOCK

    QTime time;
    m_scripts[name].running = false;
    foreach( const QObject* obj, m_scripts[name].guiPtrList )
        delete obj;
    m_scripts[name].guiPtrList.clear();
    foreach( const QObject* obj, m_scripts[name].wrapperList )
        delete obj;
    m_scripts[name].wrapperList.clear();
    m_scripts[name].log += time.currentTime().toString() + " Script ended!" + '\n';

    m_scripts[name].li->setIcon( 0, QPixmap() );
    slotCurrentChanged( m_gui->treeWidget->currentItem() );
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

QStringList
ScriptManager::scriptsOfType( const QString &type ) const
{
    QStringList scripts;
    foreach( const QString &key, m_scripts.keys() )
        if( m_scripts[key].type == type )
            scripts += key;

    return scripts;
}


QString
ScriptManager::scriptRunningOfType( const QString &type ) const
{
    foreach( const QString &key, m_scripts.keys() )
        if( m_scripts[key].running && m_scripts[key].type == type )
            return key;

    return QString();
}


QString
ScriptManager::ensureScoreScriptRunning()
{
    AMAROK_NOTIMPLEMENTED

    // FIXME this code sometimes causes a crash:
    //
    //#26 0xb5d91b24 in QTreeWidget::setCurrentItem (this=0x8757538, item=0x8941fe0) at itemviews/qtreewidget.cpp:2749
    //#27 0xb78b78e7 in ScriptManager::runScript (this=0x870bc00, name=@0x8710484, silent=true)
    //    at /var/tmp/paludis/media-sound-amarok-scm/work/amarok/amarok/src/dialogs/scriptmanager.cpp:236
    //#28 0xb78b7a94 in ScriptManager::ensureScoreScriptRunning (this=0x870bc00)
    //    at /var/tmp/paludis/media-sound-amarok-scm/work/amarok/amarok/src/dialogs/scriptmanager.cpp:798

#if 0
    QString s = scoreScriptRunning();
    if( !s.isNull() )
        return s;

    if( runScript( AmarokConfig::lastScoreScript(), true /*silent*/ ) )
        return AmarokConfig::lastScoreScript();

    const QString def = i18n( "Score" ) + ": " + "Default";
    if( runScript( def, true ) )
        return def;

    const QStringList scripts = scoreScripts();
    QStringList::ConstIterator end = scripts.constEnd();
    for( QStringList::ConstIterator it = scripts.constBegin(); it != end; ++it )
        if( runScript( *it, true ) )
            return *it;

#endif
    return QString();
}

void
ScriptManager::loadScript( const QString& path )
{
    DEBUG_BLOCK
    if( !path.isEmpty() )
    {
        QFileInfo info( path );
        QTreeWidgetItem* li = 0;
        QString name, type, version, AmarokVersion;
        const QString specPath = info.path() + '/' + "script.spec";
        if( QFile::exists( specPath ) )
        {
            const KUrl url = KUrl( path );
            QSettings spec( specPath, QSettings::IniFormat );
            if( spec.contains( "name" ) )
                name = spec.value( "name" ).toString();
            else
            {
                error() << "script name missing in "<< path;
                return;
            }
            if( spec.contains( "version" ) )
                version = spec.value( "verion" ).toString();
            else
            {
                error() << "script version missing in "<< path;
                return;
            }
            if( spec.contains( "AmarokVersion" ) )
                AmarokVersion = spec.value( "AmarokVersion" ).toString();
            else
            {
                error() << "script Amarok dependency missing in "<< path;
                return;
            }
            if( spec.contains( "type" ) )
            {
                type = spec.value( "type" ).toString();
                if( type == "lyrics" )
                {
                    li = new QTreeWidgetItem( m_lyricsCategory );
                    li->setText( 0, name );
                }
                if( type == "transcode" )
                {
                    li = new QTreeWidgetItem( m_transcodeCategory );
                    li->setText( 0, name );
                }
                if( type == "score" )
                {
                    li = new QTreeWidgetItem( m_scoreCategory );
                    li->setText( 0, name );
                }
                if( type == "service" )
                {
                    li = new QTreeWidgetItem( m_servicesCategory );
                    li->setText( 0, name );
                }
                if( type == "context" )
                {
                    li = new QTreeWidgetItem( m_contextCategory );
                        li->setText( 0, name );
                }
                if( type == "generic" )
                {
                    li = new QTreeWidgetItem( m_generalCategory );
                        li->setText( 0, name );
                }
            }
            else
            {
                error() << "script type missing in "<< path;
                return;
            }
            li->setIcon( 0, QPixmap() );

            ScriptItem item;
            item.url = url;
            item.type = type;
            item.version = version;
            item.AmarokVersion = AmarokVersion;
            item.li = li;
            item.engine = new QScriptEngine;
            item.running = false;
            m_scripts[name] = item;

            slotCurrentChanged( m_gui->treeWidget->currentItem() );
        }
        else
        {
            error() << "script.spec for "<< path << " is missing!";
        }
    }
}

void
ScriptManager::startScriptEngine( QString name )
{
    DEBUG_BLOCK

    QScriptEngine* scriptEngine = m_scripts[name].engine;
    debug() << "importing qt bindings...";
    {
        Debug::Block blockie("All extensions");
        {
            Debug::Block blockie("qt.core");
            scriptEngine->importExtension( "qt.core" );
        }
        {
            Debug::Block blockie("qt.gui");
            scriptEngine->importExtension( "qt.gui" );
        }
        {
            Debug::Block blockie("qt.network");
            scriptEngine->importExtension( "qt.network" );
        }
        {
            Debug::Block blockie("qt.sql");
            scriptEngine->importExtension( "qt.sql" );
        }
        {
            Debug::Block blockie("qt.webkit");
            scriptEngine->importExtension( "qt.webkit" );
        }
        {
            Debug::Block blockie("qt.xml");
            scriptEngine->importExtension( "qt.xml" );
        }
    }

    QObject* objectPtr;
    QScriptValue scriptObject;

    objectPtr = new Amarok::ScriptImporter( scriptEngine, m_scripts[name].url );
    scriptObject = scriptEngine->newQObject( objectPtr );
    scriptEngine->globalObject().setProperty( "Importer", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    m_scripts[name].globalPtr = new Amarok::AmarokScript( scriptEngine );
    m_global = scriptEngine->newQObject( m_scripts[name].globalPtr );
    scriptEngine->globalObject().setProperty( "Amarok", m_global );
    m_scripts[name].wrapperList.append( m_scripts[name].globalPtr );

    m_scripts[name].servicePtr = new Amarok::AmarokScriptableServiceScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( m_scripts[name].servicePtr );
    m_global.setProperty( "ScriptableService", scriptObject );
    m_scripts[name].wrapperList.append( m_scripts[name].servicePtr );

    objectPtr = new Amarok::AmarokServicePluginManagerScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "ServicePluginManager", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokCollectionScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Collection", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokEngineScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Engine", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokTrackInfoScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.property("Engine").setProperty( "TrackInfo", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokWindowScript( scriptEngine, &m_scripts[name].guiPtrList );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Window", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokPlaylistScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Playlist", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokStatusbarScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.property( "Window" ).setProperty( "Statusbar", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new Amarok::AmarokOSDScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.property( "Window" ).setProperty( "OSD", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    scriptObject = scriptEngine->newObject();
    m_global.property( "Window" ).setProperty( "ToolsMenu", scriptObject );

    scriptObject = scriptEngine->newObject();
    m_global.property( "Window" ).setProperty( "SettingsMenu", scriptObject );

}


#include "ScriptManager.moc"

