/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "FingerprintGenerator.h"

#include "Sha256File.h"
#include "Sha256.h"
#include "MP3_Source_Qt.h"

#include <QDateTime>
#include <QDebug>


using namespace fingerprint;

static const uint k_bufferSize = 1024 * 8;
static const int k_minTrackDuration = 30;

FingerprintGenerator::FingerprintGenerator( const QFileInfo& f, Mode m, QObject* parent ) :
    QThread( parent ),
	m_file( f ),
	m_mode( m )
{
    Q_ASSERT( m < Max );
    start();
}


void 
FingerprintGenerator::run()
{
    if( !m_file.isReadable() )
    {
        emit failed( ReadError );
        return;
    }
    fingerprint(m_file.filePath());
}


QString 
FingerprintGenerator::sha256()
{
    unsigned char hash[SHA256_HASH_SIZE];
    QString sha;
    Sha256File::getHash( m_file.filePath().toStdString(), hash );
    
    for (int i = 0; i < SHA256_HASH_SIZE; ++i) {
        QString hex = QString("%1").arg(uchar(hash[i]), 2, 16,
                                        QChar('0'));
        sha.append(hex);
    }
    
    return sha;
}


void 
FingerprintGenerator::fingerprint( QString filename )
{
    //int time = QDateTime::currentDateTime().toTime_t();
    int duration, samplerate, bitrate, nchannels;
    MP3_Source ms;

    try
    {
        MP3_Source::getInfo( filename, duration, samplerate, bitrate, nchannels );

        m_sampleRate = samplerate;
        m_numChannels = nchannels;

        ms.init( filename );
    }
    catch ( std::exception& e )
    {
        emit( failed( GetInfoError) );
        return;
    }
    
    if( duration < k_minTrackDuration )
    {
        emit( failed( TrackTooShortError ));
        return;
    }
    
    ms.skipSilence();

    bool fpDone = false;
    try
    {
        if ( mode() == Full )
        {
            m_extractor.initForFullSubmit( m_sampleRate, m_numChannels );
        }
        else
        {
            m_extractor.initForQuery( m_sampleRate, m_numChannels, duration );

            // Skippety skip for as long as the skipper sez (optimisation)
            ms.skip( m_extractor.getToSkipMs() );
            float secsToSkip = m_extractor.getToSkipMs() / 1000.0f;
            fpDone = m_extractor.process(
                0,
                static_cast<size_t>( m_sampleRate * m_numChannels * secsToSkip ),
                false );
        }
    }
    catch ( const std::exception& e )
    {
        emit( failed( ExtractorInitError));
        return;
    }
    
    const size_t PCMBufSize = 131072; 
    short* pPCMBuffer = new short[PCMBufSize];

    while ( !fpDone )
    {
        size_t readData = ms.updateBuffer( pPCMBuffer, PCMBufSize );
        if ( readData == 0 )
            break;

        try
        {
            fpDone = m_extractor.process( pPCMBuffer, readData, ms.eof() );
        }
        catch ( const std::exception& e )
        {
            emit( failed( ExtractorProcessError) );
            delete[] pPCMBuffer;
            return;
        }

    } // end while

    delete[] pPCMBuffer;

    if ( !fpDone )
    {
        emit( failed( ExtractorNotEnoughDataError));
        m_fingerprint.clear();
        return;
    }
    
    // We succeeded
    std::pair<const char*, size_t> fpData = m_extractor.getFingerprint();

    if( fpData.first == NULL || fpData.second == 0)
    {
        emit( failed( ExtractorNotReadyError) );
        return;
    }
        
    m_fingerprint = QByteArray( fpData.first, fpData.second );
    emit success( m_fingerprint);
}


void 
FingerprintGenerator::onStreamInitialized( long sampleRate, int channels )
{
    m_sampleRate = sampleRate;
    m_numChannels = channels;
}
