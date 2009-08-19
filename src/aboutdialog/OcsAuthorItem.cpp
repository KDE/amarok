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

#include <QHBoxLayout>
#include <QVBoxLayout>

OcsAuthorItem::OcsAuthorItem( const KAboutPerson *person, const Attica::Person *ocsPerson, QWidget *parent )
    : QWidget( parent )
    , m_person( person )
    , m_ocsPerson( ocsPerson )
{
    init();

}

OcsAuthorItem::OcsAuthorItem( const KAboutPerson *person, QWidget *parent )
    : QWidget( parent )
    , m_person( person )
{
    init();
}

void
OcsAuthorItem::init()
{


    m_name = new QLabel( this );
    m_role = new QLabel( this );
    m_email = new QLabel( this );
    m_homepage = new QLabel( this );
    m_avatar = new QLabel( this );
    m_location = new QLabel( this );
    m_ircChannels = new QLabel( this );
    m_profile = new QLabel( this );


}

OcsAuthorItem::~OcsAuthorItem()
{}
