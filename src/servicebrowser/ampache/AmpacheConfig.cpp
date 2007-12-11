/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "AmpacheConfig.h"

#include <KConfig>
#include <KConfigGroup>
#include <kdebug.h>
#include <KGlobal>

AmpacheConfig::AmpacheConfig()
{
    load();
}

void AmpacheConfig::load()
{
    KConfigGroup config = KGlobal::config()->group( "Service_Ampache" );


    int serverIndex = 0;
    QString serverEntry = "server" + QString::number( serverIndex );




    while ( config.hasKey ( serverEntry ) ) {

        QStringList list = config.readEntry(serverEntry, QStringList() );

        AmpacheServerEntry entry;
        QString name = list.takeFirst();
        entry.url = list.takeFirst();
        entry.username = list.takeFirst();
        entry.password = list.takeFirst();
        entry.addToCollection = false; //FIXME

        m_serverMap.insert( name, entry );

        serverIndex++;
        serverEntry = "server" + QString::number( serverIndex );

    }

}

void AmpacheConfig::save()
{

    KConfigGroup config = KGlobal::config()->group( "Service_Ampache" );
    
    kDebug( 14310 ) << "saving to config file " << KGlobal::config()->name() ;

    
    int serverIndex = 0;
    QString serverEntry = "server" + QString::number( serverIndex );
    
    foreach( QString name, m_serverMap.keys() ) {

        AmpacheServerEntry entry = m_serverMap.value( name );
        
        QStringList list;

        list << name;
        list << entry.url;
        list << entry.username;
        list << entry.password;

        config.writeEntry( serverEntry, list );

        serverIndex++;
        serverEntry = "server" + QString::number( serverIndex );

    }


}

int AmpacheConfig::serverCount()
{
    return m_serverMap.count();
}

AmpacheServerMap AmpacheConfig::servers()
{
    return m_serverMap;
}

void AmpacheConfig::addServer( const QString &name, const AmpacheServerEntry &server )
{
    m_serverMap.insert( name, server );
}

void AmpacheConfig::removeServer(const QString &name )
{
    m_serverMap.remove( name );
    KConfigGroup config = KGlobal::config()->group( "Service_Ampache" );

    //delete the correct entry...

    int serverIndex = 0;
    QString serverEntry = "server" + QString::number( serverIndex );

    while ( config.hasKey ( serverEntry ) ) {

        QStringList list = config.readEntry(serverEntry, QStringList() );

        AmpacheServerEntry entry;
        QString entryName = list.takeFirst();

        if ( entryName == name ) {
            config.deleteEntry( serverEntry );
            break;
        }

        serverIndex++;
        serverEntry = "server" + QString::number( serverIndex );
    }


}


