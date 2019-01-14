/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef META_TRACKEDITOR_H
#define META_TRACKEDITOR_H

#include "core/amarokcore_export.h"

#include "AmarokSharedPointer.h"

namespace Meta
{
    /**
     * Capability to edit meta-data of a track.
     *
     * If you are calling more than one setter method, you should call beginUpdate()
     * before calling any setter methods and endUpdate() when you're done.
     *
     * This class is memory-managed exclusively using AmarokSharedPointers: always use
     * TrackEditorPtr to store or pass pointer to this class. This class must be
     * implemented in a reentrant manner. Additionally, underlying Meta::Track must
     * be thread-safe -- if you return same instance of TrackEditor every time then it
     * means that even the instance must be thread-safe.
     */
    class AMAROKCORE_EXPORT TrackEditor : public virtual QSharedData // virtual inheritance
    // so that Track implementations can inherit both Meta::Track and Meta::TrackEditor
    {
        public:
            virtual ~TrackEditor();

            virtual void setAlbum( const QString &newAlbum ) = 0;
            virtual void setAlbumArtist( const QString &newAlbumArtist ) = 0;
            virtual void setArtist( const QString &newArtist ) = 0;
            virtual void setComposer( const QString &newComposer ) = 0;
            virtual void setGenre( const QString &newGenre ) = 0;
            virtual void setYear( int newYear ) = 0;
            virtual void setTitle( const QString &newTitle ) = 0;
            virtual void setComment( const QString &newComment ) = 0;
            virtual void setTrackNumber( int newTrackNumber ) = 0;
            virtual void setDiscNumber( int newDiscNumber ) = 0;
            virtual void setBpm( const qreal newBpm ) = 0;

            /**
             * The track object should not store changed meta data immediately but cache
             * the changes until endUpdate() is called
             */
            virtual void beginUpdate() = 0;

            /**
             * All meta data has been updated and the object should commit the changes
             */
            virtual void endUpdate() = 0;
    };

    typedef AmarokSharedPointer<TrackEditor> TrackEditorPtr;
}

#endif // META_TRACKEDITOR_H
