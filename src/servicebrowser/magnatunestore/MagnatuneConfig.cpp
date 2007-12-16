/***************************************************************************
 * Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU General Public License as          *
 * published by the Free Software Foundation; either version 2 of          *
 * the License or (at your option) version 3 or any later version          *
 * accepted by the membership of KDE e.V. (or its successor approved       *
 * by the membership of KDE e.V.), which shall act as a proxy              *
 * defined in Section 14 of version 3 of the license.                      *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 **************************************************************************/
 
#include "MagnatuneConfig.h"

#include <kdebug.h>
#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>

MagnatuneConfig::MagnatuneConfig()
{
    load();
}


MagnatuneConfig::~MagnatuneConfig()
{
}

void MagnatuneConfig::load()
{
    m_hasChanged = false;

    kDebug( 14310 ) << "load";
    KConfigGroup config = KGlobal::config()->group( "Service_Magnatune" );

    m_isMember = config.readEntry( "isMember", false );
    m_membershipType = config.readEntry( "membershipType", QString() );
    m_username = config.readEntry( "username", QString() );
    m_password = config.readEntry( "password", QString() );
    
}

void MagnatuneConfig::save()
{
    kDebug( 14310 ) << "save";
    if ( m_hasChanged ) {
        KConfigGroup config = KGlobal::config()->group( "Service_Magnatune" );

        config.writeEntry( "isMember", m_isMember );
        config.writeEntry( "membershipType", m_membershipType );
        config.writeEntry( "username", m_username );
        config.writeEntry( "password", m_password );
    }
}

bool MagnatuneConfig::isMember()
{
    return m_isMember;
}

void MagnatuneConfig::setIsMember( bool isMember )
{
    m_hasChanged = true;
    m_isMember = isMember;
}

QString MagnatuneConfig::membershipType()
{
    return m_membershipType;
}

void MagnatuneConfig::setMembershipType( const QString &membershipType )
{
    m_hasChanged = true;
    m_membershipType = membershipType;
}

QString MagnatuneConfig::username()
{
    return m_username;
}

QString MagnatuneConfig::password()
{
    return m_password;
}

void MagnatuneConfig::setUsername( const QString &username )
{
    m_hasChanged = true;
    m_username = username;
}

void MagnatuneConfig::setPassword( const QString &password )
{
    m_hasChanged = true;
    m_password = password;
}


