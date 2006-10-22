// (C) 2005 Martin Aumueller <aumuell@reserv.at>
// portions are (C) 2005 Umesh Shankar <ushankar@cs.berkeley.edu>
//          and (C) 2005 Andy Leadbetter <andrew.leadbetter@gmail.com>
//
// See COPYING file for licensing information

#ifndef TAGLIB_WAVPROPERTIES_H
#define TAGLIB_WAVPROPERTIES_H

#include <config.h>

#include <taglib/audioproperties.h>
#include <taglib/tstring.h>

namespace TagLib {

    namespace Wav {

        class File;

        /*!
         * This reads the data from a Wav stream to support the
         * AudioProperties API.
         */

        class Properties : public AudioProperties
        {
            public:
                /*!
                 * Initialize this structure
                 */
                Properties(Properties::ReadStyle style);

                /*!
                 * Destroys this Wav Properties instance.
                 */
                virtual ~Properties();

                // Reimplementations.

                virtual int length() const;
                virtual int bitrate() const;
                virtual int sampleRate() const;
                virtual int channels() const;

                void readWavProperties(FILE *file);


            private:
                void readAudioTrackProperties(FILE *file);
                friend class Wav::File;

                int m_length;
                int m_bitrate;
                int m_sampleRate;
                int m_channels;

                Properties(const Properties &);
                Properties &operator=(const Properties &);

                void read();
        };
    }
}

#endif
