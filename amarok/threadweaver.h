//
// Author: Max Howell (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef THREADWEAVER_H
#define THREADWEAVER_H

#include <kurl.h>     //stack allocated
#include <qevent.h>   //baseclass
#include <qmutex.h>   //stack allocated
#include <qthread.h>  //baseclass
#include <qptrlist.h> //stack allocated

class MetaBundle;
class PlaylistItem;
class QListView;
class QListViewItem;
class QString;
class QStringList;
class QWidget;

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


   class Job : public QCustomEvent
   {
   public:
      friend class ThreadWeaver;
      enum JobType { TagReader = 3000, AudioPropertiesReader, TagWriter, PLStats };

      Job( QObject*, JobType );
      virtual ~Job() {}

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


class AudioPropertiesReader : public ThreadWeaver::Job
{
public:
    AudioPropertiesReader( QObject*, PlaylistItem* );
    bool doJob();
    void bindTags();
private:
    PlaylistItem* const m_item;
    QListView*    const m_listView;
    const KURL m_url;
    QString    m_length;
    QString    m_bitrate;
};


class TagWriter : public ThreadWeaver::Job
{
public:
    TagWriter( QObject*, PlaylistItem*, const QString&, const int );
    bool doJob();
    void updatePlaylistItem();
private:
    PlaylistItem* const m_item;
    const QString m_tagString;
    const int     m_tagType;
};

#endif
