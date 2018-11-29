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

#ifndef COLLECTIONSCANNER_PLAYLIST_H
#define COLLECTIONSCANNER_PLAYLIST_H

#include "amarokshared_export.h"

#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace CollectionScanner
{

/**
 * @class Playlist
 * @short Represents a playlist
 */

class AMAROKSHARED_EXPORT Playlist
{
public:
    explicit Playlist( const QString &path );
    explicit Playlist( QXmlStreamReader *reader );

    /** The absolute path to the file.
     *  Because of symbolic links the path could be outside the original scanning directories.
     */
    QString path() const;

    /** Returns the relative path at the point of scanning */
    QString rpath() const;


    /** Writes the contents of this object to an xml stream.
     *  Only the content is written and no enclosing directory tags.
     *  This is done to make it mirror the constructor which does not read those
     *  tags either.
     */
    void toXml( QXmlStreamWriter *writer ) const;

private:
    QString m_path;
    QString m_rpath;
};

}

#endif // COLLECTIONSCANNER_PLAYLIST_H
