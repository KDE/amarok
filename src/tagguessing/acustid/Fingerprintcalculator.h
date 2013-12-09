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

#ifndef FINGERPRINTCALCULATOR_H
#define FINGERPRINTCALCULATOR_H

#include "libchromaprint.h"
#include "tagguessing/AudioToQStringDecoder.h"

class AudioDataFeeder : public TagGuessing::DecodedAudioData
{
    public:
        AudioDataFeeder();
        ~AudioDataFeeder();

        /**
         * Overrides base class' function and starts the computation of fingerprint
         */
        int setupInitial( const int sampleRate, const quint8 channels );

        /**
         * Overrides the base class' function since this is called by MusicDNSAudioDecoder's
         * private function "decode". This is required by libchromaprint.
         * Does not enter this data to the base class' m_data since its not needed at any time in the future
         */
        int appendData( const quint8 *data, const int length );

    private:
        const ChromaprintContext *m_chromarpintContext;
};

class FingerprintCalculator : public TagGuessing::AudioToQStringDecoder
{
    public:
        FingerprintCalculator( const Meta::TrackList &tracks, const int sampleLength = DEFAULT_SAMPLE_LENGTH );
        ~FingerprintCalculator();

        void run();
};

#endif // FINGERPRINTCALCULATOR_H
