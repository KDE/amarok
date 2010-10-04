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

#define DEFAULT_SAMPLE_LENGTH 135000
#define MIN_SAMPLE_LENGTH 10000

#include <core/meta/Meta.h>
#include <threadweaver/Job.h>

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
        int duration();
        void addTime( const int ms );

        const char *data();
        DecodedAudioData &operator<< ( const quint8 &byte );

        void flush();

    private:
        int m_sRate;
        quint8 m_channels;
        int m_duration;

        QByteArray *m_data;
};

class MusicDNSAudioDecoder : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        MusicDNSAudioDecoder( const Meta::TrackList &tracks, const int sampleLength = DEFAULT_SAMPLE_LENGTH );
        virtual ~MusicDNSAudioDecoder();

        void run();

    signals:
        void trackDecoded( const Meta::TrackPtr, const QString );

    private:
        Meta::TrackList m_tracks;
        int m_sampleLength;

};

#endif // MUSICDNSAUDIODECODER_H
