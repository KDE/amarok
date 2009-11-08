/****************************************************************************************
 * Copyright (c) 2004-2007 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005-2007 Seb Ruiz <ruiz@kde.org>                                      *
 * Copyright (c) 2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2006 Martin Ellis <martin.ellis@kdemail.net>                           *
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
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

#define DEBUG_PREFIX "ScriptManager"

#include "ScriptManager.h"

#include "Amarok.h"
#include "AmarokProcess.h"
#include "Debug.h"
#include "EngineController.h"
#include "MainWindow.h"
#include "Osd.h"
#include "statusbar/StatusBar.h"
#include "amarokconfig.h"
#include "services/scriptable/ScriptableServiceManager.h"
#include "scriptengine/AmarokCollectionScript.h"
#include "scriptengine/AmarokScriptConfig.h"
#include "scriptengine/AmarokEngineScript.h"
#include "scriptengine/AmarokInfoScript.h"
#include "scriptengine/AmarokLyricsScript.h"
#include "scriptengine/AmarokNetworkScript.h"
#include "scriptengine/AmarokOSDScript.h"
#include "scriptengine/AmarokPlaylistScript.h"
#include "scriptengine/AmarokScript.h"
#include "scriptengine/AmarokScriptableServiceScript.h"
#include "scriptengine/AmarokServicePluginManagerScript.h"
#include "scriptengine/AmarokStatusbarScript.h"
#include "scriptengine/AmarokWindowScript.h"
#include "scriptengine/MetaTypeExporter.h"
#include "scriptengine/ScriptImporter.h"
#include "ScriptUpdater.h"
#include "ui_ScriptManagerBase.h"

#include <KApplication>
#include <KFileDialog>
#include <KIO/NetAccess>
#include <KLocale>
#include <KMessageBox>
#include <knewstuff2/engine.h>
#include <knewstuff2/core/entry.h>
#include <KProtocolManager>
#include <KStandardDirs>
#include <KTar>
#include <KWindowSystem>

#include <QFileInfo>
#include <QTimer>
#include <QScriptEngine>
#include <QTextDocument>

#include <sys/stat.h>
#include <sys/types.h>

////////////////////////////////////////////////////////////////////////////////
// class ScriptManager
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;

ScriptManager::ScriptManager( QWidget* parent )
        : KDialog( parent )
        , EngineObserver( The::engineController() )
{
    DEBUG_BLOCK
    Ui::ScriptManagerBase gui;
    setObjectName( "ScriptManager" );
    setButtons( None );

    s_instance = this;

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n( "Script Manager" ) ) );

    // Skips a taskbar entry
#ifdef Q_WS_X11
    KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif

    QWidget* main = new QWidget( this );
    gui.setupUi( main );
    setMainWidget( main );

    // Load config
    gui.kcfg_AutoUpdateScripts->setChecked( AmarokConfig::autoUpdateScripts() );

    m_scriptSelector = gui.pluginWidget;
    gui.pluginWidget->setSizePolicy(QSizePolicy::Preferred ,QSizePolicy::Expanding);

    connect( gui.installButton,   SIGNAL( clicked() ), SLOT( slotInstallScript() ) );
    connect( gui.retrieveButton,  SIGNAL( clicked() ), SLOT( slotRetrieveScript() ) );
    connect( gui.uninstallButton, SIGNAL( clicked() ), SLOT( slotUninstallScript() ) );
    connect( gui.okButton,        SIGNAL( clicked() ), SLOT( reject() ) );
    connect( m_scriptSelector, SIGNAL( changed( bool ) ), SLOT( slotConfigChanged( bool ) ) );
    connect( m_scriptSelector, SIGNAL( configCommitted ( const QByteArray & ) ), SLOT( slotConfigComitted( const QByteArray & ) ) );
    connect( gui.kcfg_AutoUpdateScripts, SIGNAL( toggled( bool ) ), SLOT( slotUpdateSettingChanged( bool ) ) );

    gui.installButton  ->setIcon( KIcon( "folder-amarok" ) );
    gui.retrieveButton ->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );
    gui.uninstallButton->setIcon( KIcon( "edit-delete-amarok" ) );
    gui.okButton       ->setIcon( KIcon( "dialog-ok" ) );
    
    // Center the dialog in the middle of the mainwindow
    const int x = parentWidget()->width() / 2 - sizeHint().width() / 2;
    const int y = parentWidget()->height() / 2 - sizeHint().height() / 2;
    move( x, y );

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, SLOT( updateAllScripts() ) );

}

ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK

    QStringList runningScripts;
    foreach( const QString &key, m_scripts.keys() )
        if( m_scripts[key].running ) {
            runningScripts << key;
            slotStopScript( key );
        }

    // Save config
    KConfigGroup config = Amarok::config( "ScriptManager" );
    config.writeEntry( "Running Scripts", runningScripts );

    config.sync();
}

void
ScriptManager::destroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

ScriptManager*
ScriptManager::instance()
{
    return s_instance ? s_instance : new ScriptManager( The::mainWindow() );
}

////////////////////////////////////////////////////////////////////////////////
// public
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::runScript( const QString& name, bool silent )
{
    if( !m_scripts.contains( name ) )
        return false;

    return slotRunScript( name, silent );
}

bool
ScriptManager::stopScript( const QString& name )
{
    if( !m_scripts.contains( name ) )
        return false;
    slotStopScript( name );
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

bool
ScriptManager::lyricsScriptRunning()
{
    return !m_lyricsScript.isEmpty();
}

void
ScriptManager::notifyFetchLyrics( const QString& artist, const QString& title )
{
    DEBUG_BLOCK
    emit fetchLyrics( Qt::escape( artist ), Qt::escape( title ), QString() );
}

void
ScriptManager::notifyFetchLyricsByUrl( const QString& artist, const QString& title, const QString& url )
{
    emit fetchLyrics( Qt::escape( artist ), Qt::escape( title ), url );
}

////////////////////////////////////////////////////////////////////////////////
// private slots (script updater stuff)
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::updateAllScripts() // SLOT
{

    DEBUG_BLOCK
    // find all scripts (both in $KDEHOME and /usr)
    QStringList foundScripts = KGlobal::dirs()->findAllResources( "data", "amarok/scripts/*/main.js", KStandardDirs::Recursive | KStandardDirs::NoDuplicates);
    m_nScripts = foundScripts.count();
    // create a ScriptUpdater for each script
    m_updaters = new ScriptUpdater[m_nScripts];
    for (int i = 0; i < m_nScripts; i++)
    {
        // all the ScriptUpdaters are now started in parallel.
        // tell them which script to work on
        m_updaters[i].setScriptPath(foundScripts.at(i));
        // tell them whom to signal when they're finished
        connect ( &(m_updaters[i]), SIGNAL( finished( QString ) ), SLOT( updaterFinished( QString )) );
        // and finally tell them to get to work
        QTimer::singleShot( 0, &(m_updaters[i]), SLOT( updateScript() ) );
    }

}

