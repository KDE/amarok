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

#ifndef MUSICDNSAUDIODECODER_H
#define MUSICDNSAUDIODECODER_H

#include "core/meta/forward_declarations.h"

#include <ThreadWeaver/Job>

#define DEFAULT_SAMPLE_LENGTH 135000
#define MIN_SAMPLE_LENGTH 10000

class DecodedAudioData
{
    public:
        DecodedAudioData();
        ~DecodedAudioData();

        int sRate();
        void setSampleRate( const int sampleRate );

        quint8 channels();
        void setChannels( const quint8 channels );

        int length();
        qint64 duration();
        void addTime( const qint64 ms );

        const char *data();

        void appendData( const quint8 *data, int length );
        DecodedAudioData &operator<< ( const quint8 &byte );

        void flush();

    private:
        int m_sRate;
        quint8 m_channels;
        qint64 m_duration;

        QByteArray *m_data;
};

class MusicDNSAudioDecoder : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        MusicDNSAudioDecoder( const Meta::TrackList &tracks, const int sampleLength = DEFAULT_SAMPLE_LENGTH );
        virtual ~MusicDNSAudioDecoder();

        void run();

    Q_SIGNALS:
        void trackDecoded( const Meta::TrackPtr, const QString );

    private:
        int decode( const QString &fileName, DecodedAudioData *data, const int length );

        Meta::TrackList m_tracks;
        int m_sampleLength;
};

#endif // MUSICDNSAUDIODECODER_H
