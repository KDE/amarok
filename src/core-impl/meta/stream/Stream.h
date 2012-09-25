/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_STREAM_H
#define AMAROK_STREAM_H

#include "amarok_export.h"
#include "core/meta/Meta.h"

namespace MetaStream
{
    class AMAROK_EXPORT Track : public Meta::Track
    {
        public:
            class Private;

            Track( const KUrl &url );
            virtual ~Track();

        //methods inherited from Meta::Base
            virtual QString name() const;

        //methods inherited from Meta::Track
            virtual KUrl playableUrl() const;
            virtual QString prettyUrl() const;
            virtual QString uidUrl() const;

            virtual bool isPlayable() const;
            virtual Meta::AlbumPtr album() const;
            virtual Meta::ArtistPtr artist() const;
            virtual Meta::GenrePtr genre() const;
            virtual Meta::ComposerPtr composer() const;
            virtual Meta::YearPtr year() const;

            virtual qreal bpm() const;
            virtual QString comment() const;
            virtual int trackNumber() const;
            virtual int discNumber() const;

            virtual qint64 length() const;
            virtual int filesize() const;
            virtual int sampleRate() const;
            virtual int bitrate() const;

            virtual QString type() const;

        private:
            Private * const d;
    };

}

#endif
