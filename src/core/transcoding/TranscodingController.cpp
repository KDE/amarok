/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "TranscodingController.h"

#include "formats/TranscodingNullFormat.h"
#include "formats/TranscodingAacFormat.h"
#include "formats/TranscodingAlacFormat.h"
#include "formats/TranscodingFlacFormat.h"
#include "formats/TranscodingMp3Format.h"
#include "formats/TranscodingVorbisFormat.h"
#include "formats/TranscodingWmaFormat.h"

namespace Transcoding
{

Controller::Controller( QObject *parent )
    : QObject( parent )
    , m_formats( QList< Format * >()
            << new NullFormat()
            << new AacFormat()
            << new AlacFormat()
            << new FlacFormat()
            << new Mp3Format()
            << new VorbisFormat()
            << new WmaFormat() )
{
    KProcess *verifyAvailability = new KProcess( this );
    verifyAvailability->setOutputChannelMode( KProcess::MergedChannels );
    verifyAvailability->setProgram( "ffmpeg" );
    *verifyAvailability << QString( "-codecs" );
    connect( verifyAvailability, SIGNAL( finished( int, QProcess::ExitStatus ) ),
             this, SLOT( onAvailabilityVerified( int, QProcess::ExitStatus ) ) );
    verifyAvailability->start();
}

Controller::~Controller()
{
    qDeleteAll( m_formats );
}

void
Controller::onAvailabilityVerified( int exitCode, QProcess::ExitStatus exitStatus ) //SLOT
{
    Q_UNUSED( exitCode )
    Q_UNUSED( exitStatus )
    QString output = qobject_cast< KProcess * >( sender() )->readAllStandardOutput().data();
    if( output.simplified().isEmpty() )
        return;
    for( QList< Format * >::const_iterator it = m_formats.constBegin(); it != m_formats.constEnd(); ++it)
    {
        if( (*it)->verifyAvailability( output ) )
            m_availableFormats.append( *it );
    }
    sender()->deleteLater();
}

const Format *
Controller::format( Encoder encoder ) const
{
    return m_formats.at( static_cast< int >( encoder ) );
}

} //namespace Transcoding
