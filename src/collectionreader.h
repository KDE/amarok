// (c) The amaroK developers 2003-4
// See COPYING file that comes with this distribution
//

#ifndef COLLECTIONREADER_H
#define COLLECTIONREADER_H

#include <fstream>
#include "threadweaver.h"
#include <qstringlist.h>

class DbConnection;


/**
 * @class IncrementalCollectionReader
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

    void readDir( const QString& dir, QStringList& entries );
    void readTags( const QStringList& entries );

    DbConnection* const m_db;
    QStringList m_folders;

    bool m_recursively;
    bool m_importPlaylists;
    bool m_incremental;

    QStringList m_processedDirs;

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

protected:
    virtual bool doJob();
};

#endif
