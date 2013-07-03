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

#include "ScriptItem.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "MainWindow.h"
#include "amarokconfig.h"
#include "config.h"
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
#include "scriptengine/exporters/CollectionTypeExporter.h"
#include "scriptengine/exporters/MetaTypeExporter.h"
#include "scriptengine/exporters/QueryMakerExporter.h"
#include "scriptengine/ScriptImporter.h"
#include "ScriptManager.h"
#include "ScriptUpdater.h"

#include <KStandardDirs>
#include <KPushButton>

#include <QToolTip>
#include <QFileInfo>
#include <QScriptEngine>

////////////////////////////////////////////////////////////////////////////////
// ScriptTerminatorWidget
////////////////////////////////////////////////////////////////////////////////

ScriptTerminatorWidget::ScriptTerminatorWidget( const QString &message )
: PopupWidget( 0 )
{
    setFrameStyle( QFrame::StyledPanel | QFrame::Raised );

    setContentsMargins( 4, 4, 4, 4 );

    setMinimumWidth( 26 );
    setMinimumHeight( 26 );
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );

    QPalette p = QToolTip::palette();
    setPalette( p );

    QLabel *alabel = new QLabel( message, this );
    alabel->setWordWrap( true );
    alabel->setTextFormat( Qt::RichText );
    alabel->setTextInteractionFlags( Qt::TextBrowserInteraction );
    alabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
    alabel->setPalette( p );

    KPushButton *button = new KPushButton( i18n( "Terminate" ), this );
    button->setPalette(p);;
    connect( button, SIGNAL(clicked()), SIGNAL(terminate()) );
    button = new KPushButton( KStandardGuiItem::close(), this );
    button->setPalette(p);
    connect( button, SIGNAL(clicked()), SLOT(hide()) );

    reposition();
}

////////////////////////////////////////////////////////////////////////////////
// ScriptItem
////////////////////////////////////////////////////////////////////////////////


ScriptItem::ScriptItem( QObject *parent, const QString &name, const QString &path, const KPluginInfo &info )
: QObject( parent )
, m_name( name )
, m_url( path )
, m_info( info )
, m_running( false )
, m_evaluating( false )
, m_runningTime( 0 )
, m_timerId( 0 )
{}

void
ScriptItem::stop()
{
    DEBUG_BLOCK
    if( !m_engine ) {
        warning() << "Script has no script engine attached:" << m_name;
        return;
    }

    killTimer( m_timerId );
    if( m_popupWidget )
    {
        m_popupWidget.data()->hide();
        m_popupWidget.data()->deleteLater();;
    }
    //FIXME: Sometimes a script can be evaluating and cannot be abort? or can be reevaluating for some reason?
    if( m_engine.data()->isEvaluating() )
    {
        m_engine.data()->abortEvaluation();
        m_evaluating = false;
        return;
    }
    if( m_info.category() == "Scriptable Service" )
        The::scriptableServiceManager()->removeRunningScript( m_name );

    if( m_info.isPluginEnabled() )
    {
        debug() << "Disabling script" << m_info.pluginName();
        m_info.setPluginEnabled( false );
        m_info.save();
    }

    m_log << QString( "%1 Script ended" ).arg( QTime::currentTime().toString() );
    m_engine.data()->deleteLater();
    m_running = false;
    m_evaluating = false;
}

QString
ScriptItem::specPath() const
{
    QFileInfo info( m_url.path() );
    const QString specPath = QString( "%1/%2.spec" ).arg( info.path(), info.completeBaseName() );
    return specPath;
}

void
ScriptItem::timerEvent( QTimerEvent* event )
{
    Q_UNUSED( event )
    if( m_engine && m_engine.data()->isEvaluating() )
    {
        m_runningTime += 100;
        if( m_runningTime >= 5000 )
        {
            debug() << "5 seconds passed evaluating" << m_name;
            m_runningTime = 0;
            if( !m_popupWidget )
                m_popupWidget = new ScriptTerminatorWidget(
                    i18n( "Script %1 has been evaluating for over"
                    " 5 seconds now, terminate?"
                    , m_name ) );
            connect( m_popupWidget.data(), SIGNAL(terminate()), SLOT(stop()) );
            m_popupWidget.data()->show();
        }
    }
    else
    {
        if( m_popupWidget )
            m_popupWidget.data()->deleteLater();
        m_runningTime = 0;
    }
}

bool
ScriptItem::start( bool silent )
{
    DEBUG_BLOCK
    //load the wrapper classes
    initializeScriptEngine();

    QFile scriptFile( m_url.path() );
    scriptFile.open( QIODevice::ReadOnly );
    m_running = true;
    m_evaluating = true;

    QString( "%1 Script started" ).arg( QTime::currentTime().toString() );

    m_timerId = startTimer( 100 );
    m_engine.data()->evaluate( scriptFile.readAll() );
    debug() << "After Evaluation "<< m_name;
    scriptFile.close();

    if ( m_evaluating )
    {
        m_evaluating = false;
        if ( m_engine.data()->hasUncaughtException() )
        {
            QString errorString = QString( "Script Error: %1 (line: %2)" )
                            .arg( m_engine.data()->uncaughtException().toString() )
                            .arg( m_engine.data()->uncaughtExceptionLineNumber() );
            m_log << errorString;
            error() << errorString;
            m_engine.data()->clearExceptions();
            stop();

            if( !silent )
            {
                debug() << "The Log For the script that is the borked: " << m_log;
            }
            return false;
        }
        if( m_info.category() == QLatin1String("Scriptable Service") )
            m_service.data()->slotCustomize( m_name );
    }
    else
        stop();
    return true;
}

void
ScriptItem::initializeScriptEngine()
{
    DEBUG_BLOCK

    m_engine = new QScriptEngine( this );
    connect( m_engine.data(), SIGNAL(signalHandlerException(QScriptValue)), this,
             SIGNAL(signalHandlerException(QScriptValue)));
    m_engine.data()->setProcessEventsInterval( 50 );
    debug() << "starting script engine:" << m_name;

    // first create the Amarok global script object
    new AmarokScript::AmarokScript( m_name, m_engine.data() );

    // common utils
    new AmarokScript::ScriptImporter( m_engine.data(), m_url );
    new AmarokScript::AmarokScriptConfig( m_name, m_engine.data() );
    new AmarokScript::InfoScript( m_url, m_engine.data() );
    new AmarokNetworkScript( m_engine.data() );
    new Downloader( m_engine.data() );

    // backend
    new AmarokScript::AmarokCollectionScript( m_engine.data() );
    new AmarokScript::AmarokEngineScript( m_engine.data() );

    // UI
    new AmarokScript::AmarokWindowScript( m_engine.data() );
    new AmarokScript::AmarokPlaylistScript( m_engine.data() );
    new AmarokScript::AmarokStatusbarScript( m_engine.data() );
    new AmarokScript::AmarokKNotifyScript( m_engine.data() );
    new AmarokScript::AmarokOSDScript( m_engine.data() );

    AmarokScript::CollectionPrototype::init( m_engine.data() );
    AmarokScript::QueryMakerPrototype::init( m_engine.data() );

    const QString &category = m_info.category();
    if( category == QLatin1String("Lyrics") )
    {
        new AmarokScript::AmarokLyricsScript( m_engine.data() );
    }
    else if( category == QLatin1String("Scriptable Service") )
    {
        new StreamItem( m_engine.data() );
        m_service = new ScriptableServiceScript( m_engine.data() );
        new AmarokScript::AmarokServicePluginManagerScript( m_engine.data() );
    }

    new AmarokScript::MetaTrackPrototype( m_engine.data() );
}
