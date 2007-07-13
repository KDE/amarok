/*************************************************************************** -*- c++ -*-
                       moodbar.h  -  description
                          -------------------
 begin                : 6th Nov 2005
 copyright            : (C) 2006 by Joseph Rabinoff
 copyright            : (C) 2005 by Gav Wood
 email                : bobqwatson@yahoo.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Aug 5 2006 (Joe Rabinoff): Rewrote everything.  This file bears 
// no resemblance to Gav's original code.

// See moodbar.cpp for usage and implementation notes.

#ifndef MOODBAR_H
#define MOODBAR_H

#include <qobject.h>
#include <qvaluevector.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <qmutex.h>
#include <qvaluelist.h>

#include <kurl.h>


class MetaBundle;

class Moodbar : public QObject
{
  Q_OBJECT
  
public:
  typedef QValueVector<QColor> ColorList;

  typedef enum
  {
      Unloaded,   // Haven't tried to load yet
      CantLoad,   // For some reason we'll never be able to load
      JobQueued,  // An analysis job is pending 
      JobRunning, // An analysis job is running
      JobFailed,  // Our job has returned and failed
      Loaded      // Can draw()
  } State;

  // These are the state changes we emit in jobEvent
  enum
  {
      JobStateRunning,
      JobStateSucceeded,
      JobStateFailed
  };
  
  // Construct an empty, small-footprint instance
  Moodbar( MetaBundle *mb ); 
  // This is for de-queueing jobs
  ~Moodbar( void );
  
  Moodbar& operator=( const Moodbar &mood );
  void reset( void );
  void detach( void );
  
  bool dataExists( void );
  bool canHaveMood( void );
  void load( void );
  QPixmap draw( int width, int height );

  int hueSort( void ) const
  { return m_hueSort; }
  State state( void ) const
  { return m_state; }
  
  // Where are we storing the .mood file?
  static QString moodFilename( const KURL &url );
  static QString moodFilename( const KURL &url, bool withMusic );
  static bool copyFile( const QString &srcPath, const QString &dstPath );

  static bool executableExists( void );
  
public slots:
  void slotJobEvent( KURL url, int newState );
  
signals:
  void jobEvent( int newState );
  
private:
  // Undefined!  We can't construct unless we know what
  // *our* parent bundle is.
  Moodbar( const Moodbar& );

  bool readFile( void );
  
  MetaBundle    *m_bundle;      // Parent bundle
  ColorList      m_data;        // .mood file contents
  QPixmap        m_pixmap;      // Cached from the last time draw() was called
  KURL           m_url;         // Keep a copy of this, mainly for dtor
  mutable QMutex m_mutex;       // Locks the whole object
  int            m_hueSort;     // For PlaylistItem sorting
  State          m_state;
};


class KProcess;

// For internal use only (well, mostly)
class MoodServer : public QObject
{
  Q_OBJECT

public:
  static MoodServer *instance( void );
  
  bool queueJob( MetaBundle *bundle );
  void deQueueJob( KURL url );

  bool moodbarBroken( void ) const
  { return m_moodbarBroken; }

signals:
  void jobEvent( KURL url, int newState );
  
private slots:
  void slotJobCompleted( KProcess *proc );
  void slotNewJob( void );
  void slotMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic );

public slots:
  // Moodbar file organization slots
  void slotFileDeleted( const QString &absPath );
  void slotFileMoved( const QString &srcPath, const QString &dstPath );

private:

  class ProcData
  {
  public:
    ProcData( KURL url, QString infile, QString outfile )
      : m_url( url ), m_infile( infile ), m_outfile( outfile )
      , m_refcount( 1 )
    {}
    ProcData( void ) : m_refcount( 0 ) {}

    KURL    m_url;
    QString m_infile;
    QString m_outfile; 
    // Keep track of how many Moodbars are waiting on this URL
    int     m_refcount;  
  };

  typedef enum
  {
    Crash       = -1,
    Success     = 0,
    NoPlugin    = 1,
    NoFile      = 2,
    CommandLine = 3
  } ReturnStatus;

  MoodServer( void );
  void setMoodbarBroken( void );
  void clearJobs( void );

  QValueList<ProcData> m_jobQueue;
  bool m_moodbarBroken;
  KProcess *m_currentProcess;
  ProcData m_currentData;
  mutable QMutex m_mutex;
};


#endif // MOODBAR_H
