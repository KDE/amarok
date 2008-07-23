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
#include "scriptengine/AmarokLyricsScript.h"
#include "scriptengine/AmarokOSDScript.h"
#include "scriptengine/AmarokPlaylistScript.h"
#include "scriptengine/AmarokScript.h"
#include "scriptengine/AmarokScriptableServiceScript.h"
#include "scriptengine/AmarokServicePluginManagerScript.h"
#include "scriptengine/AmarokStatusbarScript.h"
#include "scriptengine/AmarokTrackInfoScript.h"
#include "scriptengine/AmarokWindowScript.h"
#include "scriptengine/ScriptImporter.h"
#include "ScriptSelector.h"
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
#include <KPluginSelector>
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
    setButtons( None );

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

    m_scriptSelector = new ScriptSelector( m_gui->pluginWidget );
    m_scriptSelector->resize ( 600, 300 );

    connect( m_gui->installButton,   SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( m_gui->retrieveButton,  SIGNAL( clicked() ), SLOT( slotRetrieveScript() ) );
    connect( m_gui->uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );

    m_gui->installButton  ->setIcon( KIcon( "folder-amarok" ) );
    m_gui->retrieveButton ->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    m_gui->uninstallButton->setIcon( KIcon( "edit-delete-amarok" ) );

    QSize sz = sizeHint();
    setMinimumSize( qMax( 630, sz.width() ), qMax( 415, sz.height() ) );
    resize( sizeHint() );

    // Center the dialog in the middle of the mainwindow
    const int x = parentWidget()->width() / 2 - width() / 2;
    const int y = parentWidget()->height() / 2 - height() / 2;
    move( x, y );

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

    return slotRunScript( silent );
}


bool
ScriptManager::stopScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return false;

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


////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::findScripts() //SLOT
{
    DEBUG_BLOCK

    const QStringList allFiles = KGlobal::dirs()->findAllResources( "data", "amarok/scripts/*/main.js", KStandardDirs::Recursive );

    // Add found scripts to treeWidget:
    QList<KPluginInfo> pluginInfoList;
    foreach( const QString &str, allFiles )
    {
        loadScript( str );
    }
    foreach( const QString &key, m_scripts.keys() )
        pluginInfoList.append( *m_scripts[key].info );
    m_scriptSelector->addPlugins( pluginInfoList, KPluginSelector::ReadConfigFile, "Scripts" );
    // Handle auto-run:

    KConfigGroup config = Amarok::config( "ScriptManager" );
    const QStringList runningScripts = config.readEntry( "Running Scripts", QStringList() );

    foreach( const QString &str, runningScripts )
        if( m_scripts.contains( str ) )
        {
            slotRunScript();
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
    const QString name = "";

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
        m_scripts.remove( key );
    }
}


bool
ScriptManager::slotRunScript( bool silent )
{
    DEBUG_BLOCK

    const QString name = "";

    const KUrl url = m_scripts[name].url;
    QTime time;
    //load the wrapper classes
    m_scripts[name].engine = new QScriptEngine;
    startScriptEngine( name );
    QFile scriptFile( url.path() );
    scriptFile.open( QIODevice::ReadOnly );
    m_scripts[name].running = true;
    //todo: setProcessEventsInterval?

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

    const QString name = "";

    m_scripts[name].engine->abortEvaluation();
    if( m_scripts.value( name ).info->category() == "service" )
        The::scriptableServiceManager()->removeRunningScript( name );

    scriptFinished( name );
}


void
ScriptManager::slotConfigureScript()
{
    const QString name = "";
    m_scripts[name].globalPtr->slotConfigured();
}

void
ScriptManager::ServiceScriptPopulate( QString name, int level, int parent_id, QString path, QString filter )
{
    m_scripts[name].servicePtr->slotPopulate( level, parent_id, path, filter );
}

/*
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
*/

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

    delete m_scripts[name].engine;
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::loadScript( const QString& path )
{
    DEBUG_BLOCK

    if( !path.isEmpty() )
    {
        QFileInfo info( path );

        const QString specPath = info.path() + '/' + "script.spec";
        if( QFile::exists( specPath ) )
        {
            const KUrl url = KUrl( path );
            ScriptItem item;
            item.info = new KPluginInfo( specPath );
            if ( !item.info->isValid() ) return false;
            if ( ( item.info->name() == "" ) || ( item.info->version() == "" ) || ( item.info->category() == "" ) ) return false;
            debug() << "script info:" << item.info->name() << " " << item.info->version() << " " << item.info->category();
            item.url = url;
            item.running = false;
            m_scripts[item.info->name()] = item;
        }
        else
        {
            error() << "script.spec for "<< path << " is missing!";
            return false;
        }
    }
    return true;
}

void
ScriptManager::startScriptEngine( QString name )
{
    DEBUG_BLOCK

    QScriptEngine* scriptEngine = m_scripts[name].engine;
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

    objectPtr = new Amarok::AmarokLyricsScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Lyrics", scriptObject );
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

