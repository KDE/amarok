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
#include <qthread.h>      //baseclass
#include <qptrlist.h>     //stack allocated

class MetaBundle;
class PlaylistItem;
class QListView;
class QListViewItem;
class QString;
class QStringList;
class QWidget;


//@ Create a weaver wherever you need one, they can be created on the stack as they are not QObjects
//  Derive Job classes and append them to the weaver and the weaver will start the job for you
//  It can handle however many Jobs you like at two priorities (immediate and queued)
//  cancel() will stop execution immediately, with a caveat since it can not disable any dispatched events you
//  have to handle any number of events from the weaver. See PlaylistWidget::clear() for a good solution
//  halt() is permanant. Only use this to ensure the thread dies before you exit the application
//  The class is designed to be in use for the durationof the application. It will not delete jobs in the queue
//  on exit etc. As such it is incomplete, but also in this way it is lightweight and still useful.
class ThreadWeaver : public QThread
{
public:
   ThreadWeaver( QWidget* );

   class Job;

   void append( Job* const, bool = false );
   bool remove( Job* const ); //TODO implement virtual operator== ?
   void cancel();
   void halt() { m_bool = false; } //thread-safe, permanant shutdown

   enum EventType { Started = 2000, Done = 2001 };


   //@ Derive a job to perform a computationaly expensive task that will not block the UI
   //  Implement doJob() to perform the task.
   //  Specify m_target and reimplement customEvent() in that QObject to receive the results
   //  GenericJob is the default and you should reimplement completeJob() to wind-up the task
   class Job : public QCustomEvent
   {
   public:
      friend class ThreadWeaver;
      enum JobType { GenericJob = 3000, TagReader, PLStats };

      Job( QObject*, JobType = GenericJob );
      virtual ~Job() {}
      virtual void completeJob() {} //called for GenericJobs

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

   QWidget* const m_parent; //TODO can const this?
   bool           m_bool;
   QPtrList<Job>  m_Q;
   Job           *m_currentJob;

   QMutex mutex;
};


//@Will read tags for a PlaylistItem
class TagReader : public ThreadWeaver::Job
{
public:
    TagReader( QObject*, PlaylistItem* );
    ~TagReader();

    bool doJob();
    static MetaBundle* readTags( const KURL&, bool = false );

    void bindTags();
    void addSearchTokens( QStringList&, QPtrList<QListViewItem>& );

private:
    PlaylistItem* const m_item;
    const KURL  m_url;
    MetaBundle* m_tags;
};


//@Will read tags for the CollectionBrowser
class CollectionReader : public ThreadWeaver::Job
{
public:
    CollectionReader( QObject*, const KURL& url );
    ~CollectionReader();

    bool doJob();
    static MetaBundle* readTags( const KURL& );

private:
    MetaBundle* m_tags;
    const KURL  m_url;
};

static const int CollectionEventType = 8888;

class CollectionEvent : public QCustomEvent
{
public:
    CollectionEvent( MetaBundle* bundle )
        : QCustomEvent( CollectionEventType )
        , m_bundle( bundle )
        {};
    MetaBundle* bundle() { return m_bundle; }

private:
    MetaBundle* m_bundle;
};


//@Will read audioProperties for a PlaylistItem
class AudioPropertiesReader : public ThreadWeaver::Job
{
public:
    AudioPropertiesReader( QObject*, PlaylistItem* );
    bool doJob();
    void completeJob();
private:
    PlaylistItem* const m_item;
    QListView*    const m_listView;
    const KURL m_url;
    QString    m_length;
    QString    m_bitrate;
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


//@Determines statistics on a playlist for the PlaylistBrowser
class PLStats : public ThreadWeaver::Job
{
public:
    PLStats( QObject*, const KURL&, const KURL::List& );
    bool doJob();
    const KURL &url() const { return m_url; }
    const KURL::List &contents() const { return m_contents; }
    uint length() const { return m_length; }
private:
    const KURL m_url;
    const KURL::List m_contents;
    uint  m_length;
};

#endif