void
ScriptManager::updaterFinished( QString scriptPath ) // SLOT
{
    DEBUG_BLOCK
    // count this event
    m_updateSemaphore.release();
    // load the script now (so that findScripts() doesn't have to do this)
    loadScript( scriptPath );
    // if all ScriptUpdaters have returned now, call findScripts()
    if ( m_updateSemaphore.tryAcquire(m_nScripts) )
    {
        findScripts();
    }
}

void
ScriptManager::slotUpdateSettingChanged( bool enabled )
{
    DEBUG_BLOCK
    // Save config
    AmarokConfig::setAutoUpdateScripts( enabled );
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::slotInstallScript( const QString& path )
{
    DEBUG_BLOCK

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
        KMessageBox::information( 0, i18n( "<p>Script successfully installed.</p>"
                                           "<p>Please restart Amarok to start the script.</p>" ) );
        return true;
    }
    else
    {
        KMessageBox::sorry( 0, i18n( "<p>Script installation failed.</p>"
                                     "<p>Please inform the package maintainer about this error.</p>" ) );

        // Delete directory recursively
        KIO::NetAccess::del( KUrl( scriptFolder ), 0 );
    }
    return false;
}

void
ScriptManager::recurseInstall( const KArchiveDirectory* archiveDir, const QString& destination )
{
    DEBUG_BLOCK
    
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
            m_installSuccess = true;
        }
    }
}

