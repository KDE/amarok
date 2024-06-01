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
#include "core/logger/Logger.h"
#include "MainWindow.h"
#include "amarokconfig.h"
#include <config.h> // for the compile flags
#include "services/scriptable/ScriptableServiceManager.h"
#include "ScriptItem.h"

#include <KMessageBox>
#include <KPluginInfo>
#include <KPluginMetaData>

#include <QFileInfo>
#include <QJSEngine>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QTimer>
#include <QDir>
#include <QTemporaryFile>

#include <sys/stat.h>
#include <sys/types.h>


ScriptManager* ScriptManager::s_instance = nullptr;

ScriptManager::ScriptManager( QObject* parent )
    : QObject( parent )
{
    DEBUG_BLOCK
    setObjectName( "ScriptManager" );

    s_instance = this;

    if( AmarokConfig::enableScripts() == false )
    {
        AmarokConfig::setEnableScripts( true );
    }

    // Delay this call via eventloop, because it's a bit slow and would block
    QTimer::singleShot( 0, this, &ScriptManager::updateAllScripts );
}

ScriptManager::~ScriptManager()
{}

void
ScriptManager::destroy() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
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
    m_scripts[name]->stop();
    return true;
}

QStringList
ScriptManager::listRunningScripts() const
{
    QStringList runningScripts;
    foreach( const ScriptItem *item, m_scripts )
    {
        if( item->running() )
            runningScripts << item->info().pluginName();
    }
    return runningScripts;
}

QString
ScriptManager::specForScript( const QString& name ) const
{
    if( !m_scripts.contains( name ) )
        return QString();
    return m_scripts[name]->specPath();
}

KPluginMetaData
ScriptManager::createMetadaFromSpec( const QString &specPath )
{
    // KPluginMetaData and KPluginInfo require file suffix to be .desktop. Thus create temporary file with suffix
    QFile specFile( specPath );
    QTemporaryFile desktopFile( QDir::tempPath() + "/XXXXXX.desktop" );

    if ( !specFile.open( QIODevice::ReadOnly ) ) {
        warning() << "Could not read from spec file: " << specPath;
        return KPluginMetaData();
    } else if ( !desktopFile.open() ) {
        warning() << "Could not create temporary .desktop file at " << QDir::tempPath();
        return KPluginMetaData();
    }

    QTextStream( &desktopFile ) << QTextStream( &specFile ).readAll();
    desktopFile.close();

    return KPluginMetaData( desktopFile.fileName() );
}

bool
ScriptManager::lyricsScriptRunning() const
{
    return !m_lyricsScript.isEmpty();
}

void
ScriptManager::notifyFetchLyrics( const QString& artist, const QString& title, const QString& url, const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    Q_EMIT fetchLyrics( artist, title, url, track );
}

