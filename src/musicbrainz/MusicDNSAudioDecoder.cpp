/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicDNSAudioDecoder"

#include "MusicDNSAudioDecoder.h"

#include <config-amarok.h>
#include "core/support/Debug.h"

extern "C" {
    typedef quint64 UINT64_C;
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
}

#include <ofa1/ofa.h>

DecodedAudioData::DecodedAudioData()
                 : m_sRate( 0 )
                 , m_channels( 0 )
                 , m_duration( 0 )
                 , m_data( new QByteArray )
{
}

DecodedAudioData::~DecodedAudioData()
{
    if( m_data )
        delete m_data;
}

int
DecodedAudioData::sRate()
{
    return m_sRate;
}

void
DecodedAudioData::setSampleRate( const int sampleRate )
{
    m_sRate = sampleRate;
}

quint8
DecodedAudioData::channels()
{
    return m_channels;
}

void
DecodedAudioData::setChannels( const quint8 channels )
{
    m_channels = channels;
}

const char *
DecodedAudioData::data()
{
    return m_data->data();
}

int
DecodedAudioData::duration()
{
    return m_duration;
}

void
DecodedAudioData::addTime( const int ms )
{
    m_duration += ms;
}

int
DecodedAudioData::length()
{
    return m_data->length();
}

DecodedAudioData &DecodedAudioData::operator<< ( const quint8 &byte )
{
    m_data->append( byte );
    return *this;
}

void DecodedAudioData::flush()
{
    m_sRate = 0;
    m_channels = 0;
    m_duration = 0;
    m_data->clear();
}

MusicDNSAudioDecoder::MusicDNSAudioDecoder( const Meta::TrackList &tracks, const int sampleLength )
                    : Job()
                    , m_tracks( tracks )
                    , m_sampleLength( sampleLength )
{
}

MusicDNSAudioDecoder::~MusicDNSAudioDecoder()
{

}

void
MusicDNSAudioDecoder::run()
{
    DecodedAudioData data;
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *decodedFrame = NULL;
    AVPacket packet, tmpPacket;
    int audioStream, decoderRet, i, j;

    av_register_all();

    av_init_packet( &tmpPacket );

    foreach( Meta::TrackPtr track, m_tracks )
    {
        if( avformat_open_input( &pFormatCtx, ( const char * )track->playableUrl().toLocalFile().toLocal8Bit(), NULL, NULL ) )
        {
            warning() << QLatin1String( "Unable to open input file: " ) + track->playableUrl().toLocalFile();
            continue;
        }

        if( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
        {
            warning() << QLatin1String( "Unable to find stream info: " ) + track->playableUrl().toLocalFile();
            avformat_close_input( &pFormatCtx );
            continue;
        }

        audioStream = -1;
        for(i = 0; i < ( int )pFormatCtx->nb_streams; i++ )
            if( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
            {
                audioStream = i;
                break;
            }

        if( audioStream == -1 )
        {
            warning() << QLatin1String( "Unable to find stream: " ) + track->playableUrl().toLocalFile();
            avformat_close_input( &pFormatCtx );
            continue;
        }

        pCodecCtx = pFormatCtx->streams[audioStream]->codec;
        pCodec = avcodec_find_decoder( pCodecCtx->codec_id );
        if( pCodec == NULL )
        {
            warning() << QLatin1String( "Unable to find codec for " ) + track->playableUrl().toLocalFile();
            avformat_close_input( &pFormatCtx );
            continue;
        }

        if( avcodec_open2( pCodecCtx, pCodec, NULL ) < 0 )
        {
            warning() << QLatin1String( "Unable to open codec " ) + track->playableUrl().toLocalFile();
            avformat_close_input( &pFormatCtx );
            continue;
        }

        if( !( pCodecCtx->sample_rate && pCodecCtx->channels ) )
        {
            warning() << QLatin1String( "Ivalid codec context, file might be corrupted: " ) + track->playableUrl().toLocalFile();
            avcodec_close( pCodecCtx );
            avformat_close_input( &pFormatCtx );
            continue;
        }

        data.setSampleRate( pCodecCtx->sample_rate );
        data.setChannels( ( pCodecCtx->channels > 1 )? 1 : 0 );

        bool isOk = true;
        while( !av_read_frame( pFormatCtx, &packet ) && isOk )
        {
            if( packet.stream_index == audioStream )
            {
                tmpPacket = packet;
                while( tmpPacket.size > 0 )
                {
                    int gotFrame = 0;

                    if( !decodedFrame )
                    {
                        if( !( decodedFrame = avcodec_alloc_frame() ) )
                        {
                            warning() << "Unable to allocate enough memory to decode file.";
                            isOk = false;
                            break;
                        }
                        else
                            avcodec_get_frame_defaults( decodedFrame );
                    }

                    decoderRet = avcodec_decode_audio4( pCodecCtx, decodedFrame, &gotFrame, &tmpPacket );
                    if( decoderRet < 0 )
                    {
                        warning() << "Error while decoding.";
                        isOk = false;
                        break;
                    }

                    tmpPacket.size -= decoderRet;
                    tmpPacket.data += decoderRet;

                    if( gotFrame )
                    {
                        int dataSize = av_samples_get_buffer_size( NULL, pCodecCtx->channels, decodedFrame->nb_samples, pCodecCtx->sample_fmt, 1);
                        for( i = 0; i < dataSize; i += pCodecCtx->channels*2  )
                            for( j = 0; j < pCodecCtx->channels*2; j++ )
                                if( j < 4 )
                                    data << decodedFrame->data[0][i + j];
                                else
                                    break;
                    }
                    else
                        continue;
                }
                data.addTime( ( packet.size * 500 ) / ( pCodecCtx->channels * pCodecCtx->sample_rate ) );
            }

            av_free_packet( &packet );

            if( data.duration() >= m_sampleLength )
                break;
        }

        if( data.duration() > MIN_SAMPLE_LENGTH )
        {
            QString fingerprint( ofa_create_print( ( unsigned char * ) data.data(),
                                                   OFA_LITTLE_ENDIAN, ( data.length() >> 1 ),
                                                   data.sRate(), data.channels() ) );
            emit trackDecoded( track, fingerprint );
        }
        else
            warning() << QLatin1String( "Some error occurred during fingerprint generation, probably track is too short: " ) +
                         track->playableUrl().toLocalFile();

        avcodec_close( pCodecCtx );
        avformat_close_input( &pFormatCtx );
        data.flush();
    }
}


#include "MusicDNSAudioDecoder.moc"
