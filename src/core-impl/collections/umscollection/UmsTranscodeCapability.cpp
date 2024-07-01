/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "UmsTranscodeCapability.h"

#include <KConfig>
#include <KConfigGroup>

UmsTranscodeCapability::UmsTranscodeCapability( const QString &configFilePath, const QString &groupName )
    : TranscodeCapability()
    , m_configFilePath( configFilePath )
    , m_groupName( groupName )
{
}

UmsTranscodeCapability::~UmsTranscodeCapability()
{
}

Transcoding::Configuration
UmsTranscodeCapability::savedConfiguration()
{
    KConfig configFile( m_configFilePath, KConfig::SimpleConfig );
    if( !configFile.hasGroup( m_groupName ) )
        return Transcoding::Configuration( Transcoding::INVALID );
    return Transcoding::Configuration::fromConfigGroup( configFile.group( m_groupName ) );
}

void
UmsTranscodeCapability::setSavedConfiguration( const Transcoding::Configuration &configuration )
{
    KConfig configFile( m_configFilePath, KConfig::SimpleConfig );
    KConfigGroup group = configFile.group( m_groupName );
    configuration.saveToConfigGroup( group );
    configFile.sync();
}

