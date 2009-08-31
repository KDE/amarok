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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_COLLECTION_SCRIPT_H
#define AMAROK_COLLECTION_SCRIPT_H

#include <QObject>
#include <QtScript>

namespace AmarokScript
{
    class AmarokCollectionScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokCollectionScript( QScriptEngine* ScriptEngine );
            ~AmarokCollectionScript();

        public slots:
            int totalAlbums() const;
            int totalArtists() const;
            int totalComposers() const;
            int totalGenres() const;
            int totalTracks() const;
            QStringList collectionLocation() const;
            QStringList query( const QString& sql ) const;
            QString escape( const QString& sql ) const;
            void scanCollection() const;
            void scanCollectionChanges() const;
            bool isDirInCollection( const QString& path ) const;

            //this method will dump the database content into the user's home directory
            //debugging only! this is not part of the supported scripting API
            void dumpDatabaseContent() const;

           //TODO: make this a more object oriented way, could make a wrapper class for both the Collection and Collection Manager class.
           //TODO: probably leave this to Amarok 2.1
        signals:
            void newFileAdded();  //TODO: implement me
    };
}

#endif
