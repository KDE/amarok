/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef COLLECTIONSCANNER_H
#define COLLECTIONSCANNER_H

#include <qmap.h>
#include <qstringlist.h>

#include <kapplication.h>

typedef QMap<QString, QString> AttributeMap;


/**
 * @class CollectionScanner
 * @short Scans directories and builds the Collection
 */

class CollectionScanner : public KApplication
{
    Q_OBJECT

public:
    CollectionScanner( const QStringList& folders,
                       bool recursive,
                       bool incremental,
                       bool importPlaylists,
                       const QString& logfile );

    ~CollectionScanner();

private slots:
    void doJob();

private:
    void readDir( const QString& path, QStringList& entries );
    void scanFiles( const QStringList& entries );

    /**
     * Read metadata tags of a given file.
     * @path Path of the file.
     * @return QMap containing tags, or empty QMap on failure.
     */
    AttributeMap readTags( const QString& path );


    /**
     * Helper method for writing XML elements to stdout.
     * @name Name of the element.
     * @attributes Key/value map of attributes.
     */
    void writeElement( const QString& name, const AttributeMap& attributes );


    /**
     * @return the LOWERCASE file extension without the preceding '.', or "" if there is none
     */
    inline QString extension( const QString &fileName )
    {
        return fileName.contains( '.' ) ? fileName.mid( fileName.findRev( '.' ) + 1 ).lower() : "";
    }

    /**
     * @return the last directory in @param fileName
     */
    inline QString directory( const QString &fileName )
    {
        return fileName.section( '/', 0, -2 );
    }


    const bool    m_importPlaylists;
    QStringList   m_folders;
    const bool    m_recursively;
    const bool    m_incremental;
    const QString m_logfile;

    QStringList   m_processedFolders;
};


#endif // COLLECTIONSCANNER_H
