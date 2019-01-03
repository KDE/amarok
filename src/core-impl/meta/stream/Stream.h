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

            explicit Track( const QUrl &url );
            virtual ~Track();

        // methods inherited from Meta::Base
            QString name() const override;

        // methods inherited from Meta::Track
            QUrl playableUrl() const override;
            QString prettyUrl() const override;
            QString uidUrl() const override;
            QString notPlayableReason() const override;

            Meta::AlbumPtr album() const override;
            Meta::ArtistPtr artist() const override;
            Meta::GenrePtr genre() const override;
            Meta::ComposerPtr composer() const override;
            Meta::YearPtr year() const override;

            qreal bpm() const override;
            QString comment() const override;
            int trackNumber() const override;
            int discNumber() const override;

            qint64 length() const override;
            int filesize() const override;
            int sampleRate() const override;
            int bitrate() const override;

            void finishedPlaying( double playedFraction ) override;

            QString type() const override;

        // MetaStream::Track methods, used to restore initial stream info

            /**
             * Set initial values to display before more accurate info can be fetched.
             * This method doesn't call notifyObservers(), it is the caller's
             * responsibility; it also doesn't overwrite already filled entries.
             *
             * @param artist track artist
             * @param album track album
             * @param title track title
             * @param length is in milliseconds
             * @param trackNumber track number
             */
            void setInitialInfo( const QString &artist, const QString &album,
                                 const QString &title, qint64 length, int trackNumber );

        private:
            Private * const d;
    };

}

#endif
