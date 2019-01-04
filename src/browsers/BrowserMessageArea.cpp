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

#include "statusbar/LongMessageWidget.h"

#define SHORT_MESSAGE_DURATION 5000
#define POPUP_MESSAGE_DURATION 5000

BrowserMessageArea::BrowserMessageArea( QWidget *parent )
    : BoxWidget( true, parent )
    , m_busy( false )
{
    setObjectName( "BrowserMessageArea" );

    m_progressBar = new CompoundProgressBar( this );
    connect( m_progressBar, &CompoundProgressBar::allDone, this, &BrowserMessageArea::hideProgress );
    layout()->addWidget( m_progressBar );

    m_progressBar->hide();

    m_messageLabel = new QLabel( this );
    m_messageLabel->setAlignment( Qt::AlignCenter );
    m_messageLabel->setWordWrap( true );
    m_messageLabel->hide();

    m_shortMessageTimer = new QTimer( this );
    m_shortMessageTimer->setSingleShot( true );
    connect( m_shortMessageTimer, &QTimer::timeout, this, &BrowserMessageArea::nextShortMessage );

    //register to carry MessageType across threads
    qRegisterMetaType<Amarok::Logger::MessageType>( "MessageType" );
    connect( this, &BrowserMessageArea::signalLongMessage,
             this, &BrowserMessageArea::slotLongMessage,
             Qt::QueuedConnection );
}

void
BrowserMessageArea::shortMessageImpl( const QString &text )
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
BrowserMessageArea::longMessageImpl( const QString &text, MessageType type )
{
    // The purpose of this emit is to make the operation thread safe. If this
    // method is called from a non-GUI thread, the "emit" relays it over the
    // event loop to the GUI thread, so that we can safely create widgets.
    emit signalLongMessage( text, type );
}

void
BrowserMessageArea::newProgressOperationImpl( KJob *job, const QString &text, QObject *context,
                                              const std::function<void ()> &function, Qt::ConnectionType type )
{
    KJobProgressBar *newBar = new KJobProgressBar( nullptr, job );
    newBar->setDescription( text );
    connect( job, &KJob::destroyed, m_progressBar,
             &CompoundProgressBar::endProgressOperation );
    newBar->setAbortSlot( context, function, type );
    m_progressBar->addProgressBar( newBar, job );
    m_progressBar->show();

    m_busy = true;
}

void
BrowserMessageArea::newProgressOperationImpl( QNetworkReply *reply, const QString &text, QObject *obj,
                                              const std::function<void ()> &function, Qt::ConnectionType type )
{
    NetworkProgressBar *newBar = new NetworkProgressBar( nullptr, reply );
    newBar->setDescription( text );
    newBar->setAbortSlot( reply, &QNetworkReply::deleteLater );
    connect( reply, &QNetworkReply::destroyed, m_progressBar,
             &CompoundProgressBar::endProgressOperation );
    newBar->setAbortSlot( obj, function, type );
    m_progressBar->addProgressBar( newBar, reply );
    m_progressBar->show();

    m_busy = true;
}

void
BrowserMessageArea::newProgressOperationImpl( QObject *sender, const QMetaMethod &increment, const QMetaMethod &end, const QString &text,
                                              int maximum, QObject *obj, const std::function<void ()> &function, Qt::ConnectionType type )
{
    ProgressBar *newBar = new ProgressBar( nullptr );
    newBar->setDescription( text );
    newBar->setMaximum( maximum );
    connect( sender, &QObject::destroyed, m_progressBar,
             &CompoundProgressBar::endProgressOperation, Qt::QueuedConnection );
    int endIndex = m_progressBar->metaObject()->indexOfSlot( "endProgressOperation(QObject*)" );
    auto endSlot = m_progressBar->metaObject()->method( endIndex );
    connect( sender, end, m_progressBar, endSlot, Qt::QueuedConnection );
    int incrementIndex = m_progressBar->metaObject()->indexOfSlot( "slotIncrementProgress()" );
    auto incrementSlot = m_progressBar->metaObject()->method( incrementIndex );
    connect( sender, increment, m_progressBar, incrementSlot, Qt::QueuedConnection );
    if( sender->metaObject()->indexOfSignal( "totalSteps(int)" ) != -1 )
        connect( sender, SIGNAL(totalSteps(int)), newBar, SLOT(slotTotalSteps(int)) );
    newBar->setAbortSlot( obj, function, type );
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
BrowserMessageArea::slotLongMessage( const QString &text, MessageType type )
{
    Q_UNUSED(type)

    LongMessageWidget *message = new LongMessageWidget( text );
    connect( message, &LongMessageWidget::closed, message, &QObject::deleteLater );
}
