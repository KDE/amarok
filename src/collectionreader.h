// (c) The amaroK developers 2003-4
// See COPYING file that comes with this distribution
//

#ifndef COLLECTIONREADER_H
#define COLLECTIONREADER_H

#include <iosfwd> //std::ofstream
#include "threadweaver.h"
#include <qstringlist.h>

class CollectionDB;


class CollectionReader : public ThreadWeaver::DependentJob
{
public:
   static const int ProgressEventType = 8889;
   class ProgressEvent : public QCustomEvent
   {
       public:
           enum State { Start = -1, Stop = -2, Total = -3, Progress = -4 };

           ProgressEvent( int state, int value = -1 )
           : QCustomEvent( ProgressEventType )
           , m_state( state )
           , m_value( value ) {}
           int state() { return m_state; }
           int value() { return m_value; }
       private:
           int m_state;
           int m_value;
   };

   static const int PlaylistFoundEventType = 8890;
   class PlaylistFoundEvent : public QCustomEvent
   {
       public:
           PlaylistFoundEvent( QString path )
           : QCustomEvent( PlaylistFoundEventType )
           , m_path( path ) {}
           QString path() { return m_path; }
       private:
           QString m_path;
   };

    CollectionReader( CollectionDB* parent, QObject* playlistBrowser, const QStringList& folders,
                      bool recursively, bool importPlaylists, bool incremental );
    ~CollectionReader();
    
    static void stop() { m_stop = true; }
    bool doJob();

private:
    void readDir( const QString& dir, QStringList& entries );
    void readTags( const QStringList& entries, std::ofstream& log );

    static bool m_stop;

    CollectionDB* m_parent;
    DbConnection* m_staticDbConnection;
    QObject* m_playlistBrowser;
    QStringList m_folders;
    bool m_recursively;
    bool m_importPlaylists;
    bool m_incremental;
    QStringList m_processedDirs;
};

#endif
