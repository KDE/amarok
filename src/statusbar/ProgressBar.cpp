/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "statusbar/ProgressBar.h"

#include "Debug.h"
#include "MainWindow.h"

#include <QTimer>

#include <KIcon>
#include <KLocale>

ProgressBarNG::ProgressBarNG( QWidget * parent )
        : QFrame( parent )
{
    QHBoxLayout *box = new QHBoxLayout( this );
    box->setMargin( 0 );
    box->setSpacing( 0 );

    m_descriptionLabel = new QLabel( this );
    m_descriptionLabel->setMinimumWidth( 50 );
    //m_descriptionLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    box->addWidget( m_descriptionLabel );

    KHBox *progressBox = new KHBox( this );

    m_extraButtonSpace = new KHBox( progressBox );
    m_extraButtonSpace->setSpacing( 0 );
    m_extraButtonSpace->setMargin( 0 );

    m_cancelButton = new QToolButton( progressBox );
    m_cancelButton->setIcon( KIcon( "dialog-cancel-amarok" ) );
    m_cancelButton->setToolTip( i18n( "Abort" ) );
    m_cancelButton->setEnabled( false );

    m_progressBar = new QProgressBar( progressBox );
    m_progressBar->setMinimum( 0 );
    m_progressBar->setMaximum( 100 );
    m_progressBar->setMinimumWidth( 200 );
    m_progressBar->setMaximumWidth( 300 );
    m_progressBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    box->addWidget( progressBox );
    box->setAlignment( progressBox, Qt::AlignRight );

    //setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed );

    setLayout( box );
}


ProgressBarNG::~ProgressBarNG()
{
}

void
ProgressBarNG::setDescription( const QString & description )
{
    DEBUG_BLOCK
    m_descriptionLabel->setText( description );

}

ProgressBarNG *
ProgressBarNG::setAbortSlot( QObject * receiver, const char * slot )
{
    DEBUG_BLOCK

    debug() << "Setting abort slot for " << m_descriptionLabel->text();

    cancelButton()->setEnabled( true );


    debug() << "connecting to " << slot;
    connect( this, SIGNAL( cancelled() ), receiver, slot );
    connect( cancelButton(), SIGNAL( clicked() ), this, SLOT( cancel() ) );


    return this;
}

void ProgressBarNG::cancel()
{
    DEBUG_BLOCK
    debug() << "cancelling operation: " << m_descriptionLabel->text();
    emit( cancelled() );
    emit( cancelled( this ) );
}

void ProgressBarNG::setValue( int percentage )
{
    progressBar()->setValue( percentage );
    emit( percentageChanged( percentage ) );

    //this safety check has to be removed as KJobs sometimes start out
    //by showing 100%, thus removing the progress info before it even gets started
    /*if ( percentage == m_progressBar->maximum() )
        QTimer::singleShot( POST_COMPLETION_DELAY, this, SLOT( delayedDone() ) );*/

}

void ProgressBarNG::delayedDone()
{
    emit( complete( this ) );
}

int ProgressBarNG::percentage()
{
    if ( m_progressBar->maximum() == 100 )
        return m_progressBar->value();
    else
    {

        return ( int )((( float ) m_progressBar->value() / ( float ) m_progressBar->maximum() ) * 100.0 );

    }
}


#include "ProgressBar.moc"
