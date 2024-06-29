/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "OcsPersonItem.h"

#include "core/support/Debug.h"

#include <QAction>
#include <QStandardPaths>
#include <KIO/OpenUrlJob>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QStyleOption>

OcsPersonItem::OcsPersonItem( const KAboutPerson &person, const QString &ocsUsername, PersonStatus status, QWidget *parent )
    : QWidget( parent )
    , m_status( status )
{
    m_person = &person;
    m_ocsUsername = ocsUsername;

    setupUi( this );
    init();
}

void
OcsPersonItem::init()
{
    m_textLabel->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_textLabel->setOpenExternalLinks( true );
    m_textLabel->setContentsMargins( 5, 0, 0, 2 );
    m_verticalLayout->setSpacing( 0 );

    m_vertLine->hide();
    m_initialSpacer->changeSize( 0, 40, QSizePolicy::Fixed, QSizePolicy::Fixed );
    layout()->invalidate();

    m_aboutText.append( QStringLiteral("<b>") + m_person->name() + QStringLiteral("</b>") );
    if( !m_person->task().isEmpty() )
        m_aboutText.append( QStringLiteral("<br/>") + m_person->task() );

    m_iconsBar = new KToolBar( this, false, false );
    m_snBar = new KToolBar( this, false, false );

    m_iconsBar->setIconSize( QSize( 22, 22 ) );
    m_iconsBar->setContentsMargins( 0, 0, 0, 0 );
    m_iconsBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

    if( m_status == Author )
    {
        QHBoxLayout *iconsLayout = new QHBoxLayout( this );
        iconsLayout->setContentsMargins( 0, 0, 0, 0 );
        iconsLayout->setSpacing( 0 );
        m_verticalLayout->insertLayout( m_verticalLayout->count() - 1, iconsLayout );
        iconsLayout->addWidget( m_iconsBar );
        iconsLayout->addWidget( m_snBar );
        iconsLayout->addStretch( 0 );

        m_snBar->setIconSize( QSize( 16, 16 ) );
        m_snBar->setContentsMargins( 0, 0, 0, 0 );
        m_snBar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    }
    else
    {
        layout()->addWidget( m_iconsBar );
        m_snBar->hide();
    }

    if( !m_person->emailAddress().isEmpty() )
    {
        QAction *email = new QAction( QIcon::fromTheme( QStringLiteral("mail-send") ), i18n("Email contributor"), this );
        email->setToolTip( m_person->emailAddress() );
        email->setData( QString( QStringLiteral( "mailto:") + m_person->emailAddress() ) );
        m_iconsBar->addAction( email );
    }

    if( !m_person->webAddress().isEmpty() )
    {
        QAction *homepage = new QAction( QIcon::fromTheme( QStringLiteral("internet-services") ), i18n("Visit contributor's homepage"), this );
        homepage->setToolTip( m_person->webAddress() );
        homepage->setData( m_person->webAddress() );
        m_iconsBar->addAction( homepage );
    }

    connect( m_iconsBar, &KToolBar::actionTriggered, this, &OcsPersonItem::launchUrl );
    connect( m_snBar, &KToolBar::actionTriggered, this, &OcsPersonItem::launchUrl );
    m_textLabel->setText( m_aboutText );
}

OcsPersonItem::~OcsPersonItem()
{}

QString
OcsPersonItem::name() const
{
    return m_person->name();
}

void
OcsPersonItem::launchUrl( QAction *action ) //SLOT
{
    QUrl url = QUrl( action->data().toString() );
    KIO::OpenUrlJob *openUrlJob = new KIO::OpenUrlJob(url, QStringLiteral("text/html"), this );
    openUrlJob->setRunExecutables( true );
    openUrlJob->start();
}
