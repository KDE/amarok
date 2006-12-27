/***************************************************************************
    copyright            : (C) 2005 by Martin Aumueller
    email                : aumuell@reserv.at

    copyright            : (C) 2005 by Andy Leadbetter
    email                : andrew.leadbetter@gmail.com
                           (original mp4 implementation)
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef TAGLIB_AUDIBLEPROPERTIES_H
#define TAGLIB_AUDIBLEPROPERTIES_H

#include <config.h>

#include <taglib/audioproperties.h>
#include <taglib/tstring.h>

namespace TagLib {

    namespace Audible {

        class File;

        /*!
         * This reads the data from a Audible stream to support the
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
                 * Destroys this Audible Properties instance.
                 */
                virtual ~Properties();

                // Reimplementations.

                virtual int length() const;
                virtual int bitrate() const;
                virtual int sampleRate() const;
                virtual int channels() const;

                void readAudibleProperties(FILE *file, int off);


            private:
                void readAudioTrackProperties(FILE *file);
                friend class Audible::File;

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
