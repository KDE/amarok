/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "statusbar/ProgressBar.h"

#include "core/support/Debug.h"
#include "MainWindow.h"

#include <QTimer>

#include <KIcon>
#include <KLocale>

ProgressBar::ProgressBar( QWidget *parent )
        : QFrame( parent )
{

    QVBoxLayout *box = new QVBoxLayout( this );
    box->setMargin( 0 );
    box->setSpacing( 5 );

    QFrame *descriptionFrame = new QFrame( this );
    QHBoxLayout *descriptionLayout = new QHBoxLayout( this );
    descriptionLayout->setMargin( 0 );
    descriptionLayout->setSpacing( 2 );

    m_extraButtonSpace = new KHBox( descriptionFrame );
    m_extraButtonSpace->setSpacing( 0 );
    m_extraButtonSpace->setMargin( 0 );
    descriptionLayout->addWidget( m_extraButtonSpace );
    descriptionLayout->setAlignment( m_extraButtonSpace, Qt::AlignLeft );

    m_descriptionLabel = new QLabel( this );
    m_descriptionLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    descriptionLayout->addWidget( m_descriptionLabel );

    m_cancelButton = new QToolButton( descriptionFrame );
    m_cancelButton->setIcon( KIcon( "dialog-cancel-amarok" ) );
    m_cancelButton->setToolTip( i18n( "Abort" ) );
    m_cancelButton->setEnabled( false );
    descriptionLayout->addWidget( m_cancelButton );
    descriptionLayout->setAlignment( m_cancelButton, Qt::AlignRight );

    descriptionFrame->setLayout( descriptionLayout );

    box->addWidget( descriptionFrame );
    box->setAlignment( descriptionFrame, Qt::AlignTop );

    m_progressBar = new QProgressBar( this );
    m_progressBar->setMinimum( 0 );
    m_progressBar->setMaximum( 100 );
    m_progressBar->setFixedHeight( 5 );
    m_progressBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_progressBar->setTextVisible( false );
    box->addWidget( m_progressBar, 0, Qt::AlignBottom );

    setLayout( box );
}


ProgressBar::~ProgressBar()
{
}

void
ProgressBar::setDescription( const QString & description )
{
    m_descriptionLabel->setText( description );

}

ProgressBar *
ProgressBar::setAbortSlot( QObject * receiver, const char * slot, Qt::ConnectionType type )
{
    // DEBUG_BLOCK
    // debug() << "Setting abort slot for " << m_descriptionLabel->text();
    cancelButton()->setEnabled( true );
    // debug() << "connecting to " << slot;
    connect( this, SIGNAL( cancelled() ), receiver, slot, type );
    connect( cancelButton(), SIGNAL( clicked() ), this, SLOT( cancel() ) );


    return this;
}

void ProgressBar::cancel()
{
    DEBUG_BLOCK
    debug() << "cancelling operation: " << m_descriptionLabel->text();
    emit( cancelled() );
    emit( cancelled( this ) );
}

void ProgressBar::setValue( int percentage )
{
    progressBar()->setValue( percentage );
    emit( percentageChanged( percentage ) );

    //this safety check has to be removed as KJobs sometimes start out
    //by showing 100%, thus removing the progress info before it even gets started
    /*if ( percentage == m_progressBar->maximum() )
        QTimer::singleShot( POST_COMPLETION_DELAY, this, SLOT( delayedDone() ) );*/
}

void ProgressBar::delayedDone()
{
    emit( complete( this ) );
}

int ProgressBar::percentage()
{
    if ( m_progressBar->maximum() == 100 )
        return m_progressBar->value();
    return ( int )((( float ) m_progressBar->value() / ( float ) m_progressBar->maximum() ) * 100.0 );
}


#include "ProgressBar.moc"