void
ScriptManager::slotRetrieveScript()
{
    bool installed = false;
    bool deleted = false;
    KNS::Engine engine( this );
    engine.init( "amarok.knsrc" );
    KNS::Entry::List entries = engine.downloadDialogModal( this );
    foreach( KNS::Entry* entry, entries )
    {
        if ( entry->status() == KNS::Entry::Installed )
            installed = true;
        else if ( entry->status() == KNS::Entry::Deleted )
            deleted = true;
    }
    if ( installed )
        KMessageBox::information( 0, i18n( "<p>Script successfully installed.</p>"
                                            "<p>Please restart Amarok to start the script.</p>" ) );
    else if (  deleted )
        KMessageBox::information( 0, i18n( "<p>Script successfully uninstalled.</p>"
                                            "<p>Please restart Amarok to totally remove the script.</p>" ) );
}

void
ScriptManager::slotUninstallScript()
{
    DEBUG_BLOCK

    const QString name = m_scriptSelector->currentItem();
    if( name.isEmpty() )
        return;

    if( KMessageBox::warningContinueCancel( this, i18n( "Are you sure you want to uninstall the script '%1'?", name ), i18n("Uninstall Script"), KGuiItem( i18n("Uninstall") ) ) == KMessageBox::Cancel )
        return;

    const QString directory = m_scripts[name].url.directory();

    // Delete directory recursively
    const KUrl url = KUrl( directory );
    if( !KIO::NetAccess::del( url, 0 ) )
    {
        KMessageBox::sorry( 0, i18n( "<p>Could not uninstall this script.</p><p>The ScriptManager can only uninstall scripts which have been installed as packages.</p>" ) );
        return;
    }
    else
    {
        if ( m_scripts[name].running )
            slotStopScript( name );

        m_scripts.remove( name );
        KMessageBox::information( 0, i18n( "<p>Script successfully uninstalled.</p>"
        "<p>Please restart Amarok to totally remove the script.</p>" ) );
    }
}

bool
ScriptManager::slotRunScript( QString name, bool silent )
{
    DEBUG_BLOCK

    const KUrl url = m_scripts[name].url;
    QTime time;
    //load the wrapper classes
    m_scripts[name].engine = new QScriptEngine();
    startScriptEngine( name );
    QFile scriptFile( url.path() );
    scriptFile.open( QIODevice::ReadOnly );
    m_scripts[name].running = true;
    m_scripts[name].evaluating = true;
    if( m_scripts[name].info.category() == "Lyrics" )
        m_lyricsScript = name;

    m_scripts[name].log += time.currentTime().toString() + " Script Started!" + '\n';
    m_scripts[name].engine->setProcessEventsInterval( 100 );
    m_scripts[name].engine->evaluate( scriptFile.readAll() );
    scriptFile.close();

    if ( m_scripts[name].evaluating )
    {
        m_scripts[name].evaluating = false;
        if ( m_scripts[name].engine->hasUncaughtException() )
        {
            error() << "Script Error:" << time.currentTime().toString() + ' ' + m_scripts[name].engine->uncaughtException().toString() + " on Line: " + QString::number( m_scripts[name].engine->uncaughtExceptionLineNumber() );
            m_scripts[name].log += time.currentTime().toString() + ' ' + m_scripts[name].engine->uncaughtException().toString() + " on Line: " + QString::number( m_scripts[name].engine->uncaughtExceptionLineNumber() ) + '\n';
            m_scripts[name].engine->clearExceptions();
            slotStopScript( name );

            if ( !silent )
            {
                KMessageBox::sorry( 0, i18n( "There are exceptions caught in the script '%1'. Please refer to the console debug output for more information.", name ) );
                debug() << "The Log For the script that is the borked: " << m_scripts[name].log;
            }
            return false;
        }

        if( m_scripts[name].info.category() == "Scriptable Service" )
            ServiceScriptCustomize( name );
    }
    else
        slotStopScript( name );

    return true;
}

