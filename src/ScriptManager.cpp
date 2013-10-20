/****************************************************************************************
 * Copyright (c) 2004-2010 Mark Kretschmann <kretschmann@kde.org>                       *
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

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "MainWindow.h"
#include "amarokconfig.h"
#include <config.h> // for the compile flags
#include "services/scriptable/ScriptableServiceManager.h"
#include "scriptengine/AmarokCollectionScript.h"
#include "scriptengine/AmarokScriptConfig.h"
#include "scriptengine/AmarokEngineScript.h"
#include "scriptengine/AmarokInfoScript.h"
#include "scriptengine/AmarokKNotifyScript.h"
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

#include <KMessageBox>
#include <KStandardDirs>

#include <QFileInfo>
#include <QTimer>
#include <QScriptEngine>

#include <sys/stat.h>
#include <sys/types.h>

////////////////////////////////////////////////////////////////////////////////
// class ScriptManager
////////////////////////////////////////////////////////////////////////////////

ScriptManager* ScriptManager::s_instance = 0;

ScriptManager::ScriptManager( QObject* parent )
    : QObject( parent )
{
    DEBUG_BLOCK
    setObjectName( "ScriptManager" );

    s_instance = this;

    if( AmarokConfig::enableScripts() == false )
    {
        if( !minimumBindingsAvailable() )
        {
            KMessageBox::error( 0,
                                i18n( "Scripts have been disabled since you are missing the QtScriptQtBindings "
                                      "package. Please install the package and restart Amarok for scripts to work." ),
                                i18n( "Scripts Disabled!" )  );
            return;
        }
        AmarokConfig::setEnableScripts( true );
    }

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, SLOT(updateAllScripts()) );
}

bool
ScriptManager::minimumBindingsAvailable()
{
    QStringList minimumBindings;
    minimumBindings << "qt.core" << "qt.gui" << "qt.sql" << "qt.xml" << "qt.uitools" << "qt.network";
    QScriptEngine engine;
    foreach( const QString &binding, minimumBindings )
    {
        // simply compare with availableExtensions()? Or can import still fail?
        QScriptValue error = engine.importExtension( binding );
        if( error.isUndefined() )
            continue; // undefined indicates success

        debug() << "Extension" << binding <<  "not found:" << error.toString();
        debug() << "Available extensions:" << engine.availableExtensions();
        return false;
    }
    return true;
}

ScriptManager::~ScriptManager()
{
    DEBUG_BLOCK
    qDeleteAll( m_scripts.values() );
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
    if( name.isEmpty() )
        return false;
    if( !m_scripts.contains( name ) )
        return false;
    slotStopScript( name );
    return true;
}

QStringList
ScriptManager::listRunningScripts() const
{
    QStringList runningScripts;
    foreach( const ScriptItem *item, m_scripts )
    {
        if( item->running )
            runningScripts << item->info.pluginName();
    }
    return runningScripts;
}

QString
ScriptManager::specForScript( const QString& name ) const
{
    if( !m_scripts.contains( name ) )
        return QString();
    QFileInfo info( m_scripts.value(name)->url.path() );
    const QString specPath = QString( "%1/%2.spec" ).arg( info.path(), info.completeBaseName() );
    return specPath;
}

bool
ScriptManager::lyricsScriptRunning() const
{
    return !m_lyricsScript.isEmpty();
}

void
ScriptManager::notifyFetchLyrics( const QString& artist, const QString& title )
{
    DEBUG_BLOCK
    emit fetchLyrics( artist, title, QString() );
}

void
ScriptManager::notifyFetchLyricsByUrl( const QString& artist, const QString& title, const QString& url )
{
    DEBUG_BLOCK
    emit fetchLyrics( artist, title, url );
}

////////////////////////////////////////////////////////////////////////////////
// private slots (script updater stuff)
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::updateAllScripts() // SLOT
{
// note: we can't update scripts without the QtCryptoArchitecture, so don't even try
#ifdef QCA2_FOUND
    DEBUG_BLOCK
    // find all scripts (both in $KDEHOME and /usr)
    QStringList foundScripts = KGlobal::dirs()->findAllResources( "data", "amarok/scripts/*/main.js",
                                                                  KStandardDirs::Recursive |
                                                                  KStandardDirs::NoDuplicates );
    m_nScripts = foundScripts.count();

    // get timestamp of the last update check
    KConfigGroup config = Amarok::config( "ScriptManager" );
    const uint lastCheck = config.readEntry( "LastUpdateCheck", QVariant( 0 ) ).toUInt();
    const uint now = QDateTime::currentDateTime().toTime_t();

    // last update was at least 7 days ago -> check now if auto update is enabled
    if( AmarokConfig::autoUpdateScripts() && (now - lastCheck > 7*24*60*60) )
    {
        debug() << "ScriptUpdater: Performing script update check now!";
        for( int i = 0; i < m_nScripts; ++i )
        {
            ScriptUpdater *updater = new ScriptUpdater( this );
            // all the ScriptUpdaters are now started in parallel.
            // tell them which script to work on
            updater->setScriptPath( foundScripts.at( i ) );
            // tell them whom to signal when they're finished
            connect( updater, SIGNAL(finished(QString)), SLOT(updaterFinished(QString)) );
            // and finally tell them to get to work
            QTimer::singleShot( 0, updater, SLOT(updateScript()) );
        }
        // store current timestamp
        config.writeEntry( "LastUpdateCheck", QVariant( now ) );
        config.sync();
    }
    // last update was pretty recent, don't check again
    else
    {
        debug() << "ScriptUpdater: Skipping update check";
        for ( int i = 0; i < m_nScripts; i++ )
        {
            loadScript( foundScripts.at( i ) );
        }
        configChanged( true );
    }
