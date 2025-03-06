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

#ifndef COLLECTIONSCANNER_TRACK_H
#define COLLECTIONSCANNER_TRACK_H

#include "FileType.h"
#include "MetaReplayGain.h"
#include "amarokshared_export.h"

#include <QString>
#include <QDateTime>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace CollectionScanner
{

class Directory;

/**
   @class Track
   @short Represents a scanned track
   An empty QString or a negative int means that the value is unset.
 */
class AMAROKSHARED_EXPORT Track
{
public:
    /** Reads a track from the given path */
    Track( const QString &path, Directory* directory );

    /** Tries to parse the track from the xml stream */
    Track( QXmlStreamReader *reader, Directory* directory );

    /** Writes the contents of this object to an xml stream.
        Only the content is written and no enclosing directory tags.
        This is done to make it mirror the constructor which does not read those
        tags either.
     */
    void toXml( QXmlStreamWriter *writer ) const;

    /** Returns true if this track is really a song. */
    bool isValid() const;

    /** Returns the directory this track was found in. */
    Directory* directory() const;

    QString uniqueid() const;

    /** The absolute path to the file.
        Because of symbolic links the path could be outside the original scanning directories.
     */
    QString path() const;

    /** Returns the relative path at the point of scanning */
    QString rpath() const;

    Amarok::FileType filetype() const;
    QString title() const;
    QString artist() const;
    QString albumArtist() const;
    QString album() const;
    // if !isCompilation && !isNoCompilation then it's undefined
    bool isCompilation() const;
    bool isNoCompilation() const;
    bool hasCover() const;
    QString comment() const;
    QString genre() const;
    int year() const;
    int disc() const;
    int track() const;
    int bpm() const;
    int bitrate() const;
    qint64 length() const;
    int samplerate() const;
    qint64 filesize() const;
    QDateTime modified() const;

    QString composer() const;

    qreal replayGain( Meta::ReplayGainTag mode ) const;

    /** Rating is a value from 0.0 to 10.0 inclusive */
    qreal rating() const;

    /** Score is a value from 0.0 to 100.0 inclusive */
    qreal score() const;

    int playcount() const;

private:
    Q_DISABLE_COPY(Track)

    void write( QXmlStreamWriter *writer, const QString &tag, const QString &str ) const;
    bool m_valid;

    Directory* m_directory;

    QString m_uniqueid;

    QString m_path;
    QString m_rpath;

    Amarok::FileType m_filetype;
    QString m_title;
    QString m_artist;
    QString m_albumArtist;
    QString m_album;
    bool m_compilation;
    bool m_noCompilation;
    bool m_hasCover;
    QString m_comment;
    QString m_genre;
    int m_year;
    int m_disc;
    int m_track;
    qreal m_bpm;
    int m_bitrate;
    qint64 m_length;
    int m_samplerate;
    qint64 m_filesize;
    QDateTime m_modified;

    qreal m_trackGain;
    qreal m_trackPeakGain;
    qreal m_albumGain;
    qreal m_albumPeakGain;

    QString m_composer;

    qreal m_rating;
    qreal m_score;
    int m_playcount;
};

}

#endif // COLLECTIONSCANNER_TRACK_H