////////////////////////////////////////////////////////////////////////////////
// private slots (script updater stuff)
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::updateAllScripts() // SLOT
{
    DEBUG_BLOCK
    // Disable script updating for now. TODO review situation post 3.0
    debug() << "Script updating hard disabled, not doing an update";
    return;

    // find all scripts (both in $KDEHOME and /usr)
    QStringList foundScripts;
    QStringList locations = QStandardPaths::standardLocations( QStandardPaths::GenericDataLocation );
    for( const auto &location : locations )
    {
        QDir dir( location + "/amarok/scripts" );

        if( !dir.exists() )
            continue;

        for( const auto &scriptLocation : dir.entryList( QDir::NoDotAndDotDot | QDir::Dirs ) )
        {
            QDir scriptDir( dir.absoluteFilePath( scriptLocation ) );
            if( scriptDir.exists( QStringLiteral( "main.js" ) ) )
                foundScripts << scriptDir.absoluteFilePath( QStringLiteral( "main.js" ) );
        }
    }

    // remove deleted scripts
    foreach( ScriptItem *item, m_scripts )
    {
        const QString specPath = QString( "%1/script.spec" ).arg( QFileInfo( item->url().path() ).path() );
        if( !QFile::exists( specPath ) )
        {
            debug() << "Removing script " << item->info().pluginName();
            item->uninstall();
            m_scripts.remove( item->info().pluginName() );
        }
    }

    m_nScripts = foundScripts.count();

    // get timestamp of the last update check
    KConfigGroup config = Amarok::config( "ScriptManager" );
    const uint lastCheck = config.readEntry( "LastUpdateCheck", QVariant( 0 ) ).toUInt();
    const uint now = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    bool autoUpdateScripts = AmarokConfig::autoUpdateScripts();
    // note: we can't update scripts without the QtCryptoArchitecture, so don't even try
    #ifndef QCA2_FOUND
    autoUpdateScripts = false;
    #endif

    // last update was at least 7 days ago -> check now if auto update is enabled
    if( autoUpdateScripts && (now - lastCheck > 7*24*60*60) )
    {
        debug() << "ScriptUpdater: Performing script update check now!";
        for( int i = 0; i < m_nScripts; ++i )
        {
            /*ScriptUpdater *updater = new ScriptUpdater( this );
            // all the ScriptUpdaters are now started in parallel.
            // tell them which script to work on
            updater->setScriptPath( foundScripts.at( i ) );
            // tell them whom to signal when they're finished
            connect( updater, &ScriptUpdater::finished, this, &ScriptManager::updaterFinished );
            // and finally tell them to get to work
            QTimer::singleShot( 0, updater, &ScriptUpdater::updateScript );*/
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
    ScriptItem *item = m_scripts.value( name );
    connect( item, &ScriptItem::signalHandlerException,
             this, &ScriptManager::handleException );
    if( item->info().category() == "Lyrics" )
    {
        m_lyricsScript = name;
        debug() << "lyrics script started:" << name;
        Q_EMIT lyricsScriptStarted();
    }
    return item->start( silent );
}

void
ScriptManager::handleException(const QJSValue& value)
{
    DEBUG_BLOCK

    Amarok::Logger::longMessage( i18n( "Script error reported by: %1\n%2", value.property("name").toString(), value.property("message").toString() ), Amarok::Logger::Error );
}

void
ScriptManager::ServiceScriptPopulate( const QString &name, int level, int parent_id,
                                      const QString &path, const QString &filter )
{
    if( m_scripts.value( name )->service() )
        m_scripts.value( name )->service()->slotPopulate( name, level, parent_id, path, filter );
}

void
ScriptManager::ServiceScriptCustomize( const QString &name )
{
    if( m_scripts.value( name )->service() )
        m_scripts.value( name )->service()->slotCustomize( name );
}

void
ScriptManager::ServiceScriptRequestInfo( const QString &name, int level, const QString &callbackString )
{
    if( m_scripts.value( name )->service() )
        m_scripts.value( name )->service()->slotRequestInfo( name, level, callbackString );
}

void
ScriptManager::configChanged( bool changed )
{
    Q_EMIT scriptsChanged();
    if( !changed )
        return;
    //evil scripts may prevent the config dialog from dismissing, delay execution
    QTimer::singleShot( 0, this, &ScriptManager::slotConfigChanged );
}

////////////////////////////////////////////////////////////////////////////////
// private
////////////////////////////////////////////////////////////////////////////////

void
ScriptManager::slotConfigChanged()
{
    foreach( ScriptItem *item, m_scripts )
    {
        const QString name = item->info().pluginName();
        bool enabledByDefault = item->info().isPluginEnabledByDefault();
        bool enabled = Amarok::config( "Plugins" ).readEntry( name + "Enabled", enabledByDefault );

        if( !item->running() && enabled )
        {
            slotRunScript( name );
        }
        else if( item->running() && !enabled )
        {
            item->stop();
        }
    }
}

bool
ScriptManager::loadScript( const QString& path )
{
    if( path.isEmpty() )
        return false;

    QStringList SupportAPIVersion;
    //SupportAPIVersion << QLatin1String("API V1.0.0") << QLatin1String("API V1.0.1");
    QString ScriptVersion;
    QFileInfo info( path );
    const QString jsonPath = QString( "%1/script.json" ).arg( info.path() );
    const QString specPath = QString( "%1/script.spec" ).arg( info.path() );
    KPluginMetaData pluginMetadata;

    if( QFile::exists( jsonPath ) )
    {
        pluginMetadata = KPluginMetaData( jsonPath );
    }
    else if( QFile::exists( specPath ) )
    {
        warning() << "Reading legacy spec file: " << specPath;
        pluginMetadata = createMetadaFromSpec( specPath );
    }
    else
    {
        error() << "script.json for "<< path << " is missing!";
        return false;
    }

    if( !pluginMetadata.isValid() )
    {
        error() << "PluginMetaData invalid for" << jsonPath;
        return false;
    }

    const QString pluginName = pluginMetadata.pluginId();
    const QString category   = pluginMetadata.category();
    const QString version    = pluginMetadata.version();

    if( pluginName.isEmpty() || category.isEmpty() || version.isEmpty() )
    {
        error() << "PluginMetaData has empty values for" << jsonPath;
        return false;
    }

    KPluginInfo pluginInfo( pluginMetadata );

    ScriptItem *item;
    if( !m_scripts.contains( pluginName ) )
    {
        item = new ScriptItem( this, pluginName, path, pluginInfo );
        m_scripts[ pluginName ] = item;
    }
    else if( m_scripts[pluginName]->info().version() < pluginInfo.version() )
    {
        m_scripts[ pluginName ]->deleteLater();
        item = new ScriptItem( this, pluginName, path, pluginInfo );
        m_scripts[ pluginName ] = item;
    }
    else
        item = m_scripts.value( pluginName );

    //assume it is API V1.0.0 if there is no "API V" prefix found
    /*if( !item->info().dependencies().at(0).startsWith("API V") )
        ScriptVersion = QLatin1String("API V1.0.0");
    else
        ScriptVersion = item->info().dependencies().at(0);

    if( !SupportAPIVersion.contains( ScriptVersion ) )
    {*/
        warning() << "script API version not compatible with Amarok.";
    /*  return false;
    }*/

    debug() << "found script:" << category << pluginName << version; // << item->info().dependencies();
    return true;
}

KPluginInfo::List
ScriptManager::scripts( const QString &category ) const
{
    KPluginInfo::List scripts;
    foreach( const ScriptItem *script, m_scripts )
    {
        if( script->info().category() == category )
            scripts << script->info();
    }
    return scripts;
}

QString
ScriptManager::scriptNameForEngine( const QJSEngine *engine ) const
{
    foreach( const QString &name, m_scripts.keys() )
    {
        ScriptItem *script = m_scripts[name];
        if( script->engine() == engine )
            return name;
    }

    return QString();
}
