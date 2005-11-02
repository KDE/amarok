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

#include <fstream>

#include <taglib/audioproperties.h>

#include <qstringlist.h>
#include <kapplication.h>


/**
 * @class CollectionScanner
 * @short Scans directories and builds the Collection
 */

class CollectionScanner : public KApplication
{
    Q_OBJECT

public:
    CollectionScanner( const QStringList& folders,
                       bool recursive = false,
                       bool importPlaylists = false );

    ~CollectionScanner();

private slots:
    void doJob();

private:
    void readDir( const QString& path, QStringList& entries );
    void scanFiles( const QStringList& entries );

    /** If you want Accurate reading say so */
    void readTags( const QString& path, TagLib::AudioProperties::ReadStyle );

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


    bool        m_importPlaylists;
    QStringList m_folders;
    bool        m_recursively;

    std::ofstream log;
};


#endif // COLLECTIONSCANNER_H
