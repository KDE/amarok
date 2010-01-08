/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "Mp3tunesConfig.h"

#include <kdebug.h>
#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>

#include <QNetworkInterface>

Mp3tunesConfig::Mp3tunesConfig()
{
    m_hasChanged = false;
    load();
}


Mp3tunesConfig::~Mp3tunesConfig()
{
}

void Mp3tunesConfig::load()
{
    kDebug( 14310 ) << "load";
    KConfigGroup config = KGlobal::config()->group( "Service_Mp3tunes" );
    m_email = config.readEntry( "email", QString() );
    m_password = config.readEntry( "password", QString() );
    m_identifier = config.readEntry( "identifier", QString() );
    m_pin = config.readEntry( "pin", QString() );
    m_harmonyEmail = config.readEntry( "harmonyEmail", QString() );
    m_partnerToken = config.readEntry( "partnerToken", QString( "4895500420" ) );
    m_harmonyEnabled = config.readEntry( "harmonyEnabled", false );

    if( m_identifier.isEmpty() )
    {
        foreach( const QNetworkInterface &iface, QNetworkInterface::allInterfaces() )
        {
            QString addr = iface.hardwareAddress();
            if( addr != "00:00:00:00:00:00" ) {
                addr.remove( ':' );
                kDebug( 14310 ) << "Using iface \"" << iface.name() << " addr: " << addr;
                setIdentifier( addr + m_partnerToken );
                save();
                break;
            }
        }
    }
}

void Mp3tunesConfig::save()
{
    kDebug( 14310 ) << "save";
    if ( m_hasChanged ) {
        KConfigGroup config = KGlobal::config()->group( "Service_Mp3tunes" );
        config.writeEntry( "email", m_email );
        config.writeEntry( "password", m_password );
        config.writeEntry( "identifier", m_identifier );
        config.writeEntry( "harmonyEnabled", m_harmonyEnabled );
        config.writeEntry( "partnerToken", m_partnerToken );
        config.writeEntry( "harmonyEmail", m_harmonyEmail );
        config.writeEntry( "pin", m_pin );
    }
}

QString Mp3tunesConfig::email()
{
    return m_email;
}

QString Mp3tunesConfig::password()
{
    return m_password;
}

QString Mp3tunesConfig::partnerToken()
{
    return m_partnerToken;
}

QString Mp3tunesConfig::identifier()
{
    return m_identifier;
}

QString Mp3tunesConfig::pin()
{
    return m_pin;
}

QString Mp3tunesConfig::harmonyEmail()
{
    return m_harmonyEmail;
}

bool Mp3tunesConfig::harmonyEnabled()
{
   return m_harmonyEnabled;
}

void Mp3tunesConfig::setHarmonyEnabled( bool enabled )
{
    kDebug( 14310 ) << "set harmony";
    if ( enabled != m_harmonyEnabled ) {
        m_harmonyEnabled = enabled;
        m_hasChanged = true;
    }
}

void Mp3tunesConfig::setIdentifier( const QString &ident )
{
    kDebug( 14310 ) << "set hwaddress";
    if ( ident != m_identifier ) {
        m_identifier = ident;
        m_hasChanged = true;
    }
}

void Mp3tunesConfig::setEmail( const QString &email )
{
    kDebug( 14310 ) << "set email";
    if ( email != m_email ) {
        m_email = email;
        m_hasChanged = true;
    }
}

void Mp3tunesConfig::setPassword( const QString &password )
{
    kDebug( 14310 ) << "set Password";
    if( password != m_password ) {
        m_password = password;
        m_hasChanged = true;
    }
}

void Mp3tunesConfig::setPartnerToken( const QString &token )
{
    kDebug( 14310 ) << "set token";
    if( token != m_partnerToken ) {
        m_partnerToken = token;
        m_hasChanged = true;
    }
}

void Mp3tunesConfig::setPin( const QString &pin )
{
    kDebug( 14310 ) << "set pin";
    if( pin != m_pin ) {
        m_pin = pin;
        m_hasChanged = true;
    }
}

void Mp3tunesConfig::setHarmonyEmail( const QString &harmonyEmail )
{
    kDebug( 14310 ) << "set harmonyEmail";
    if( harmonyEmail != m_harmonyEmail ) {
        m_harmonyEmail = harmonyEmail;
        m_hasChanged = true;
    }
}
