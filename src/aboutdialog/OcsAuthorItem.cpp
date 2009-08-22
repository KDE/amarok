/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "OcsAuthorItem.h"

#include "Debug.h"

#include <KAction>
#include <KRun>
#include <KStandardDirs>

#include <QHBoxLayout>
#include <QVBoxLayout>

OcsAuthorItem::OcsAuthorItem( const KAboutPerson &person, const Attica::Person &ocsPerson, QWidget *parent )
    : QWidget( parent )
{
    m_person = &person;
    m_ocsPerson = &ocsPerson;

    setupUi( this );
    init();

    m_avatar->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    m_avatar->setFixedSize( 56, 56 );
    m_avatar->setFrameStyle( QFrame::StyledPanel );
    m_avatar->setPixmap( m_ocsPerson->avatar() );
    m_avatar->setAlignment( Qt::AlignCenter );

    m_aboutText.append( "<br/>" + ( m_ocsPerson->city().isEmpty() ? "" : ( m_ocsPerson->city() + ", " ) ) + m_ocsPerson->country() );
    if( !m_ocsPerson->extendedAttribute( "ircchannels" ).isEmpty() )
    {
        QString channelsString = m_ocsPerson->extendedAttribute( "ircchannels" );
        //We extract the channel names from the string provided by OCS:
        QRegExp channelrx = QRegExp( "#+[\\w\\.\\-\\/!()+]+([\\w\\-\\/!()+]?)", Qt::CaseInsensitive );
        QStringList channels;
        int pos = 0;
        while( ( pos = channelrx.indexIn( channelsString, pos ) ) != -1 )
        {
            channels << channelrx.cap( 0 );
            pos += channelrx.matchedLength();
        }

        m_aboutText.append( "<br/>" + i18n("IRC channels: ") );
        QString link;
        foreach( QString channel, channels )
        {
            QString channelName = channel;
            channelName.remove( "#" );
            link = QString( "irc://irc.freenode.org/%1" ).arg( channelName );
            m_aboutText.append( QString( "<a href=\"%1\">%2</a>" ).arg( link, channel ) + "  " );
        }
    }
    if( !m_ocsPerson->extendedAttribute( "favouritemusic" ).isEmpty() )
    {
        QStringList artists = m_ocsPerson->extendedAttribute( "favouritemusic" ).split( ", " );
        //TODO: make them clickable
        m_aboutText.append( "<br/>" + i18n( "Favorite music: " ) + artists.join( ", " ) );

    }


    KAction *visitProfile = new KAction( KIcon( QPixmap( KStandardDirs::locate( "data",
            "amarok/images/opendesktop.png" ) ) ), i18n( "Visit openDesktop.org profile" ), this );
    visitProfile->setToolTip( i18n( "Visit the contributor's profile on openDesktop.org" ) );
    visitProfile->setData( m_ocsPerson->extendedAttribute( "profilepage" ) );
    m_iconsBar->addAction( visitProfile );

    m_textLabel->setText( m_aboutText );
    //TODO: Add favorite artists!

}

OcsAuthorItem::OcsAuthorItem( const KAboutPerson &person, QWidget *parent )
    : QWidget( parent )
{
    m_person = &person;

    setupUi( this );
    init();

    m_textLabel->setText( m_aboutText );
}

void
OcsAuthorItem::init()
{
    m_textLabel->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_textLabel->setOpenExternalLinks( true );
    m_textLabel->setContentsMargins( 5, 0, 0, 2 );
    m_verticalLayout->setSpacing( 0 );


    m_aboutText.append( "<b>" + m_person->name() + "</b>" );
    m_aboutText.append( "<br/>" + m_person->task() );

    m_iconsBar = new KToolBar( this, false, false );
    m_verticalLayout->insertWidget( m_verticalLayout->count() - 1, m_iconsBar );
    m_iconsBar->setIconSize( QSize( 22, 22 ) );
    m_iconsBar->setContentsMargins( 0, 0, 0, 0 );

    KAction *email = new KAction( KIcon( "internet-mail" ), i18n("Email contributor"), this );
    email->setToolTip( m_person->emailAddress() );
    email->setData( QString( "mailto:" + m_person->emailAddress() ) );
    m_iconsBar->addAction( email );


    if( !m_person->webAddress().isEmpty() )
    {
        KAction *homepage = new KAction( KIcon( "applications-internet" ), i18n("Visit contributor's homepage"), this );
        homepage->setToolTip( m_person->webAddress() );
        homepage->setData( m_person->webAddress() );
        m_iconsBar->addAction( homepage );
    }

    connect( m_iconsBar, SIGNAL( actionTriggered( QAction * ) ), this, SLOT( launchUrl( QAction * ) ) );
}

OcsAuthorItem::~OcsAuthorItem()
{}

QString
OcsAuthorItem::name()
{
    return m_person->name();
}

void
OcsAuthorItem::launchUrl( QAction *action )
{
    KUrl url = KUrl( action->data().toString() );
    KRun::runUrl( url, "text/html", 0, false );
}
