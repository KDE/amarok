/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef AUDIOTOQSTRINGDECODER_H
#define AUDIOTOQSTRINGDECODER_H

#include "core/meta/forward_declarations.h"

#include <ThreadWeaver/Job>

#define DEFAULT_SAMPLE_LENGTH 135000
#define MIN_SAMPLE_LENGTH 10000

/**
 * NOTE: these classes will be compiled only if libofa is found
 */
namespace TagGuessing {
    class DecodedAudioData;
    class AudioToQStringDecoder;
}

/**
 * Some methods are virtual so that this class can be properly extended
 */
class DecodedAudioData
{
    public:
        DecodedAudioData();
        virtual ~DecodedAudioData();

        int sRate() const;
        quint8 channels() const;

        /**
         * sets up the initial @param sampleRate and @param channels.
         * Also, serves as a marker for the beginning of decoding of audio
         * @return error code. Base implementation always returns 0 (no error)
         */
        virtual int setupInitial(const int sampleRate, const quint8 &channels );

        int length() const;
        qint64 duration() const;
        void addTime(const qint64 &ms );

        const char *data() const;

        /**
         * appends the first @param length in @param data to m_data
         * Also, servers as a marker for every data packet read from audio file
         * @return error code. Base implementation always returns 0 (no error)
         */
        virtual int appendData( const quint8 *data, int length );
        DecodedAudioData &operator<< ( const quint8 &byte );

        void flush();

    private:
        int m_sRate;
        quint8 m_channels;
        qint64 m_duration;

        QByteArray *m_data;
};

class AudioToQStringDecoder : public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        AudioToQStringDecoder( const Meta::TrackList &tracks, const int sampleLength = DEFAULT_SAMPLE_LENGTH );
        virtual ~AudioToQStringDecoder();

        virtual void run();

    signals:
        void trackDecoded( const Meta::TrackPtr, const QString );

    protected:
        int decode( const QString &fileName, DecodedAudioData *data, const int length );

        Meta::TrackList m_tracks;
        int m_sampleLength;
};

#endif // AUDIOTOQSTRINGDECODER_H
