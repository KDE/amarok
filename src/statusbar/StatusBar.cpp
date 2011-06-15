/****************************************************************************************
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "StatusBar"

#include "statusbar/StatusBar.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "LongMessageWidget.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/interfaces/Logger.h"
#include "core/support/Components.h"
#include "core-impl/logger/ProxyLogger.h"
#include "playlist/PlaylistItem.h"
#include "playlist/PlaylistModelStack.h"

#include "KJobProgressBar.h"
#include "NetworkProgressBar.h"

#include <QNetworkReply>
#include <QLabel>
#include <QFrame>
#include <QVariant>

#include <cmath>

class LoggerAdaptor : public Amarok::Logger
{
public:
    LoggerAdaptor( StatusBar *bar )
        : m_statusBar( bar )
    {
        setParent( bar );
    }

    virtual void shortMessage( const QString &text )
    {
        m_statusBar->shortMessage( text );
    }

    virtual void longMessage( const QString &text, MessageType type )
    {
        StatusBar::MessageType otherType = StatusBar::Information;
        switch( type )
        {
        case Amarok::Logger::Information:
            otherType = StatusBar::Information;
            break;
        case Amarok::Logger::Warning:
            otherType = StatusBar::Warning;
            break;
        case Amarok::Logger::Error:
            otherType = StatusBar::Error;
            break;
        }
        m_statusBar->longMessage( text, otherType );
    }

    virtual void newProgressOperation( KJob *job, const QString &text, QObject *obj,
                                      const char *slot, Qt::ConnectionType type )
    {
        ProgressBar *bar = m_statusBar->newProgressOperation( job, text );
        if( obj )
        {
            bar->setAbortSlot( obj, slot, type );
        }
    }

    virtual void newProgressOperation( QNetworkReply *reply, const QString &text, QObject *obj,
                                      const char *slot, Qt::ConnectionType type )
    {
        ProgressBar *bar = m_statusBar->newProgressOperation( reply, text );
        if( obj )
        {
            bar->setAbortSlot( obj, slot, type );
        }
    }

private:
    StatusBar *m_statusBar;
};

StatusBar *StatusBar::s_instance = 0;

namespace The
{
    StatusBar *statusBar()
    {
        return StatusBar::instance();
    }
}

StatusBar::StatusBar( QWidget *parent )
        : KStatusBar( parent )
        , m_progressBar( new CompoundProgressBar( this ) )
        , m_busy( false )
        , m_shortMessageTimer( new QTimer( this ) )
{
    s_instance = this;

    m_progressArea = new QFrame( this );
    m_progressArea->setLayout( new QVBoxLayout( m_progressArea ) );

    m_progressArea->layout()->addWidget( m_progressBar );
    m_progressBar->hide();

    m_messageLabel = new QLabel( m_progressBar );
    m_messageLabel->setAlignment( Qt::AlignCenter );
    m_messageLabel->setWordWrap( true );
    m_progressArea->layout()->addWidget( m_messageLabel );
    m_messageLabel->hide();

    connect( m_progressBar, SIGNAL( allDone() ), this, SLOT( hideProgress() ) );

    //setMaximumSize( The::mainWindow()->width(), m_progressBar->height() );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setContentsMargins( 0, 0, 0, 0 );
    setSizeGripEnabled( false );

    m_shortMessageTimer->setSingleShot( true );
    connect( m_shortMessageTimer, SIGNAL( timeout() ), this, SLOT( nextShortMessage() ) );

    qRegisterMetaType<MessageType>( "MessageType" );
    connect( this, SIGNAL( signalLongMessage( const QString &, MessageType ) ),
             SLOT( slotLongMessage( const QString &, MessageType ) ), Qt::QueuedConnection );

    Amarok::Logger *logger = Amarok::Components::logger();
    ProxyLogger *proxy = qobject_cast<ProxyLogger*>( logger );
    if( proxy )
        proxy->setLogger( new LoggerAdaptor( this ) );
    else
        warning() << "Was not able to register statusbar as logger";
}


StatusBar::~StatusBar()
{
    DEBUG_BLOCK

    delete m_progressBar;
    m_progressBar = 0;

    s_instance = 0;
}

ProgressBar *StatusBar::newProgressOperation( QObject *owner, const QString & description )
{
    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    ProgressBar *newBar = new ProgressBar( 0 );
    newBar->setDescription( description );
    connect( owner, SIGNAL(destroyed( QObject * )), SLOT(endProgressOperation( QObject * )) );
    m_progressBar->addProgressBar( newBar, owner );
    m_progressBar->show();
    m_busy = true;

    return newBar;
}

ProgressBar *StatusBar::newProgressOperation( KJob *job, const QString &description )
{
    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    KJobProgressBar *newBar = new KJobProgressBar( 0, job );
    newBar->setDescription( description );
    connect( job, SIGNAL(destroyed( QObject * )), this, SLOT(endProgressOperation( QObject * )) );
    m_progressBar->addProgressBar( newBar, job );
    m_progressBar->show();
    m_busy = true;

    return newBar;
}

ProgressBar *StatusBar::newProgressOperation( QNetworkReply *reply, const QString &description )
{
    //clear any short message currently being displayed and stop timer if running...
    clearMessage();
    m_shortMessageTimer->stop();

    NetworkProgressBar *newBar = new NetworkProgressBar( 0, reply );
    newBar->setDescription( description );
    newBar->setAbortSlot( reply, SLOT(deleteLater()) );
    connect( reply, SIGNAL(destroyed( QObject * )), SLOT(endProgressOperation( QObject * )) );
    m_progressBar->addProgressBar( newBar, reply );
    m_progressBar->show();
    m_busy = true;

    return newBar;
}

void StatusBar::shortMessage( const QString &text )
{
    if( !m_busy )
    {
        //not busy, so show right away
        showMessageInProgressArea( text );
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        m_shortMessageQue.append( text );
    }
}

void StatusBar::hideProgress()
{
    DEBUG_BLOCK

    m_progressBar->hide();
    m_busy = false;

    nextShortMessage();
}

void StatusBar::nextShortMessage()
{
    if( m_shortMessageQue.count() > 0 )
    {
        m_busy = true;
        showMessageInProgressArea( m_shortMessageQue.takeFirst() );
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        clearMessageInProgressArea();
        m_busy = false;
    }
}

void StatusBar::longMessage( const QString &text, MessageType type )
{
    DEBUG_BLOCK

    // The purpose of this emit is to make the operation thread safe. If this
    // method is called from a non-GUI thread, the "emit" relays it over the
    // event loop to the GUI thread, so that we can safely create widgets.
    emit signalLongMessage( text, type );
}

void StatusBar::slotLongMessage( const QString &text, MessageType type ) //SLOT
{
    DEBUG_BLOCK

    debug() << "Text:" << text;

    LongMessageWidget *message = new LongMessageWidget( this, text, type );
    connect( message, SIGNAL( closed() ), this, SLOT( hideLongMessage() ) );
}

void StatusBar::hideLongMessage()
{
    sender()->deleteLater();
}

void
StatusBar::showMessageInProgressArea( const QString &message )
{
    m_busy = true;
    m_messageLabel->setText( message );
    m_messageLabel->show();
}

void
StatusBar::clearMessageInProgressArea()
{
    m_busy = false;
    m_messageLabel->hide();
}

#include "StatusBar.moc"
