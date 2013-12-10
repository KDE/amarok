/****************************************************************************************
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#include "Fingerprintcalculator.h"

#include "support/Debug.h"

AudioDataFeeder::AudioDataFeeder( const ChromarpintContext *ctx )
    : TagGuessing::DecodedAudioData()
    , m_chromarpintContext( ctx )
{
}

AudioDataFeeder::~AudioDataFeeder()
{
}

int
AudioDataFeeder::setupInitial( const int sampleRate, const quint8 channels )
{
    TagGuessing::DecodedAudioData::setupInitial( sampleRate, channels );
    if( chromaprint_start( m_chromarpintContext, sampleRate, ( int )channels ) )
        return 0;
    else
    {
        return 1;
        warning() << "Some error occurred in \"chromaprint_start\" " ;
    }
}

int
AudioDataFeeder::appendData( const quint8 *data, const int length )
{
    if( chromaprint_feed( m_chromarpintContext, data, length ) )
        return 0;
    else
    {
        return 1;
        warning() << "Some error occurred in \"chromaprint_feed\" " ;
    }
}

FingerprintCalculator::FingerprintCalculator( const Meta::TrackList &tracks, const int sampleLength )
    : TagGuessing::MusicDNSAudioDecoder( tracks, sampleLength )
{
}

FingerprintCalculator::~FingerprintCalculator()
{
}

FingerprintCalculator::run()
{
    const ChromaprintContext * const chromaprintContext = chromaprint_new( CHROMAPRINT_ALGORITHM_DEFAULT );
    AudioDataFeeder data( chromaprintContext );

    avcodec_register_all();
    av_register_all();

    Debug::Block fingerprintCreation( "creating fingerrpint of " + m_tracks->size() + " tracks" );
    foreach( const Meta::TrackPtr track, m_tracks )
    {
        Debug::Block singleTrack( "current track: " + track->prettyName() );
        decode( track->playableUrl().toLocalFile(), &data, m_sampleLength );
        if( chromaprint_finish( chromaprintContext ) )
        {
            const char **fingerprintString;
            if( chromaprint_get_fingerprint( chromaprintContext, fingerprintString ) )
                emit trackDecoded( track, QString( *fingerprintString ) );
            else
                warning() << "Some error occurred in \"chromaprint_get_fingerprint\" " ;
        }
        else
            warning() << QLatin1String( "Some error occurred in \"chromaprint_finish\" in file " ) +
                         track->playableUrl().toLocalFile();
        // TODO remember to free the memory of fingerprintString by calling chromaprint_dealloc
        data.flush();
    }

    chromaprint_free( chromaprintContext );
}
