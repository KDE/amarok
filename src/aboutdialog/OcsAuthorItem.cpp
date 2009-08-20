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

    avatar->setPixmap( m_ocsPerson->avatar() );
    location->setText( m_ocsPerson->city() + ", " + m_ocsPerson->country() );
    ircChannels->setText( m_ocsPerson->extendedAttribute( "ircchannels" ) );
    profile->setTextInteractionFlags( Qt::TextBrowserInteraction );
    profile->setOpenExternalLinks( true );
    profile->setText( QString( i18n( "<a href=\"%1\">Visit profile</a>", m_ocsPerson->extendedAttribute( "profilepage" ) ) ) );
}

OcsAuthorItem::OcsAuthorItem( const KAboutPerson &person, QWidget *parent )
    : QWidget( parent )
{
    m_person = &person;

    setupUi( this );
    init();

    location->hide();
    ircChannels->hide();
    profile->hide();
}

void
OcsAuthorItem::init()
{
    name->setText( "<b>" + m_person->name() + "</b>" );
    task->setText( m_person->task() );
    email->setText( m_person->emailAddress() );
    if( m_person->webAddress().isEmpty() )
        homepage->hide();
    else
        homepage->setText( m_person->webAddress() );
}

OcsAuthorItem::~OcsAuthorItem()
{}
