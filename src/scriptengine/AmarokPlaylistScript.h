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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLIST_SCRIPT_H
#define AMAROK_PLAYLIST_SCRIPT_H

#include "MetaTypeExporter.h"

#include <KUrl>

#include <QList>
#include <QObject>
#include <QUrl>
#include <QtScript>
#include <QVariant>

namespace AmarokScript
{
    class AmarokPlaylistScript : public QObject
    {
        Q_OBJECT
		
        public:
            AmarokPlaylistScript( QScriptEngine* ScriptEngine, QList<QObject*>* wrapperList );
            ~AmarokPlaylistScript();

        public slots:
            int activeIndex();
            int totalTrackCount();
            QString saveCurrentPlaylist();
            void addMedia( const QUrl &url );
            void addMediaList( const QVariantList &urls );
            void clearPlaylist();
            void playByIndex( int index );
            void playMedia( const QUrl &url );
            void removeCurrentTrack();
            void removeByIndex( int index );
            void savePlaylist( const QString& path );
            void setStopAfterCurrent( bool on );
            void togglePlaylist();
            QStringList filenames();
            QVariant trackAt( int row );
            QList<int> selectedIndexes();
            QStringList selectedFilenames();

        signals:
            void activeRowChanged( int row );
            void trackInserted( int start, int end );
            void trackRemoved( int start, int end );

        private slots:
            void slotTrackInserted( const QModelIndex&, int start, int end );
            void slotTrackRemoved( const QModelIndex&, int start, int end );

        private:
            QList<QObject*>* m_wrapperList;
            QScriptEngine* m_scriptEngine;
    };
}

#endif