void
ScriptManager::slotStopScript( QString name )
{
    DEBUG_BLOCK
    //FIXME: Sometimes a script can be evaluating and cannot be abort? or can be reevaluating for some reason?
    if ( m_scripts[name].engine->isEvaluating() )
    {
        m_scripts[name].engine->abortEvaluation();
        m_scripts[name].evaluating = false;
        return;
    }
    if( m_scripts[name].info.category() == "Scriptable Service" )
        The::scriptableServiceManager()->removeRunningScript( name );
    if ( m_scripts[name].info.isPluginEnabled() )
    {
        m_scripts[name].info.setPluginEnabled( false );
        m_scripts[name].info.save();
    }
    scriptFinished( name );
}

void
ScriptManager::ServiceScriptPopulate( QString name, int level, int parent_id, QString path, QString filter )
{
    m_scripts[name].servicePtr->slotPopulate( name, level, parent_id, path, filter );
}

void ScriptManager::ServiceScriptCustomize( QString name )
{
    m_scripts[name].servicePtr->slotCustomize( name );
}

void ScriptManager::ServiceScriptRequestInfo( QString name, int level, QString callbackString )
{
    m_scripts[name].servicePtr->slotRequestInfo( name, level, callbackString );
}

void
ScriptManager::slotConfigChanged( bool changed )
{
    DEBUG_BLOCK
    if ( changed )
    {
        m_scriptSelector->save();
        foreach( const QString &key, m_scripts.keys() )
        {
            if( ( !m_scripts[key].running ) && ( m_scripts[key].info.isPluginEnabled() ) )
            {
                slotRunScript( m_scripts[key].info.name() );
            }
            if( ( m_scripts[key].running ) && ( !m_scripts[key].info.isPluginEnabled() ) )
            {
                slotStopScript( m_scripts[key].info.name() );
            }
        }
    }
}

void
ScriptManager::slotConfigComitted( const QByteArray & name )
{
    AMAROK_NOTIMPLEMENTED
    Q_UNUSED( name );
/*
    TODO: to be implemented
    debug() << "config comitted for: " << name;
    m_configChanged = true;
    m_changedScripts << QString( name );
*/
}

