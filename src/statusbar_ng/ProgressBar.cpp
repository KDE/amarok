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

#include "statusbar_ng/ProgressBar.h"

#include "Debug.h"
#include "MainWindow.h"

#include <QTimer>

#include <KIcon>
#include <KLocale>

ProgressBarNG::ProgressBarNG( QWidget * parent )
        : KHBox( parent )
{

    //setup the basics

    setSpacing( 4 );
    setContentsMargins( 0, 2, 0, 2 );

    m_extraButtonSpace = new KHBox( this );
    m_extraButtonSpace->setSpacing( 0 );
    m_extraButtonSpace->setContentsMargins( 0, 0, 0, 0 );

    m_cancelButton = new QToolButton( this );
    m_cancelButton->setIcon( KIcon( "dialog-cancel-amarok" ) );
    m_cancelButton->setToolTip( i18n( "Abort" ) );

    m_cancelButton->setEnabled( false );

    m_progresBar = new QProgressBar( this );
    m_progresBar->setMinimum( 0 );
    m_progresBar->setMaximum( 100 );
    m_progresBar->setMaximumWidth( 300 );
    m_progresBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    m_descriptionLabel = new QLabel( this );
    m_descriptionLabel->setMinimumWidth( 300 );
    m_descriptionLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    const int contentHeight = QFontMetrics( m_descriptionLabel->font() ).height();
    const int barHeight = contentHeight + 6;

    setFixedHeight( barHeight );

    m_progresBar->setFixedHeight( barHeight - 4 );

    setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed );

    /*setBackgroundRole( QPalette::Link );
    setAutoFillBackground ( true );
    */

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
    progresBar()->setValue( percentage );
    emit( percentageChanged( percentage ) );

    if ( percentage == m_progresBar->maximum() )
        QTimer::singleShot( POST_COMPLETION_DELAY, this, SLOT( delayedDone() ) );

}

void ProgressBarNG::delayedDone()
{
    emit( complete( this ) );
}

int ProgressBarNG::percentage()
{
    if ( m_progresBar->maximum() == 100 )
        return m_progresBar->value();
    else
    {

        return ( int )((( float ) m_progresBar->value() / ( float ) m_progresBar->maximum() ) * 100.0 );

    }
}


#include "ProgressBar.moc"
