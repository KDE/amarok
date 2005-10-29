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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <qstringlist.h>


/**
 * @class CollectionScanner
 * @short Scans directories and builds the Collection
 */

class CollectionScanner : public QObject
{
    Q_OBJECT

public:
    static const int PlaylistFoundEventType = 8890;

    class PlaylistFoundEvent : public QCustomEvent {
    public:
        PlaylistFoundEvent( QString path )
            : QCustomEvent( PlaylistFoundEventType )
            , m_path( path ) {}
        QString path() { return m_path; }
    private:
        QString m_path;
    };

    CollectionScanner( CollectionDB* parent, const QStringList& folders );
   ~CollectionScanner();

protected:
    virtual bool doJob();

    bool m_importPlaylists;
    bool m_incremental;
    QStringList m_folders;

private:
    void readDir( const QString& dir, QStrList& entries );
    void readTags( const QStrList& entries );

    DbConnection* const m_db;

    bool m_recursively;

    struct direntry {
      dev_t dev;
      ino_t ino;
    } KDE_PACKED;

    QMemArray<direntry> m_processedDirs;

    std::ofstream log;
};


/**
 * @class IncrementalCollectionScanner
 * @short Only scans directories that have been modified since the last scan
 */

class IncrementalCollectionScanner : public CollectionScanner
{
public:
    IncrementalCollectionScanner( CollectionDB* );

    bool hasChanged() const { return m_hasChanged; }

protected:
    virtual bool doJob();

    bool m_hasChanged;
};

#endif // COLLECTIONSCANNER_H
