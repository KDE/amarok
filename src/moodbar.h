/*
    Copyright (c) 2005 Gav Wood <gav@kde.org>
              (c) 2006 Joseph Rabinoff <bobqwatson@yahoo.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Aug 5 2006 (Joe Rabinoff): Rewrote everything.  This file bears
// no resemblance to Gav's original code.

// See moodbar.cpp for usage and implementation notes.

#ifndef MOODBAR_H
#define MOODBAR_H

#include <KUrl>

#include <q3valuelist.h>
#include <q3valuevector.h>
#include <QColor>
#include <QMutex>
#include <QObject>
#include <QPixmap>

#include "amarok_export.h"

class MetaBundle;

class AMAROK_EXPORT Moodbar : public QObject
{
#ifdef __GNUC__
#warning reenable Q_OBJECT once moodbar.cpp gets compiled again
#endif
#if 0
  Q_OBJECT
#endif

public:
  typedef Q3ValueVector<QColor> ColorList;

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

  bool dataExists( void );
  bool canHaveMood( void );
  void load( void );
  QPixmap draw( int width, int height );

  int hueSort( void ) const
  { return m_hueSort; }
  State state( void ) const
  { return m_state; }

  // Where are we storing the .mood file?
  static QString moodFilename( const KUrl &url );
  static QString moodFilename( const KUrl &url, bool withMusic );
  static bool copyFile( const QString &srcPath, const QString &dstPath );

  static bool executableExists( void );

public slots:
  void slotJobEvent( KUrl url, int newState );

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
  KUrl           m_url;         // Keep a copy of this, mainly for dtor
  mutable QMutex m_mutex;       // Locks the whole object
  int            m_hueSort;     // For PlaylistItem sorting
  State          m_state;
};


class AmarokProcess;

// For internal use only (well, mostly)
class MoodServer : public QObject
{
  Q_OBJECT

public:
  static MoodServer *instance( void );

  bool queueJob( MetaBundle *bundle );
  void deQueueJob( KUrl url );

  bool moodbarBroken( void ) const
  { return m_moodbarBroken; }

signals:
  void jobEvent( KUrl url, int newState );

private slots:
  void slotJobCompleted( int );
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
    ProcData( KUrl url, QString infile, QString outfile )
      : m_url( url ), m_infile( infile ), m_outfile( outfile )
      , m_refcount( 1 )
    {}
    ProcData( void ) {}

    KUrl    m_url;
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

  Q3ValueList<ProcData> m_jobQueue;
  bool m_moodbarBroken;
  AmarokProcess *m_currentProcess;
  ProcData m_currentData;
  mutable QMutex m_mutex;
};


#endif // MOODBAR_H
