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


    m_avatar->resize( 56, 56 );
    m_avatar->setFrameStyle( QFrame::StyledPanel );
    m_avatar->setPixmap( m_ocsPerson->avatar() );

    m_aboutText.append( "<br/>" + ( m_ocsPerson->city().isEmpty() ? "" : ( m_ocsPerson->city() + ", " ) ) + m_ocsPerson->country() );
    if( !m_ocsPerson->extendedAttribute( "ircchannels" ).isEmpty() )
    {
        QString channelsString = m_ocsPerson->extendedAttribute( "ircchannels" );
        //FIXME: Learn regexps, extract channel names and feed them into links.
        //QRegExp channelRegExp = QRegExp( "(#\\S*)([,\\s]+$)", Qt::CaseInsensitive );
        //channelRegExp.indexIn( channelsString );
        //QStringList channels = channelRegExp.capturedTexts();
        //debug()<< "Irc channels are" << channels;
        //  irc://irc.freenode.org/
        m_aboutText.append( "<br/>" + m_ocsPerson->extendedAttribute( "ircchannels" ) );
    }
    m_aboutText.append( QString( "<br/>" + i18n( "<a href=\"%1\">Visit profile</a>", m_ocsPerson->extendedAttribute( "profilepage" ) ) ) );
    m_textLabel->setText( m_aboutText );
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

    m_aboutText.append( "<b>" + m_person->name() + "</b>" );
    m_aboutText.append( "<br/>" + m_person->task() );
    m_aboutText.append( QString( "<br/><a href=\"mailto:%1\">%1</a>" ).arg( m_person->emailAddress() ) );
    if( !m_person->webAddress().isEmpty() )
        m_aboutText.append( QString( "<br/><a href=\"%3\">%3</a>" ).arg( m_person->webAddress() ) );
}

OcsAuthorItem::~OcsAuthorItem()
{}

QString
OcsAuthorItem::name()
{
    return m_person->name();
}
