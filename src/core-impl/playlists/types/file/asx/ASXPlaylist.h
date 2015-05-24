/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                                                                                     *
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
#ifndef ASXPLAYLIST_H
#define ASXPLAYLIST_H

#include "core-impl/playlists/types/file/PlaylistFile.h"

#include <QDomElement>

namespace Playlists {
/**
 *  TODO: Use QDomDocument locally just in saving and loading methods.
 */
class AMAROK_EXPORT ASXPlaylist : public PlaylistFile, public QDomDocument
{
    public:
        ASXPlaylist( const QUrl &url, PlaylistProvider *provider = 0 );

        virtual bool save( bool relative ) { return PlaylistFile::save( relative ); }

        using PlaylistFile::load;
        virtual bool load( QTextStream &stream ) { return loadAsx( stream ); }

        virtual QString extension() const { return "asx"; }
        virtual QString mimetype() const { return "video/x-ms-asf"; }

    protected:
        bool loadAsx( QTextStream &stream );
        /** Writes tracks to file */
        void writeTrackList();
        virtual void savePlaylist( QFile &file );
        bool processContent( QTextStream &stream );
};
}
#endif
