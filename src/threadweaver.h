//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef THREADWEAVER_H
#define THREADWEAVER_H

#include <kurl.h>         //stack allocated

#include <qevent.h>       //baseclass
#include <qmutex.h>       //stack allocated
#include <qstringlist.h>  //stack allocated
#include <qthread.h>      //baseclass
#include <qptrlist.h>     //stack allocated

class MetaBundle;
class PlaylistItem;
class CollectionDB;
class KListView;
class KListViewItem;
class QListView;
class QListViewItem;
class QString;
class QStringList;
class QWidget;


//  Weavers allow you to use a single QThread for a variety of computationally expensive tasks.
//
//  Weavers process ThreadWeaver::Jobs. Derive from one and reimplement doJob(). The weaver will process
//  all the jobs append()ed to its queue in order. Jobs are processed in FIFO order unless you set the
//  priority to immediate (see append()).
//
//  The class is designed to handle frequently requested tasks, or situations where you have lots of
//  different tasks that must all be handled by a thread. For example, the playlist dispatches a Job for
//  every file that requires tag reading. It also has a Job for writing tags, and reading audio properties.
//  If you plan to batch process something you probably should just derive a class from QThread.
//
//  When a job has completed and returned true from doJob(), it is dispatched to the specified target object
//  which can recieve the Job itself as an event in its QCustomEvent().
//
//  cancel() will prevent any further Jobs being dispatched and will clear the queue. However almost
//  certainly some Jobs will already be in Qt's event queue on there way to the target. The user has to ensure
//  this situation is safe. See Playlist::clear() for a good solution.
//
//  halt() will permanantly stop the thread, it should be used in conjunction with Qthread::wait() when the
//  program is exiting.
//
//  The parent of the thread also gets two other events, ThreadWeaver::Started and ThreadWeaver::Done,
//  you may want to set the application overrideCursor based on these events for instance.
//
//  Finally, please note, QThread is not a QObject so you must delete the ThreadWeaver in your classes dtor.
//
//  The class is not complete in that it has a few obvious flaws, feel free to improve!

class ThreadWeaver : public QThread
{
public:
   ThreadWeaver( QObject* );

   class Job;

   void append( Job* const, bool = false );
   bool remove( Job* const ); //TODO implement virtual operator== ?
   void cancel();
   void halt() { m_bool = false; } //thread-safe, permanant shutdown

   enum EventType { Started = 2000, Done = 2001 };


   //  Derive a job to perform a computationaly expensive task that will not block the UI
   //  Implement doJob() to perform the task.
   //  @param QObject *
   //    The Job isa QEvent, and will be posted to this object if you return true from your implementation
   //    of doJob()
   //  @param JobType
   //    This QEvent type, so you can distinguish this event when you recieve the event. Generic Job is
   //    the default value, and using this is covienient if you plan to just reimplement completeJob(),
   //    thus preventing casting at the target customEvent()
   class Job : public QCustomEvent
   {
   public:
      friend class ThreadWeaver;
      enum JobType { GenericJob = 3000, TagReader, CollectionReader, SearchModule };

      Job( QObject *, JobType = GenericJob );
      virtual ~Job() {}
      virtual void completeJob() {} //should be called in QObject::CustomEvent

   protected:
      Job();
      Job( const Job& );
      bool operator==( const Job& ) const;

   private:
      virtual bool doJob() = 0;
      void postJob();

      QObject *m_target;
   };

private:
   void run();

   QObject* const m_parent; //TODO can const this?
   bool           m_bool;
   QPtrList<Job>  m_Q;
   Job           *m_currentJob;

   QMutex mutex;
};


//@Will search files for the SearchBrowser
class SearchModule : public ThreadWeaver::Job
{
public:
    static const int ProgressEventType = 8889;
    class ProgressEvent : public QCustomEvent
    {
        public:
            enum State { Start = -1, Stop = -2, Progress = -3 };

            ProgressEvent( int state, KListViewItem* historyItem = 0, KListView* resultView = 0, int count = 0, QString curPath = "", QString curFile = "" )
            : QCustomEvent( ProgressEventType )
            , m_state( state )
            , m_count( count )
            , m_tresultView ( resultView )
            , m_item ( historyItem )
            , m_curPath ( curPath )
            , m_curFile ( curFile ) {}

            int state() { return m_state; }
            int count() { return m_count; }
            KListView* resultView() { return m_tresultView; }
            KListViewItem* item() { return m_item; }
            QString curPath() { return m_curPath; }
            QString curFile() { return m_curFile; }
        private:
            int m_state;
            int m_count;
            KListView* m_tresultView;
            KListViewItem* m_item;
            QString m_curPath;
            QString m_curFile;
    };

    SearchModule( QObject* parent, const QString path, const QString token, KListView* resultView, KListViewItem* historyItem );

    static void stop() { m_stop = true; }
    bool doJob();
    QStringList resultList() { return m_resultList; }
private:
    void searchDir( QString path );

    static bool m_stop;

    uint resultCount;
    QObject *m_parent;
    QString m_path;
    QString m_token;
    KListView *m_resultView;
    KListViewItem *m_historyItem;
    QStringList m_resultList;
};


//@Will read tags for the CollectionBrowser
class CollectionReader : public ThreadWeaver::Job
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

    static void stop() { m_stop = true; }
    bool doJob();

private:
    void readDir( const QString& dir, QStringList& entries );
    void readTags( const QStringList& entries );

    static bool m_stop;

    CollectionDB* m_parent;
    QObject* m_playlistBrowser;
    QStringList m_folders;
    bool m_recursively;
    bool m_importPlaylists;
    bool m_incremental;
    QStringList m_processedDirs;
};


//@Simple threaded TagWriting
class TagWriter : public ThreadWeaver::Job
{
public:
    TagWriter( QObject*, PlaylistItem*, const QString&, const int );
    bool doJob();
    void completeJob();
private:
    PlaylistItem* const m_item;
    const QString m_tagString;
    const int     m_tagType;
};


#endif
