/***************************************************************************
 *   Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>              *
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

#ifndef TRACKORGANIZER_H
#define TRACKORGANIZER_H

#include "amarok_export.h"
#include "core/meta/forward_declarations.h"

#include <QObject>
#include <QMap>

/**
 * Generates a list of paths formatted according to the specified
 * format string.
 * @author Casey Link
 */
class AMAROK_EXPORT TrackOrganizer : public QObject
{
    Q_OBJECT
public:
    explicit TrackOrganizer( const Meta::TrackList &tracks, QObject* parent = 0 );

    /**
     * Sets the format string. Required.
     * @param format the format, e.g., %artist - %title.%filetype
     */
    void setFormatString( const QString &format );
    /**
     * Sets the folder (i.e. collection prefix)
     * @param prefix the folder prefix, e.g.,  /home/user/Music/
     */
    void setFolderPrefix( const QString &prefix );

    /**
     * Sets whether to move the "the" in an artist name to the end of the name.
     * Default value is false.
     * @param flag turns the option on
     */
    void setPostfixThe( bool flag );
    /**
     * Sets whether to restrict filenames to ASCII
     * Default value is false.
     * @param flag turns the option on
     */
    void setAsciiOnly( bool flag );
    /**
     * Sets whether to replaces spaces with underscores.auto
     * Default value is false.
     * @param flag turns the option on
     */
    void setReplaceSpaces( bool flag );
    /**
     * Sets whether to restrict filenames to VFAT safe names.
     * Default value is false.
     * @param flag turns the option on
     */
    void setVfatSafe( bool flag);
    /**
     * Sets a regex and replacement string to perform custom replacement
     * @param regex the regex value
     * @param string the string substitute for the regex match
     */
    void setReplace( const QString &regex, const QString &string );
    /**
     * Sets a new file extension for the target file names.
     * @param fileExtension the file extension
     */
    void setTargetFileExtension( const QString &fileExtension );

    /**
     * Get the list of processed destinations
     * Only call after setting all the appropriate options
     * @see setFormatString
     * @arg batchSize How many to return this run of the function. If 0 (default) will calculate the
     * complete list. This function can return a shorter list at the end of the results list.
     * Over consecutive runs of this function the same number of results as the length of the
     * tracklist passed in the constructor will be returned.
     */
    QMap<Meta::TrackPtr, QString> getDestinations( unsigned int batchSize = 0 );

    /** Call this function if you want getDestinations to return results starting from the
        first track. */
    void resetTrackOffset() { m_trackOffset = 0; }


private:
    QString buildDestination( const QString &format, const Meta::TrackPtr &track ) const;
    QString cleanPath( const QString &path ) const;

    /** Returns the number of characters that are the same in both strings beginning. */
    static int commonPrefixLength( const QString &a, const QString &b );

    Meta::TrackList m_allTracks;
    /** The starting track that is to be processed. */
    int m_trackOffset;

    //options
    QString m_format;
    QString m_folderPrefix;
    bool m_postfixThe;
    bool m_AsciiOnly;
    bool m_UnderscoresNotSpaces;
    bool m_vfatSafe;
    QString m_regexPattern;
    QString m_replaceString;
    QString m_targetFileExtension;
};

#endif // TRACKORGANIZER_H
