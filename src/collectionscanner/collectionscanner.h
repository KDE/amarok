/***************************************************************************
 *   Copyright (C) 2003-2005 by The Amarok Developers                      *
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

#include "metabundle.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
                       bool restart );

    ~CollectionScanner();
    int newInstance() { return 0; }

public slots:
    void pause();
    void resume();

private slots:
    void doJob();

private:
    void readDir( const QString& dir, QStringList& entries );
    void scanFiles( const QStringList& entries );

    /**
     * Read metadata tags of a given file.
     * @mb MetaBundle for the file.
     * @return QMap containing tags, or empty QMap on failure.
     */
    AttributeMap readTags( const MetaBundle& mb );

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
    const bool    m_restart;
    const QString m_logfile;
    bool          m_pause;

    struct direntry {
      dev_t dev;
      ino_t ino;
    } KDE_PACKED;

    QMemArray<direntry> m_processedDirs;
};


#endif // COLLECTIONSCANNER_H
