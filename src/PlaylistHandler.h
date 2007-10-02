/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#ifndef PlaylistHandler_H
#define PlaylistHandler_H

#include "amarok.h"
#include "debug.h"        //stack allocated
#include "meta/meta.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <QTime>

class KJob;


/**
 * @class PlaylistHandler
 * @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 * @short Class for dealing with playlists (both loading and saving)
 */

class PlaylistHandler : public QObject
{
Q_OBJECT
public:

    PlaylistHandler();

    bool isPlaylist( const KUrl &path );
    void load( const QString &path );

    enum Format { M3U, PLS, XML, RAM, SMIL, ASX, XSPF, Unknown, NotPlaylist = Unknown };

    bool save( Meta::TrackList tracks, const QString &location );


private:

    Format m_format;
    KIO::StoredTransferJob * m_downloadJob;
    QString m_contents;
    QString m_path;


    void downloadPlaylist( const KUrl & path );

    bool loadPls( QTextStream &stream );
    bool savePls( Meta::TrackList tracks, const QString &location );
    unsigned int loadPls_extractIndex( const QString &str ) const;
    bool loadM3u( QTextStream &stream );
    bool saveM3u( Meta::TrackList tracks, const QString &location );
    bool loadRealAudioRam( QTextStream& );
    bool loadASX( QTextStream& );
    bool loadSMIL( QTextStream& );
    bool loadXSPF( QTextStream& );
    bool saveXSPF( Meta::TrackList tracks, const QString &location );

    KUrl::List recurse( const KUrl & url );

    Format getFormat( const KUrl &path );
    void handleByFormat( QTextStream &stream, Format format);
    Format getType( QString &contents );

    static QTime stringToTime(const QString&);

private slots:

    void downloadComplete( KJob *job );

};

#endif

