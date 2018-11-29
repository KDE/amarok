/***************************************************************************
 *   Copyright (C) 2010 Ralf Engels <ralf-engels@gmx.de>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef COLLECTIONSCANNER_DIRECTORY_H
#define COLLECTIONSCANNER_DIRECTORY_H

#include "Album.h"
#include "Playlist.h"
#include "amarokshared_export.h"

#include <QString>
#include <QList>
#include <QHash>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace CollectionScanner
{

class Track;
class ScanningState;

/**
 * @class Directory
 * @short Represents a scanned directory and it's contents
 */
class AMAROKSHARED_EXPORT Directory
{
public:
    /**
     * This constructor actually scans the directory and is supposed only to be
     * called by CollectionScanner directly.
     */
    Directory( const QString &path, ScanningState *state, bool skip );

    /** Reads a directory from an xml stream.
     * @see toXml()
     */
    explicit Directory( QXmlStreamReader *reader );

    ~Directory();

    /** The absolute path to the file.
     *  Because of symbolic links the path could be outside the original scanning directories.
     */
    QString path() const;

    /** Returns the relative path at the point of scanning */
    QString rpath() const;

    /** Returns the modification time of the directory. */
    uint mtime() const;

    /** Returns true if the directory was skipped and not scanned.
     *  Usually this is being done because the directory was unmodified in an
     *  Incremental scan.
     */
    bool isSkipped() const;

    const QStringList& covers() const;
    const QList<Track*>& tracks() const;
    const QList<Playlist>& playlists() const;

    /** Writes the contents of this object to an xml stream.
     *  Only the content is written and no enclosing directory tags.
     *  This is done to make it mirror the constructor which does not read those
     *  tags either.
     */
    void toXml( QXmlStreamWriter *writer ) const;

private:
    Q_DISABLE_COPY(Directory)

    QString m_path;
    QString m_rpath;
    uint m_mtime;
    bool m_skipped;
    bool m_ignored; // the directory was ignored e.g. because of "fmps_ignore"

    QStringList m_covers;
    QList<Track*> m_tracks;
    QList<Playlist> m_playlists;
};

}

#endif // COLLECTIONSCANNER_DIRECTORY_H
