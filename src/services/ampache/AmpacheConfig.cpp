/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "AmpacheConfig.h"

#include "core/support/Amarok.h"


AmpacheConfig::AmpacheConfig()
{
    load();
}

void
AmpacheConfig::load()
{
    KConfigGroup config = Amarok::config( QStringLiteral("Service_Ampache") );

    int serverIndex = 0;
    QString serverEntry = QStringLiteral("server") + QString::number( serverIndex );

    while ( config.hasKey( serverEntry ) )
    {
        QStringList list = config.readEntry(serverEntry, QStringList() );
        if ( list.isEmpty() )
            continue;

        AmpacheServerEntry entry;
        entry.name = list.takeFirst();
        entry.url = QUrl( list.takeFirst() );
        entry.username = list.takeFirst();
        entry.password = list.takeFirst();
        entry.addToCollection = false; //FIXME

        m_servers.append( entry );

        serverIndex++;
        serverEntry = QStringLiteral("server") + QString::number( serverIndex );
    }
}

void
AmpacheConfig::save()
{
    //delete all entries to make sure the indexes are correct
    KConfigGroup config = Amarok::config( QStringLiteral("Service_Ampache") );

    int serverIndex = 0;
    QString serverEntry = QStringLiteral("server") + QString::number( serverIndex );

    while ( config.hasKey ( serverEntry ) )
    {
//         kDebug( 14310 ) << "deleting " << serverEntry;
        config.deleteEntry( serverEntry );
        serverIndex++;
        serverEntry = QStringLiteral("server") + QString::number( serverIndex );
    }

    for( int i = 0; i < m_servers.size(); i++ )
    {
        AmpacheServerEntry entry = m_servers.at( i );
        QStringList list;

        list << entry.name;
        list << entry.url.url();
        list << entry.username;
        list << entry.password;

        serverEntry = QStringLiteral("server") + QString::number( i );
//         kDebug( 14310 ) << "adding " << serverEntry;
        config.writeEntry( serverEntry, list );
    }
}

int
AmpacheConfig::serverCount() const
{
    return m_servers.size();
}

AmpacheServerList
AmpacheConfig::servers() const
{
    return m_servers;
}

void
AmpacheConfig::addServer( const AmpacheServerEntry &server )
{
    m_servers.append( server );
}

void
AmpacheConfig::removeServer( int index )
{
    m_servers.removeAt( index );
}

void
AmpacheConfig::updateServer( int index, const AmpacheServerEntry & server )
{
    m_servers.removeAt( index );
    m_servers.insert( index, server );
}