#endif
}

void
ScriptManager::updaterFinished( const QString &scriptPath ) // SLOT
{
    DEBUG_BLOCK
    // count this event
    m_updateSemaphore.release();
    loadScript( scriptPath );
    if ( m_updateSemaphore.tryAcquire(m_nScripts) )
    {
        configChanged( true );
    }
    sender()->deleteLater();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::slotRunScript( const QString &name, bool silent )
{
    DEBUG_BLOCK

    ScriptItem *item = m_scripts.value( name );
    const KUrl url = item->url;
    //load the wrapper classes
    item->engine = new QScriptEngine( this );
    connect(item->engine,
            SIGNAL(signalHandlerException(QScriptValue)),
            SLOT(handleException(QScriptValue)));

    startScriptEngine( name );
    QFile scriptFile( url.path() );
    scriptFile.open( QIODevice::ReadOnly );
    item->info.setPluginEnabled( true );
    item->running = true;
    item->evaluating = true;
    if( item->info.category() == "Lyrics" )
    {
        m_lyricsScript = name;
        debug() << "lyrics script started:" << name;
        emit lyricsScriptStarted();
    }

    item->log << QString( "%1 Script started" ).arg( QTime::currentTime().toString() );
    item->engine->setProcessEventsInterval( 100 );
    item->engine->evaluate( scriptFile.readAll() );
    scriptFile.close();

    if ( item->evaluating )
    {
        item->evaluating = false;
        if ( item->engine->hasUncaughtException() )
        {
            QString errorString = QString( "Script Error: %1 (line: %2)" )
                .arg( item->engine->uncaughtException().toString() )
                .arg( item->engine->uncaughtExceptionLineNumber() );
            item->log << errorString;
            error() << errorString;
            item->engine->clearExceptions();
            slotStopScript( name );

            if( !silent )
            {
                debug() << "The Log For the script that is the borked: " << item->log;
            }
            return false;
        }

        if( item->info.category() == QLatin1String("Scriptable Service") )
            ServiceScriptCustomize( name );
    }
    else
        slotStopScript( name );

    return true;
}

void ScriptManager::handleException( const QScriptValue &value )
{
    DEBUG_BLOCK

    QScriptEngine * engine = value.engine();
    if (!engine)
        return;

    Amarok::Components::logger()->longMessage( i18n( "Script error reported by: %1\n%2", scriptNameForEngine( engine ), value.toString() ), Amarok::Logger::Error );
}

void
ScriptManager::slotStopScript( const QString &name )
{
    DEBUG_BLOCK

    ScriptItem *item = m_scripts.value( name );
    item->servicePtr = 0;
    if( !item->engine ) {
        warning() << "Script has no script engine attached:" << name;
        return;
    }

    //FIXME: Sometimes a script can be evaluating and cannot be abort? or can be reevaluating for some reason?
    if( item->engine->isEvaluating() )
    {
        item->engine->abortEvaluation();
        item->evaluating = false;
        return;
    }

    if( item->info.category() == "Scriptable Service" )
        The::scriptableServiceManager()->removeRunningScript( name );

    if( item->info.isPluginEnabled() )
    {
        debug() << "Disabling sccript" << item->info.pluginName();
        item->info.setPluginEnabled( false );
        item->info.save();
    }
    scriptFinished( name );
}

void
ScriptManager::ServiceScriptPopulate( const QString &name, int level, int parent_id,
                                      const QString &path, const QString &filter )
{
    if( m_scripts.value( name )->servicePtr )
        m_scripts.value( name )->servicePtr->slotPopulate( name, level, parent_id, path, filter );
}

void ScriptManager::ServiceScriptCustomize( const QString &name )
{
    if( m_scripts.value( name )->servicePtr )
        m_scripts.value( name )->servicePtr->slotCustomize( name );
}

void ScriptManager::ServiceScriptRequestInfo( const QString &name, int level, const QString &callbackString )
{
    if( m_scripts.value( name )->servicePtr )
        m_scripts.value( name )->servicePtr->slotRequestInfo( name, level, callbackString );
}

void
ScriptManager::configChanged( bool changed )
{
    DEBUG_BLOCK
    if( changed )
    {
        foreach( const ScriptItem *item, m_scripts )
        {
            const QString name = item->info.pluginName();
            bool enabledByDefault = item->info.isPluginEnabledByDefault();
            bool enabled = Amarok::config( "Plugins" ).readEntry( name + "Enabled", enabledByDefault );

            if( !item->running && enabled )
            {
                slotRunScript( name );
            }
            else if( item->running && !enabled )
            {
                slotStopScript( name );
            }
        }
    }
}

void
ScriptManager::scriptFinished( const QString &name )
{
    DEBUG_BLOCK
    //FIXME: probably can cause crash if you stop a script from evaluating. eg. if a deadlock is introduced in a menu_click_slot.
    if( !m_scripts.contains( name ) )
    {
        warning() << "Script is not in m_scripts?";
        return;
    }

    ScriptItem *item = m_scripts.value( name );
    item->log << QString( "%1 Script ended" ).arg( QTime::currentTime().toString() );
    delete item->engine;
    item->engine = 0;
    item->running = false;
    item->evaluating = false;
}


////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

bool
ScriptManager::loadScript( const QString& path )
{
    if( path.isEmpty() )
        return false;

    QStringList SupportAPIVersion;
    SupportAPIVersion << QLatin1String("API V1.0.0") << QLatin1String("API V1.0.1");
    QString ScriptVersion;
    QFileInfo info( path );
    const QString specPath = QString( "%1/script.spec" ).arg( info.path() );
    if( !QFile::exists( specPath ) )
    {
        error() << "script.spec for "<< path << " is missing!";
        return false;
    }

    KPluginInfo pluginInfo( specPath );
    if( !pluginInfo.isValid() )
    {
        error() << "PluginInfo invalid for" << specPath;
        return false;
    }

    const QString pluginName = pluginInfo.pluginName();
    const QString category   = pluginInfo.category();
    const QString version    = pluginInfo.version();
    const QStringList dependencies = pluginInfo.dependencies();

    if( pluginName.isEmpty() || category.isEmpty() || version.isEmpty()
        || dependencies.isEmpty() )
    {
        error() << "PluginInfo has empty values for" << specPath;
        return false;
    }

    if( !m_scripts.contains( pluginName ) )
    {
        ScriptItem *item = new ScriptItem;
        m_scripts[ pluginName ] = item;
    }

    ScriptItem *item = m_scripts.value( pluginName );
    item->info = pluginInfo;

    //assume it is API V1.0.0 if there is no "API V" prefix found
    if( !item->info.dependencies().at(0).startsWith("API V") )
        ScriptVersion = QLatin1String("API V1.0.0");
    else
        ScriptVersion = item->info.dependencies().at(0);

    if( !SupportAPIVersion.contains( ScriptVersion ) )
    {
        warning() << "script API version not compatible with Amarok.";
        return false;
    }

    debug() << "found script:" << category << pluginName << version << item->info.dependencies();
    item->url = KUrl( path );
    item->running = false;
    return true;
}

KPluginInfo::List
ScriptManager::scripts( const QString &category ) const
{
    KPluginInfo::List scripts;
    foreach( const ScriptItem *script, m_scripts )
    {
        if( script->info.category() == category )
            scripts << script->info;
    }
    return scripts;
}

QString ScriptManager::scriptNameForEngine(const QScriptEngine* engine) const
{
    foreach( const QString& name, m_scripts.keys() ) {
        ScriptItem *script = m_scripts[name];
        if (script->engine == engine)
            return name;
    }

    return QString();
}

void
ScriptManager::startScriptEngine( const QString &name )
{
    DEBUG_BLOCK

    debug() << "start script engine:" << name;
    ScriptItem *item = m_scripts.value( name );
    QScriptEngine* scriptEngine = item->engine;

    // first create the Amarok global script object
    new AmarokScript::AmarokScript( name, scriptEngine );

    // common utils
    new AmarokScript::ScriptImporter( scriptEngine, item->url );
    new AmarokScript::AmarokScriptConfig( name, scriptEngine );
    new AmarokScript::InfoScript( item->url, scriptEngine );
    new AmarokNetworkScript( scriptEngine );
    new Downloader( scriptEngine );

    // backend
    new AmarokScript::AmarokCollectionScript( scriptEngine );
    new AmarokScript::AmarokEngineScript( scriptEngine );

    // UI
    new AmarokScript::AmarokWindowScript( scriptEngine );
    new AmarokScript::AmarokPlaylistScript( scriptEngine );
    new AmarokScript::AmarokStatusbarScript( scriptEngine );
    new AmarokScript::AmarokKNotifyScript( scriptEngine );
    new AmarokScript::AmarokOSDScript( scriptEngine );
    QScriptValue windowObject = scriptEngine->globalObject().property( "Amarok" ).property( "Window" );
    windowObject.setProperty( "ToolsMenu", scriptEngine->newObject() );
    windowObject.setProperty( "SettingsMenu", scriptEngine->newObject() );

    const QString &category = item->info.category();
    if( category == QLatin1String("Lyrics") )
    {
        new AmarokScript::AmarokLyricsScript( scriptEngine );
    }
    else if( category == QLatin1String("Scriptable Service") )
    {
        new StreamItem( scriptEngine );
        item->servicePtr = new ScriptableServiceScript( scriptEngine );
        new AmarokScript::AmarokServicePluginManagerScript( scriptEngine );
    }

    MetaTrackPrototype* trackProto = new MetaTrackPrototype( this );
    scriptEngine->setDefaultPrototype( qMetaTypeId<Meta::TrackPtr>(),
                                       scriptEngine->newQObject(trackProto, QScriptEngine::AutoOwnership) );
}

ScriptItem::ScriptItem()
    : engine( 0 )
    , running( false )
    , servicePtr( 0 )
{}

ScriptItem::~ScriptItem()
{
    delete engine;
}

#include "ScriptManager.moc"
