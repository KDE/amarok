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

#include <KConfig>
#include <KConfigGroup>

using namespace Capabilities;

IpodTranscodeCapability::IpodTranscodeCapability( IpodCollection *coll, const QString &deviceDirPath  )
    : TranscodeCapability()
    , m_coll( coll )
    , m_configFilePath( deviceDirPath + QStringLiteral( "/AmarokTranscodingPrefs" ) )
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
        return m_coll->supportedFormats();
    return QStringList();
}

Transcoding::Configuration
IpodTranscodeCapability::savedConfiguration()
{
    KConfig config( m_configFilePath, KConfig::SimpleConfig );
    return Transcoding::Configuration::fromConfigGroup( config.group( QString() ) );
}

void
IpodTranscodeCapability::setSavedConfiguration( const Transcoding::Configuration &configuration )
{
    KConfig config( m_configFilePath, KConfig::SimpleConfig );
    KConfigGroup group = config.group( QString() );
    configuration.saveToConfigGroup( group );
    config.sync();
}