void
ScriptManager::scriptFinished( QString name ) //SLOT
{
    DEBUG_BLOCK
    //FIXME: probably can cause crash if you stop a script from evaluating. eg. if a deadlock is introduced in a menu_click_slot.
    if( !m_scripts.contains( name ) )
    {
        warning() << "Script is not in m_scripts?";
        return;
    }
    const QTime time;
    m_scripts[name].running = false;
    qDeleteAll( m_scripts[name].guiPtrList.begin(), m_scripts[name].guiPtrList.end() );
    m_scripts[name].guiPtrList.clear();
    qDeleteAll( m_scripts[name].wrapperList.begin(), m_scripts[name].wrapperList.end() );
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

	QStringList SupportAPIVersion;
	SupportAPIVersion << "API V1.0.0" << "API V1.0.1";
	QString ScriptVersion;
    if( !path.isEmpty() )
    {
        QFileInfo info( path );

        const QString specPath = info.path() + '/' + "script.spec";
        if( QFile::exists( specPath ) )
        {
            const KUrl url = KUrl( path );
            ScriptItem item;
            item.info = KPluginInfo( specPath );
            if ( !item.info.isValid() ) return false;
            if ( ( item.info.name() == "" ) || ( item.info.version() == "" ) || ( item.info.category() == "" ) ) return false;

            //assume it is API V1.0.0 if there is no "API V" prefix found
            if ( !item.info.dependencies().at(0).startsWith("API V") )
                ScriptVersion = "API V1.0.0";
            else
                ScriptVersion = item.info.dependencies().at(0);

            if ( !SupportAPIVersion.contains( ScriptVersion ) )
            {
                debug() << "script API version not compatible with Amarok.";
                return false;
            }

            if ( m_scripts.contains( item.info.name() ) ) return false; //check if script is already loaded...
            debug() << "script info:" << item.info.name() << " " << item.info.version() << " " << item.info.category() << " " << item.info.dependencies().at(0);
            item.info.setConfig( KGlobal::config()->group( item.info.name() ) );
            item.url = url;
            item.running = false;
            //FIXME: non english name will cause the crash
            m_scripts[item.info.name()] = item;
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
ScriptManager::findScripts()
{
    DEBUG_BLOCK

    // Add found scripts to treeWidget:
    QList<KPluginInfo> LyricsInfoList;
    QList<KPluginInfo> GenericInfoList;
    QList<KPluginInfo> ServiceInfoList;
    foreach( const QString &key, m_scripts.keys() )
    {
        if ( m_scripts[key].info.category() == "Generic" )
            GenericInfoList.append( m_scripts[key].info );
        else if ( m_scripts[key].info.category() == "Lyrics" )
            LyricsInfoList.append( m_scripts[key].info );
        else if ( m_scripts[key].info.category() == "Scriptable Service" )
            ServiceInfoList.append( m_scripts[key].info );
    }
    m_scriptSelector->addScripts( GenericInfoList, KPluginSelector::ReadConfigFile, "Generic" );
    m_scriptSelector->addScripts( LyricsInfoList, KPluginSelector::ReadConfigFile, "Lyrics" );
    m_scriptSelector->addScripts( ServiceInfoList, KPluginSelector::ReadConfigFile, "Scriptable Service" );

    // Handle auto-run:
    slotConfigChanged( true );
}

void
ScriptManager::startScriptEngine( QString name )
{
    DEBUG_BLOCK

    QScriptEngine* scriptEngine = m_scripts[name].engine;
    QObject* objectPtr = 0;
    QScriptValue scriptObject;

    objectPtr = new AmarokScript::ScriptImporter( scriptEngine, m_scripts[name].url );
    scriptObject = scriptEngine->newQObject( objectPtr );
    scriptEngine->globalObject().setProperty( "Importer", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    m_scripts[name].globalPtr = new AmarokScript::AmarokScript( name );
    m_global = scriptEngine->newQObject( m_scripts[name].globalPtr );
    scriptEngine->globalObject().setProperty( "Amarok", m_global );
    m_scripts[name].wrapperList.append( m_scripts[name].globalPtr );

    objectPtr = new AmarokScript::AmarokScriptConfig( name );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Script", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new InfoScript( m_scripts[name].url );
    QScriptValue infoContext = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Info", infoContext );
    m_scripts[name].wrapperList.append( objectPtr );
    scriptObject = scriptEngine->newQMetaObject( &IconEnum::staticMetaObject );
    infoContext.setProperty( "IconSizes", scriptObject );

    m_scripts[name].servicePtr = new ScriptableServiceScript( scriptEngine );
//    scriptObject = scriptEngine->newQObject( m_scripts[name].servicePtr );
//    m_global.setProperty( "ScriptableServiceScript", scriptObject );
    m_scripts[name].wrapperList.append( m_scripts[name].servicePtr );

    objectPtr = new StreamItem( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "StreamItem", scriptObject );
    scriptEngine->setDefaultPrototype( qMetaTypeId<StreamItem*>(), QScriptValue() );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokLyricsScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Lyrics", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokServicePluginManagerScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "ServicePluginManager", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokCollectionScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Collection", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokEngineScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Engine", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokWindowScript( scriptEngine, &m_scripts[name].guiPtrList );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Window", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokPlaylistScript( scriptEngine, &m_scripts[name].wrapperList );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Playlist", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokNetworkScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.setProperty( "Network", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );
    
    objectPtr = new Downloader( scriptEngine );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokStatusbarScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.property( "Window" ).setProperty( "Statusbar", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    objectPtr = new AmarokScript::AmarokOSDScript( scriptEngine );
    scriptObject = scriptEngine->newQObject( objectPtr );
    m_global.property( "Window" ).setProperty( "OSD", scriptObject );
    m_scripts[name].wrapperList.append( objectPtr );

    scriptObject = scriptEngine->newObject();
    m_global.property( "Window" ).setProperty( "ToolsMenu", scriptObject );

    scriptObject = scriptEngine->newObject();
    m_global.property( "Window" ).setProperty( "SettingsMenu", scriptObject );

    MetaTrackPrototype* trackProto = new MetaTrackPrototype();
    scriptEngine->setDefaultPrototype( qMetaTypeId<Meta::TrackPtr>(),
                                scriptEngine->newQObject( trackProto ) );
    m_scripts[name].wrapperList.append( trackProto );
}

#include "ScriptManager.moc"

