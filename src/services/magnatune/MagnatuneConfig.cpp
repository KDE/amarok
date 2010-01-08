/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "MagnatuneConfig.h"
#include "MagnatuneMeta.h"

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

void
MagnatuneConfig::load()
{
    m_hasChanged = false;

    kDebug() << "load";
    KConfigGroup config = KGlobal::config()->group( "Service_Magnatune" );

    m_isMember = config.readEntry( "isMember", false );


    m_membershipType = config.readEntry( "membershipType", -1 );

    if( m_membershipType == -1 )
    {
        //try to read the old style string version if that is present and valid.
        QString oldMEmbershipType = config.readEntry( "membershipType", QString() );
        if( oldMEmbershipType.toLower() == "stream" )
            m_membershipType = MagnatuneConfig::STREAM;
        else if ( oldMEmbershipType.toLower() == "download" )
            m_membershipType = MagnatuneConfig::DOWNLOAD;
        else
            m_membershipType = MagnatuneConfig::DOWNLOAD;
            //default to download for now.   
    }

    m_username = config.readEntry( "username", QString() );
    m_password = config.readEntry( "password", QString() );
    m_email = config.readEntry( "email", QString() );


    qulonglong defaultLong = 0;
    m_lastUpdateTimestamp = config.readEntry( "lastUpdate", defaultLong );

    QString streamTypeString = config.readEntry( "streamType", QString() );
   

    //make ogg the default
    if ( streamTypeString == "mp3" )
        m_streamType = MagnatuneMetaFactory::MP3;
    else if (  streamTypeString == "lofi_mp3"  )
        m_streamType = MagnatuneMetaFactory::LOFI;
    else 
        m_streamType = MagnatuneMetaFactory::OGG;
    
}

void
MagnatuneConfig::save()
{
    kDebug() << "save";
    if ( m_hasChanged ) {
        KConfigGroup config = KGlobal::config()->group( "Service_Magnatune" );

        config.writeEntry( "isMember", m_isMember );
        config.writeEntry( "membershipType", m_membershipType );
        config.writeEntry( "username", m_username );
        config.writeEntry( "password", m_password );
        config.writeEntry( "lastUpdate", QVariant( m_lastUpdateTimestamp ) );
        config.writeEntry( "email", m_email );
        
        QString streamTypeString;
        if ( m_streamType == MagnatuneMetaFactory::MP3 )
            streamTypeString = "mp3";
        else if ( m_streamType == MagnatuneMetaFactory::LOFI )
            streamTypeString = "lofi_mp3";
        else
            streamTypeString = "ogg";

        config.writeEntry( "streamType", streamTypeString );

    }
}

bool
MagnatuneConfig::isMember()
{
    return m_isMember;
}

void
MagnatuneConfig::setIsMember( bool isMember )
{
    m_hasChanged = true;
    m_isMember = isMember;
}

int
MagnatuneConfig::membershipType()
{
    return m_membershipType;
}

void
MagnatuneConfig::setMembershipType( int membershipType )
{
    m_hasChanged = true;
    m_membershipType = membershipType;
}

QString
MagnatuneConfig::membershipPrefix()
{
    QString prefix;
    if( m_membershipType == MagnatuneConfig::STREAM )
        prefix = "stream";
    else
        prefix = "download";

    return prefix;
}

QString
MagnatuneConfig::username()
{
    return m_username;
}

QString
MagnatuneConfig::password()
{
    return m_password;
}

void
MagnatuneConfig::setUsername( const QString &username )
{
    m_hasChanged = true;
    m_username = username;
}

void
MagnatuneConfig::setPassword( const QString &password )
{
    m_hasChanged = true;
    m_password = password;
}


int
MagnatuneConfig::streamType() const
{
    return m_streamType;
}


void
MagnatuneConfig::setStreamType( int theValue )
{
    m_streamType = theValue;
}


qulonglong
MagnatuneConfig::lastUpdateTimestamp()
{
    return m_lastUpdateTimestamp;
}

void
MagnatuneConfig::setLastUpdateTimestamp( qulonglong timestamp )
{
    m_hasChanged = true;
    m_lastUpdateTimestamp = timestamp;
}

QString
MagnatuneConfig::email()
{
    return m_email;
}

void
MagnatuneConfig::setEmail( const QString &email )
{
    m_email = email;
    m_hasChanged = true;
}

