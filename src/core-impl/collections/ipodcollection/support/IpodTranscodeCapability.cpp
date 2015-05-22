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

#include "IpodTranscodeCapability.h"


using namespace Capabilities;

IpodTranscodeCapability::IpodTranscodeCapability( IpodCollection *coll, const QString &deviceDirPath  )
    : TranscodeCapability()
    , m_coll( coll )
    , m_configFilePath( deviceDirPath + QString( "/AmarokTranscodingPrefs" ) )
{
}

IpodTranscodeCapability::~IpodTranscodeCapability()
{
    // nothing to do
}

QStringList
IpodTranscodeCapability::playableFileTypes()
{
    if( m_coll )
        return m_coll.data()->supportedFormats();
    return QStringList();
}

Transcoding::Configuration
IpodTranscodeCapability::savedConfiguration()
{
    KConfig config( m_configFilePath, KConfig::SimpleConfig );
    return Transcoding::Configuration::fromConfigGroup( config.group( 0 ) );
}

void
IpodTranscodeCapability::setSavedConfiguration( const Transcoding::Configuration &configuration )
{
    KConfig config( m_configFilePath, KConfig::SimpleConfig );
    KConfigGroup group = config.group( 0 );
    configuration.saveToConfigGroup( group );
    config.sync();
}

