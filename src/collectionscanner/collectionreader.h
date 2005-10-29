// (c) The amaroK developers 2003-4
// See COPYING file that comes with this distribution
//

#ifndef COLLECTIONREADER_H
#define COLLECTIONREADER_H

#include <fstream>
#include "threadweaver.h"
#include <qstringlist.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

class DbConnection;


/**
 * @class CollectionReader
 * @short Scans directories and builds the Collection
 */

class CollectionReader : public ThreadWeaver::DependentJob
{
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

    CollectionReader( CollectionDB* parent, const QStringList& folders );
   ~CollectionReader();

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
 * @class IncrementalCollectionReader
 * @short Only scans directories that have been modified since the last scan
 */

class IncrementalCollectionReader : public CollectionReader
{
public:
    IncrementalCollectionReader( CollectionDB* );

    bool hasChanged() const { return m_hasChanged; }

protected:
    virtual bool doJob();

    bool m_hasChanged;
};

#endif
