/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org                              *
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

#include "BrowserMessageArea.h"
#include "KJobProgressBar.h"
#include "LongMessageWidget.h"
#include "NetworkProgressBar.h"

#define SHORT_MESSAGE_DURATION 5000
#define POPUP_MESSAGE_DURATION 5000

BrowserMessageArea::BrowserMessageArea( QWidget *parent )
    : QFrame( parent )
    , m_busy( false )
{
    setLayout( new QVBoxLayout( this ) );

    m_progressBar = new CompoundProgressBar( this );
    connect( m_progressBar, SIGNAL(allDone()), this, SLOT(hideProgress()) );
    layout()->addWidget( m_progressBar );
    m_progressBar->hide();

    m_messageLabel = new QLabel( this );
    m_messageLabel->setAlignment( Qt::AlignCenter );
    m_messageLabel->setWordWrap( true );
    layout()->addWidget( m_messageLabel );
    m_messageLabel->hide();

    m_shortMessageTimer = new QTimer( this );
    m_shortMessageTimer->setSingleShot( true );
    connect( m_shortMessageTimer, SIGNAL(timeout()), SLOT(nextShortMessage()) );

    //register to carry MessageType accross threads
    qRegisterMetaType<Amarok::Logger::MessageType>( "MessageType" );
    connect( this, SIGNAL(signalLongMessage( const QString &, MessageType )),
             this, SLOT(slotLongMessage( const QString &, MessageType )),
             Qt::QueuedConnection );
}

void
BrowserMessageArea::shortMessage( const QString &text )
{
    if( !m_busy )
    {
        m_busy = true;
        m_messageLabel->setText( text );
        m_messageLabel->show();
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        m_shortMessageQueue.append( text );
    }
}

void
BrowserMessageArea::longMessage( const QString &text, MessageType type )
{
    // The purpose of this emit is to make the operation thread safe. If this
    // method is called from a non-GUI thread, the "emit" relays it over the
    // event loop to the GUI thread, so that we can safely create widgets.
    emit signalLongMessage( text, type );
}

void
BrowserMessageArea::newProgressOperation( KJob *job, const QString &text, QObject *obj,
                                      const char *slot, Qt::ConnectionType type )
{
    KJobProgressBar *newBar = new KJobProgressBar( 0, job );
    newBar->setDescription( text );
    connect( job, SIGNAL(destroyed( QObject * )), newBar,
             SLOT(endProgressOperation( QObject * )) );
    newBar->setAbortSlot( obj, slot, type );
    m_progressBar->addProgressBar( newBar, job );
    m_progressBar->show();

    m_busy = true;
}

void
BrowserMessageArea::newProgressOperation( QNetworkReply *reply, const QString &text, QObject *obj,
                                          const char *slot, Qt::ConnectionType type )
{
    NetworkProgressBar *newBar = new NetworkProgressBar( 0, reply );
    newBar->setDescription( text );
    newBar->setAbortSlot( reply, SLOT(deleteLater()) );
    connect( reply, SIGNAL(destroyed( QObject * )), newBar,
             SLOT(endProgressOperation( QObject * )) );
    newBar->setAbortSlot( obj, slot, type );
    m_progressBar->addProgressBar( newBar, reply );
    m_progressBar->show();

    m_busy = true;
}

void
BrowserMessageArea::newProgressOperation( QObject *sender, const QString &text, int maximum,
                                          QObject *obj, const char *slot, Qt::ConnectionType type )
{
    ProgressBar *newBar = new ProgressBar( 0 );
    newBar->setDescription( text );
    newBar->setMaximum( maximum );
    connect( sender, SIGNAL(destroyed( QObject * )), m_progressBar,
             SLOT(endProgressOperation( QObject * )) );
    connect( sender, SIGNAL(endProgressOperation( QObject * )), m_progressBar,
             SLOT(endProgressOperation( QObject * )) );
    connect( sender, SIGNAL(incrementProgress()), m_progressBar,
             SLOT(slotIncrementProgress()) );
    connect( sender, SIGNAL(totalSteps( int )), newBar, SLOT(slotTotalSteps( int )) );
    newBar->setAbortSlot( obj, slot, type );
    m_progressBar->addProgressBar( newBar, sender );
    m_progressBar->show();

    m_busy = true;
}

void
BrowserMessageArea::hideProgress() //SLOT
{
    m_progressBar->hide();
    m_busy = false;

    nextShortMessage();
}

void
BrowserMessageArea::nextShortMessage()
{
    if( m_shortMessageQueue.count() > 0 )
    {
        m_busy = true;
        m_messageLabel->setText( m_shortMessageQueue.takeFirst() );
        m_messageLabel->show();
        m_shortMessageTimer->start( SHORT_MESSAGE_DURATION );
    }
    else
    {
        m_messageLabel->hide();
        m_busy = false;
    }
}

void
BrowserMessageArea::hideLongMessage()
{
    sender()->deleteLater();
}

void
BrowserMessageArea::slotLongMessage( const QString &text, MessageType type )
{
    LongMessageWidget *message = new LongMessageWidget( this, text, type );
    connect( message, SIGNAL( closed() ), this, SLOT( hideLongMessage() ) );
}
