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

#include <QIcon>
#include <KLocalizedString>

ProgressBar::ProgressBar( QWidget *parent )
        : QFrame( parent )
{
    setFixedHeight( 30 );
    setContentsMargins( 0, 0, 0, 4 );

    QVBoxLayout *box = new QVBoxLayout;
    box->setContentsMargins( 0, 0, 0, 0 );
    box->setSpacing( 3 );

    QHBoxLayout *descriptionLayout = new QHBoxLayout;
    descriptionLayout->setContentsMargins( 0, 0, 0, 0 );
    descriptionLayout->setSpacing( 2 );

    m_descriptionLabel = new QLabel;
    m_descriptionLabel->setWordWrap( true );
    //add with stretchfactor 1 so it takes up more space then the cancel button
    descriptionLayout->addWidget( m_descriptionLabel, 1 );

    m_cancelButton = new QToolButton;
    m_cancelButton->setIcon( QIcon::fromTheme( QStringLiteral("dialog-cancel-amarok") ) );
    m_cancelButton->setToolTip( i18n( "Abort" ) );
    m_cancelButton->setHidden( true );
    m_cancelButton->setFixedWidth( 16 );
    m_cancelButton->setFixedHeight( 16 );
    m_cancelButton->setAutoFillBackground( false );
    m_cancelButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    descriptionLayout->addWidget( m_cancelButton );
    descriptionLayout->setAlignment( m_cancelButton, Qt::AlignRight );

    box->addLayout( descriptionLayout );

    m_progressBar = new QProgressBar;
    m_progressBar->setMinimum( 0 );
    m_progressBar->setMaximum( 100 );
    m_progressBar->setFixedHeight( 5 );
    m_progressBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_progressBar->setTextVisible( false );
    box->addWidget( m_progressBar );
    box->setAlignment( m_progressBar, Qt::AlignBottom );

    setLayout( box );
}


ProgressBar::~ProgressBar()
{
}

void
ProgressBar::setDescription( const QString &description )
{
    m_descriptionLabel->setText( description );
}

void ProgressBar::cancel()
{
    DEBUG_BLOCK
    debug() << "cancelling operation: " << m_descriptionLabel->text();
    Q_EMIT( cancelled( this ) );
}

void ProgressBar::setValue( int percentage )
{
    progressBar()->setValue( percentage );
    Q_EMIT( percentageChanged( percentage ) );

    //this safety check has to be removed as KJobs sometimes start out
    //by showing 100%, thus removing the progress info before it even gets started
    /*if ( percentage == m_progressBar->maximum() )
        QTimer::singleShot( POST_COMPLETION_DELAY, this, SLOT(delayedDone()) );*/
}

void ProgressBar::delayedDone()
{
    Q_EMIT( complete( this ) );
}

int ProgressBar::percentage()
{
    if( m_progressBar->maximum() == 100 )
        return m_progressBar->value();
    return (int)( ( (float) m_progressBar->value() / (float)m_progressBar->maximum() ) * 100.0 );
}


