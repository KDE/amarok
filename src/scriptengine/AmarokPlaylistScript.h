/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_PLAYLIST_SCRIPT_H
#define AMAROK_PLAYLIST_SCRIPT_H

#include "core/meta/forward_declarations.h"

#include <QObject>
#include <QStringList>
#include <QVariant>

class QModelIndex;
class QScriptEngine;
class QUrl;

namespace AmarokScript
{
    class AmarokPlaylistScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokPlaylistScript( QScriptEngine* ScriptEngine );

        public slots:
            int activeIndex();
            int totalTrackCount();
            QString saveCurrentPlaylist();
            void addMedia( const QUrl &url );
            void addMediaList( const QVariantList &urls );
            void clearPlaylist();
            void playByIndex( int index );
            void playMedia( const QUrl &url );
            void playMediaList( const QVariantList &urls );
            void removeCurrentTrack();
            void removeByIndex( int index );
            void savePlaylist( const QString& path );
            void setStopAfterCurrent( bool on );
            bool stopAfterCurrent();
            void togglePlaylist();
            QStringList filenames();
            Meta::TrackPtr trackAt( int row );
            QList<int> selectedIndexes();
            QStringList selectedFilenames();

        signals:
            void trackInserted( int start, int end );
            void trackRemoved( int start, int end );

        private slots:
            void slotTrackInserted( const QModelIndex&, int start, int end );
            void slotTrackRemoved( const QModelIndex&, int start, int end );

        private:
            QScriptEngine* m_scriptEngine;
    };
}

#endif
