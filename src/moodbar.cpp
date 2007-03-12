/***************************************************************************
                        moodbar.cpp  -  description
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

// Although the current incarnation of moodbar.cpp shares bits and
// pieces of code with Gav Wood's original, it has been completely
// rewritten -- the only code I kept was purely algorithmic.  Also
// lots of Moodbar-related functionality has been moved from other
// places to here (all of it really).

// The Moodbar is used by small amounts of code in playlistitem.cpp
// and sliderwidget.cpp.  There are also trivial amounts of support
// code in other places.

// Moodbar usage
// -------------
//
// The Moodbar is part of the track's metadata, so it's held by a
// MetaBundle.  The actual Moodbar object is only used to draw a
// QPixmap, which it does efficiently -- it caches a pixmap of the
// last thing it drew, and just copies that pixmap if the dimensions
// have not changed. To use the moodbar, one just needs a few lines of
// code, such as the following, based on PrettySlider:
//
//    void MyClass::MyClass( void )
//    {
//        // This only needs to be done once!
//        connect( &m_bundle.moodbar(), SIGNAL( jobEvent( int ) ),
//                 SLOT( newMoodData( int ) ) );
//    }
//
//    void MyClass::newMetaBundle( const MetaBundle &b )
//    {
//        m_bundle = b;
//
//        if( !m_bundle.moodbar().dataExists() )
//          m_bundle.moodbar().load();
//        else
//          update();
//    }
//
//    void MyClass::draw( void )
//    {
//        QPixmap toDraw;
//        if( m_bundle.moodbar().dataExists() )
//          toDraw = m_bundle.moodbar().draw( width(), height() );
//        // else draw something else...
//    }
//
//    void MyClass::newMoodData( int newState )
//    {
//        if( newState == Moodbar::JobStateSucceeded )
//          update();
//    }
//
// Explanation:
//
//  * In the constructor we listen for the jobEvent() signal from the
//    Moodbar.  The Moodbar emits this signal when an analyzer process
//    has started or completed and it has loaded its moodbar data.
//    (This connection will exist for the lifetime of the instance of
//    MyClass and hence only needs to be created once.)
//
//  * Whenever the MetaBundle associated with this instance of MyClass
//    is changed, so does the moodbar, so we should reload it.  The
//    dataExists() method is meant to return whether the mood has
//    already been analyzed for that track (it will always return false
//    for streaming bundles and the like).  If it returns true then the
//    moodbar has already loaded its data, and can draw it.
//
//  * Otherwise we run the Moodbar's load() method.  This method may
//    be called many times; it will only actually do anything the first
//    time it's called (unless the moodbar is reset()).  Hence it's
//    totally reasonable to call load() in the draw() method too; this
//    is in fact what the PlaylistItem does.  When load() has completed,
//    it emits a jobEvent() signal.
//
//  * Note that jobEvent() will also be emitted if there is an error
//    in analyzing or loading the data, with a state indicating failure.
//    In this case, subsequent calls to dataExists() will still return
//    false, and subsequent calls to load() will do nothing.
//

// Implementation
// --------------
//
// There are two new classes, namely the Moodbar (a member of
// MetaBundle), and the MoodServer.  The former is the only public
// class.  In a nutshell, the Moodbar is responsible for reading
// and drawing mood data, and the MoodServer is in charge of
// queueing analyzer jobs and notifying interested Moodbar's when
// their job is done.


// The Moodbar class --
//
// The only public interface to the moodbar system.  An unloaded
// Moodbar is meant to have a very small footprint, since there are
// lots of MetaBundle's floating around that aren't going to be
// displayed.  Most of the data in loaded Moodbars is implicitly
// shared anyway (unless you call detach()), so it's reasonable to
// pass them around by value.
//
// Much care has been taken to absolutely minimize the amount of time
// a Moodbar is listening for a signal.  The only signal a Moodbar
// will connect to is MoodServer::jobEvent; this connection is made
// when MoodServer::queueJob() is called, and is disconnected in
// slotJobEvent().  The reason for this care is because MetaBundle's,
// and hence Moodbar's, are copied around and passed-by-value all the
// time, so I wanted to reduce overhead; also QObject::disconnect() is
// not reentrant (from what I understand), so we don't want that being
// called every time a Moodbar is destroyed!  For the same reason, the
// PlaylistItem does not listen for the jobEvent() signal; instead it
// reimplements the MetaBundle::moodbarJobEvent() virtual method.
//
// Again for this reason, the individual Moodbar's don't listen for
// the App::moodbarPrefs() signal (which is emitted every time the
// configuration is changed); thus Moodbar's aren't automatically
// updated when the AlterMood variable is changed, for instance.  This
// is a small annoyance, as the owner of the Moodbar has to listen for
// that signal and call reset().  This happens in sliderwidget.cpp and
// playlist.cpp.
//
// A moodbar is always in one of the following states:
//
//   Unloaded:   A newly-created (or newly reset()) Moodbar is in this
//               state.  The Moodbar remains in this state until
//               dataExists() or load() is called.  Note that load()
//               will return immediately unless the state is Unloaded.
//   CantLoad:   For some reason we know that we'll never be able to
//               load the Moodbar, for instance if the parent bundle
//               describes a streaming source.  Most methods will return
//               immediately in this state.
//   JobQueued:  At some point load() was called, so we queued a job with
//               the MoodServer which hasn't started yet.  In this state,
//               ~Moodbar(), reset(), etc. knows to dequeue jobs and
//               disconnect signals.
//   JobRunning: Our analyzer job is actually running.  The moodbar behaves
//               basically the same as in the JobQueued state; this state
//               exists so the PlaylistItem knows the difference.
//   JobFailed:  The MoodServer has tried to run our job (or gave up before
//               trying), and came up empty.  This state behaves basically
//               the same as CantLoad.
//   Loaded:     This is the only state in which draw() will work.
//
//
// Note that nothing is done to load until dataExists() is called; this
// is because there may very well be MetaBundle's floating around that
// aren't displayed in the GUI.
//
// Important members:
//   m_bundle: link to the parent bundle
//   m_data:   if we are loaded, this is the contents of the .mood file
//   m_pixmap: the last time draw() was called, we cached what we drew
//             here
//   m_url:    cache the URL of our queued job for de-queueing
//   m_state:  our current state
//   m_mutex:  lock for the entire object.  The Moodbar object should
//             be entirely reentrant (but see below), so most methods lock the
//             object before doing anything.  (Of course the calling code has to
//             be threadsafe for this to mean anything.)
//
// Important methods:
//
//   dataExists(): When this is called, we check if the .mood file
//       exists for our bundle.  If so, we load the corresponding file,
//       and if all goes well, return true.  If our bundle is a streaming
//       track, or is otherwise unloadable, always return false.
//
//   load(): First run readFile() to see if we can load.  If not, then
//       ask MoodServer to run a job for us.  Always changes the state
//       from Unloaded so subsequent calls to load() do nothing.
//
//   draw(): Draw the moodbar onto a QPixmap.  Cache what we drew
//       so that if draw() is called again with the same dimensions
//       we don't have to redraw.
//
//   reset(): Reset to the unloaded state.  This is basically the same
//       as calling moodbar = Moodbar().
//
//   (protected) slotJobEvent(): Only run by MoodServer, to notify us
//       when a job is started or completed.  Emits the jobEvent()
//       signal.
//
//   (private) readFile(): When we think there's a file available, this
//       method tries to load it.  We also do the display-independent
//       analysis here, namely, calculating the sorting index (for sort-
//       by-hue in the Playlist), and Making Moodier.


// The MoodServer class --
//
// This is a singleton class.  It is responsible for queueing analyzer
// jobs requested by Moodbar's, running them, and notifying the
// Moodbar's when the job has started and completed, successful or no.
// This class is also responsible for remembering if the moodbar
// system is totally broken (e.g. if the GStreamer plugins are
// missing), notifying the user if such is the case, and refusing to
// queue any more jobs.  MoodServer should be threadsafe, in that you
// should be able to run queueJob() from any thread.
//
// Jobs are referenced by URL.  If a Moodbar tries to queue a job
// with the same URL as an existing job, the job will not be re-queued;
// instead, each queued job has a refcount, which is increased.  This
// is to support the de-queueing of jobs when Moodbar's are destroyed;
// the use case I have in mind is if the user has the moodbar column
// displayed in the playlist, he/she adds 1000 tracks to the playlist
// (at which point all the displayed tracks queue moodbar jobs), and
// then decides to clear the playlist again.  The jobEvent() signal
// passes the URL of the job that was completed.
//
// The analyzer is actually run using a KProcess.  ThreadManager::Job
// is not a good solution, since we need more flexibility in the
// queuing process, and in addition, KProcess'es must be started from
// the GUI thread!
//
// Important members:
//   m_jobQueue:       this is a list of MoodServer::ProcData structures,
//                     which contain the data needed to start and reference
//                     a process, as well as a refcount.
//   m_currentProcess: the currently-running KProcess, if any.
//   m_currentData:    the ProcData structure for the currently-running
//                     process.
//   m_moodbarBroken:  this is set when there's an error running the analyzer
//                     that indicates the analyzer will never be able to run.
//                     When m_moodbarBroken == true, the MoodServer will refuse
//                     to queue new jobs.
//   m_mutex:          you should be able to run queueJob() from any thread,
//                     so most methods lock the object.
//
// Important methods:
//
//   queueJob(): Add a job to the queue.  If the job is being run, do nothing;
//       if the job is already queued, increase its refcount, and if
//       m_moodbarBroken == true, do nothing.
//
//   deQueueJob(): Called from ~Moodbar(), for instance.  Decreases
//       the refcount of a job, removing it from the queue when the
//       refcount hits zero.  This won't kill a running process.
//
//   (private slot) slotJobCompleted(): Called when a job finishes.  Do some
//       cleanup, and notify the interested parties.  Set m_moodbarBroken if
//       necessary; otherwise call slotNewJob().
//
//   (private slot) slotNewJob(): Called by slotJobCompleted() and queueJob().
//       Take a job off the queue and start the KProcess.
//
//   (private slot) slotMoodbarPrefs(): Called when the Amarok config changes.
//       If the moodbar has been disabled completely, kill the current job
//       (if any), clear the queue, and notify the interested Moodbar's.
//
//   (private slot) slotFileDeleted(): Called when a music file is deleted, so
//       we can delete the associated moodbar
//
//   (private slot) slotFileMoved(): Called when a music file is moved, so
//       we can move the associated moodbar

// TODO: off-color single bars in dark areas -- do some interpolation when
//       averaging.  Big jumps in hues when near black.
//
// BUGS:

#define DEBUG_PREFIX "Moodbar"

#include <config.h>

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "collectiondb.h"
#include "debug.h"
#include "metabundle.h"
#include "mountpointmanager.h"
#include "statusbar.h"

#include <qfile.h>
#include <qdir.h>  // For QDir::rename()
#include <qpainter.h>
#include <qtimer.h>

#include <kstandarddirs.h>

#include <string.h> // for memset()


#define CLAMP(n, v, x) ((v) < (n) ? (n) : (v) > (x) ? (x) : (v))

#define WEBPAGE "http://amarok.kde.org/wiki/Moodbar"


///////////////////////////////////////////////////////////////////////////////
// MoodServer class
///////////////////////////////////////////////////////////////////////////////


MoodServer *
MoodServer::instance( void )
{
  static MoodServer m;
  return &m;
}


MoodServer::MoodServer( void )
    : m_moodbarBroken( false )
    , m_currentProcess( 0 )
{
    connect( App::instance(), SIGNAL( moodbarPrefs( bool, bool, int, bool ) ),
             SLOT( slotMoodbarPrefs( bool, bool, int, bool ) ) );
    connect( CollectionDB::instance(),
             SIGNAL( fileMoved( const QString &, const QString & ) ),
             SLOT( slotFileMoved( const QString &, const QString & ) ) );
    connect( CollectionDB::instance(),
             SIGNAL( fileMoved( const QString &, const QString &, const QString & ) ),
             SLOT( slotFileMoved( const QString &, const QString & ) ) );
    connect( CollectionDB::instance(),
             SIGNAL( fileDeleted( const QString & ) ),
             SLOT( slotFileDeleted( const QString & ) ) );
    connect( CollectionDB::instance(),
             SIGNAL( fileDeleted( const QString &, const QString & ) ),
             SLOT( slotFileDeleted( const QString & ) ) );
}


// Queue a job, but not before checking if the moodbar is enabled
// in the config, if the moodbar analyzer appears to be working,
// and if a job for that URL isn't already queued.  Returns true
// if the job is already running, false otherwise.
bool
MoodServer::queueJob( MetaBundle *bundle )
{
    if( m_moodbarBroken  ||  !AmarokConfig::showMoodbar() )
      return false;

    m_mutex.lock();

    // Check if the currently running job is for that URL
    if( m_currentProcess != 0  &&
        m_currentData.m_url == bundle->url() )
      {
        debug() << "MoodServer::queueJob: Not re-queueing already-running job "
                << bundle->url().path() << endl;
        m_mutex.unlock();
        return true;
      }

    // Check if there's already a job in the queue for that URL
    QValueList<ProcData>::iterator it;
    for( it = m_jobQueue.begin(); it != m_jobQueue.end(); ++it )
      {
        if( (*it).m_url == bundle->url() )
          {
            (*it).m_refcount++;
            debug() << "MoodServer::queueJob: Job for " << bundle->url().path()
                    << " already in queue, increasing refcount to "
                    << (*it).m_refcount << endl;
            m_mutex.unlock();
            return false;
          }
      }

    m_jobQueue.append( ProcData( bundle->url(),
                                 bundle->url().path(),
                                 bundle->moodbar().moodFilename( bundle->url() ) ) );

    debug() << "MoodServer::queueJob: Queued job for " << bundle->url().path()
            << ", " << m_jobQueue.size() << " jobs in queue." << endl;

    m_mutex.unlock();

    // New jobs *must* be started from the GUI thread!
    QTimer::singleShot( 1000, this, SLOT( slotNewJob( void ) ) );

    return false;
}


// Decrements the refcount of the job for the given URL
// and deletes that job if necessary.
void
MoodServer::deQueueJob( KURL url )
{
    m_mutex.lock();

    // Can't de-queue running jobs
    if( m_currentProcess != 0  &&
        m_currentData.m_url == url )
      {
        debug() << "MoodServer::deQueueJob: Not de-queueing already-running job "
                << url.path() << endl;
        m_mutex.unlock();
        return;
      }

    // Check if there's already a job in the queue for that URL
    QValueList<ProcData>::iterator it;
    for( it = m_jobQueue.begin(); it != m_jobQueue.end(); ++it )
      {
        if( (*it).m_url == url )
          {
            (*it).m_refcount--;

            if( (*it).m_refcount == 0 )
              {
                debug() << "MoodServer::deQueueJob: nobody cares about "
                        << (*it).m_url.path()
                        << " anymore, deleting from queue" << endl;
                m_jobQueue.erase( it );
              }

            else
              debug() << "MoodServer::deQueueJob: decrementing refcount of "
                      << (*it).m_url.path() << " to " << (*it).m_refcount
                      << endl;

            m_mutex.unlock();
            return;
          }
      }

    debug() << "MoodServer::deQueueJob: tried to delete nonexistent job "
            << url.path() << endl;

    m_mutex.unlock();
}


// This slot exists so that jobs can be started from the GUI thread,
// just in case queueJob() is run from another thread.  Only run
// directly if you're in the GUI thread!
void
MoodServer::slotNewJob( void )
{
  if( m_moodbarBroken )
    return;

  m_mutex.lock();

  // Are we already running a process?
  if( m_jobQueue.isEmpty()  ||  m_currentProcess != 0 )
    {
      m_mutex.unlock();
      return;
    }

  m_currentData = m_jobQueue.first();
  m_jobQueue.pop_front();

  debug() << "MoodServer::slotNewJob: starting new analyzer process: "
          << "moodbar -o " << m_currentData.m_outfile << ".tmp "
          << m_currentData.m_infile << endl;
  debug() << "MoodServer::slotNewJob: " << m_jobQueue.size()
          << " jobs left in queue." << endl;


  // Write to outfile.mood.tmp so that new Moodbar instances
  // don't think the mood data exists while the analyzer is
  // running.  Then rename the file later.
  m_currentProcess = new Amarok::Process( this );
  m_currentProcess->setPriority( 18 );  // Nice the process
  *m_currentProcess << KStandardDirs::findExe( "moodbar" ) << "-o"
                    << (m_currentData.m_outfile + ".tmp")
                    << m_currentData.m_infile;

  connect( m_currentProcess, SIGNAL( processExited( KProcess* ) ),
           SLOT( slotJobCompleted( KProcess* ) ) );

  // We have to enable KProcess::Stdout (even though we don't monitor
  // it) since otherwise the child process crashes every time in
  // KProcess::start() (but only when started from the loader!).  I
  // have no idea why, but I imagine it's a bug in KDE.
  if( !m_currentProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
    {
      // If we have an error starting the process, it's never
      // going to work, so call moodbarBroken()
      warning() << "Can't start moodbar analyzer process!" << endl;
      delete m_currentProcess;
      m_currentProcess = 0;
      m_mutex.unlock();
      setMoodbarBroken();
      return;
    }

  // Extreme reentrancy pedatry :)
  KURL url = m_currentData.m_url;
  m_mutex.unlock();

  emit jobEvent( url, Moodbar::JobStateRunning );
}


// This always run in the GUI thread.  It is called
// when an analyzer process terminates
void
MoodServer::slotJobCompleted( KProcess *proc )
{
    m_mutex.lock();

    // Pedantry
    if( proc != m_currentProcess )
      warning() << "MoodServer::slotJobCompleted: proc != m_currentProcess!" << endl;

    ReturnStatus returnval;
    if( !m_currentProcess->normalExit() )
      returnval = Crash;
    else
      returnval = (ReturnStatus) m_currentProcess->exitStatus();

    bool success = (returnval == Success);
    KURL url = m_currentData.m_url;

    if( success )
      {
        QString file = m_currentData.m_outfile;
        QString dir = file.left( file.findRev( '/' ) );
        file = file.right( file.length() - file.findRev( '/' ) - 1 );
        QDir( dir ).rename( file + ".tmp", file );
      }
    else
      QFile::remove( m_currentData.m_outfile + ".tmp" );

    delete m_currentProcess;
    m_currentProcess = 0;


    // If the moodbar was disabled, we killed the process
    if( !AmarokConfig::showMoodbar() )
      {
        debug() << "MoodServer::slotJobCompleted: moodbar disabled, job killed" << endl;
        m_mutex.unlock();
        emit jobEvent( url, Moodbar::JobStateFailed );
        return;
      }


    switch( returnval )
      {
      case Success:
        debug() << "MoodServer::slotJobCompleted: job completed successfully" << endl;
        m_mutex.unlock();
        slotNewJob();
        break;

        // Crash and NoFile don't mean that moodbar is broken.
        // Something bad happened, but it's probably a problem with this file
        // Just log an error message and emit jobEvent().
      case Crash:
        debug() << "MoodServer::slotJobCompleted: moodbar crashed on "
                << m_currentData.m_infile << endl;
        m_mutex.unlock();
        slotNewJob();
        break;

      case NoFile:
        debug() << "MoodServer::slotJobCompleted: moodbar had a problem with "
                << m_currentData.m_infile << endl;
        m_mutex.unlock();
        slotNewJob();
        break;

        // NoPlugin and CommandLine mean the moodbar is broken
        // The moodbar analyzer is not likely to work ever, so let the
        // user know about it and disable new jobs.
      default:
        m_mutex.unlock();
        setMoodbarBroken();
        break;

      }

    emit jobEvent( url, success ? Moodbar::JobStateSucceeded
                                : Moodbar::JobStateFailed );
}


// This is called whenever "Ok" or "Apply" is pressed on the configuration
// dialog.  If the moodbar is disabled, kill the current process and
// clear the queue
void
MoodServer::slotMoodbarPrefs( bool show, bool moodier, int alter, bool withMusic )
{
    if( show == true)
      return;

    (void) moodier;  (void) alter;  (void) withMusic;

    // If we have a current process, kill it.  Cleanup happens in
    // slotJobCompleted() above.  We do *not* want to lock the
    // mutex when calling this!
    if( m_currentProcess != 0 )
      m_currentProcess->kill();

    clearJobs();
}


// When a file is deleted, either manually using Organize Collection or
// automatically detected using AFT, delete the corresponding mood file.
void
MoodServer::slotFileDeleted( const QString &path )
{
    QString mood = Moodbar::moodFilename( KURL::fromPathOrURL( path ) );
    if( mood.isEmpty()  ||  !QFile::exists( mood ) )
      return;

    debug() << "MoodServer::slotFileDeleted: deleting " << mood << endl;
    QFile::remove( mood );
}


// When a file is moved, either manually using Organize Collection or
// automatically using AFT, move the corresponding mood file.
void
MoodServer::slotFileMoved( const QString &srcPath, const QString &dstPath )
{
    QString srcMood = Moodbar::moodFilename( KURL::fromPathOrURL( srcPath ) );
    QString dstMood = Moodbar::moodFilename( KURL::fromPathOrURL( dstPath ) );

    if( srcMood.isEmpty()   ||  dstMood.isEmpty()  ||
        srcMood == dstMood  ||  !QFile::exists( srcMood ) )
      return;

    debug() << "MoodServer::slotFileMoved: moving " << srcMood << " to "
            << dstMood << endl;

    Moodbar::copyFile( srcMood, dstMood );
    QFile::remove( srcMood );
}


// This is called when we decide that the moodbar analyzer is
// never going to work.  Disable further jobs, and let the user
// know about it.  This should only be called when m_currentProcess == 0.
void
MoodServer::setMoodbarBroken( void )
{
    warning() << "Uh oh, it looks like the moodbar analyzer is not going to work"
              << endl;

    Amarok::StatusBar::instance()->longMessage( i18n(
        "The Amarok moodbar analyzer program seems to be broken. "
        "This is probably because the moodbar package is not installed "
        "correctly.  The moodbar package, installation instructions, and "
        "troubleshooting help can be found on the wiki page at <a href='"
        WEBPAGE "'>" WEBPAGE "</a>. "
        "When the problem is fixed, please restart Amarok."),
        KDE::StatusBar::Error );


    m_moodbarBroken = true;
    clearJobs();
}


// Clear the job list and emit signals
void
MoodServer::clearJobs( void )
{
    // We don't want to emit jobEvent (or really do anything
    // external) while the mutex is locked.
    m_mutex.lock();
    QValueList<ProcData> queueCopy
      = QDeepCopy< QValueList<ProcData> > ( m_jobQueue );
    m_jobQueue.clear();
    m_mutex.unlock();

    QValueList<ProcData>::iterator it;
    for( it = queueCopy.begin(); it != queueCopy.end(); ++it )
      emit jobEvent( (*it).m_url, Moodbar::JobStateFailed );
}



///////////////////////////////////////////////////////////////////////////////
// Moodbar class
///////////////////////////////////////////////////////////////////////////////


// The moodbar behavior is nearly identical in the JobQueued and
// JobRunning states, but we have to keep track anyway so the
// PlaylistItem knows what do display

#define JOB_PENDING(state) ((state)==JobQueued||(state)==JobRunning)


// The passed MetaBundle _must_ be non-NULL, and the pointer must be valid
// as long as this instance is alive.  The Moodbar is only meant to be a
// member of a MetaBundle, in other words.

Moodbar::Moodbar( MetaBundle *mb )
    : QObject   ( )
    , m_bundle  ( mb )
    , m_hueSort ( 0 )
    , m_state   ( Unloaded )
{
}


// If we have any pending jobs, de-queue them.  The use case I
// have in mind is if the user has the moodbar column displayed
// and adds all his/her tracks to the playlist, then deletes
// them again.
Moodbar::~Moodbar( void )
{
  if( JOB_PENDING( m_state ) )
    MoodServer::instance()->deQueueJob( m_url );
}


// MetaBundle's are often assigned using operator=, so so are we.
Moodbar&
Moodbar::operator=( const Moodbar &mood )
{
    // Need to check this before locking both!
    if( &mood == this )
      return *this;

    m_mutex.lock();
    mood.m_mutex.lock();

    State oldState = m_state;
    KURL oldURL    = m_url;

    m_data    = mood.m_data;
    m_pixmap  = mood.m_pixmap;
    m_state   = mood.m_state;
    m_url     = mood.m_url;
    // DO NOT overwrite m_bundle!  That should never change.

    // Signal connections and job queues are part of our "state",
    // so those should be updated too.
    if( JOB_PENDING( m_state )  &&  !JOB_PENDING( oldState ) )
      {
        connect( MoodServer::instance(),
                 SIGNAL( jobEvent( KURL, int ) ),
                 SLOT( slotJobEvent( KURL, int ) ) );
        // Increase the refcount for this job.  Use mood.m_bundle
        // since that one's already initialized.
        MoodServer::instance()->queueJob( mood.m_bundle );
      }

    // If we had a job pending, de-queue it
    if( !JOB_PENDING( m_state )  &&  JOB_PENDING( oldState ) )
      {
        MoodServer::instance()->disconnect( this, SLOT( slotJobEvent( KURL, int ) ) );
        MoodServer::instance()->deQueueJob( oldURL );
      }

    mood.m_mutex.unlock();
    m_mutex.unlock();

    return *this;
}


// Reset the moodbar to its Unloaded state.  This is useful when
// the configuration is changed, and all the moodbars need to be
// reloaded.
void
Moodbar::reset( void )
{
  m_mutex.lock();

  debug() << "Resetting moodbar: " << m_bundle->url().path() << endl;

  if( JOB_PENDING( m_state ) )
    {
      MoodServer::instance()->disconnect( this, SLOT( slotJobEvent( KURL, int ) ) );
      MoodServer::instance()->deQueueJob( m_url );
    }

  m_data.clear();
  m_pixmap  = QPixmap();
  m_url     = KURL();
  m_hueSort = 0;
  m_state   = Unloaded;

  m_mutex.unlock();
}


// Make a copy of all of our implicitly shared data
void
Moodbar::detach( void )
{
  m_mutex.lock();

  m_data = QDeepCopy<ColorList>(m_data);
  m_pixmap.detach();

  // Apparently this is the wrong hack -- don't detach urls
  //QString url( QDeepCopy<QString>( m_url.url() ) );
  //m_url = KURL::fromPathOrURL( url );

  m_mutex.unlock();
}


// If possible, try to open the bundle's .mood file.  When this method
// returns true, this instance must be able to draw().  This may
// change the state to CantLoad, but usually leaves the state
// untouched.
bool
Moodbar::dataExists( void )
{
    // Put this first for efficiency
    if( m_state == Loaded )
      return true;

    // Should we bother checking for the file?
    if( m_state == CantLoad    ||
        JOB_PENDING( m_state ) ||
        m_state == JobFailed   ||
        !canHaveMood() )
      return false;

    m_mutex.lock();
    bool res = readFile();
    m_mutex.unlock();

    return res;
}


// If m_bundle is not a local file or for some other reason cannot
// have mood data, return false, and set the state to CantLoad to
// save future checks.  Note that MoodServer::m_moodbarBroken == true
// does not mean we can't have a mood file; it just means that we
// can't generate new ones.
bool
Moodbar::canHaveMood( void )
{
    if( m_state == CantLoad )
      return false;

    // Don't try to analyze it if we can't even determine it has a length
    // If for some reason we can't determine a file name, give up
    // If the moodbar is disabled, set to CantLoad -- if the user re-enables
    // the moodbar, we'll be reset() anyway.
    if( !AmarokConfig::showMoodbar()   ||
        !m_bundle->url().isLocalFile() ||
        !m_bundle->length()            ||
        moodFilename( m_bundle->url() ).isEmpty() )
      {
        m_state = CantLoad;
        return false;
      }

    return true;
}


// Ask MoodServer to queue an analyzer job for us if necessary.  This
// method will only do something the first time it's called, as it's
// guaranteed to change the state from Unloaded.
void
Moodbar::load( void )
{
    if( m_state != Unloaded )
      return;

    m_mutex.lock();

    if( !canHaveMood() )
      {
        // State is now CantLoad
        m_mutex.unlock();
        return;
      }

    if( readFile() )
      {
        // State is now Loaded
        m_mutex.unlock();
        return;
      }

    if( MoodServer::instance()->moodbarBroken() )
      {
        m_state = JobFailed;
        m_mutex.unlock();
        return;
      }

    // Ok no more excuses, we have to queue a job
    connect( MoodServer::instance(),
             SIGNAL( jobEvent( KURL, int ) ),
             SLOT( slotJobEvent( KURL, int ) ) );
    bool isRunning = MoodServer::instance()->queueJob( m_bundle );
    m_state = isRunning ? JobRunning : JobQueued;
    m_url = m_bundle->url();  // Use this URL for MoodServer::deQueueJob

    m_mutex.unlock();
}


// This is called by MoodServer when our moodbar analyzer job starts
// or finishes.  It may change the state from JobQueued / JobRunning
// to JobRunning, Loaded, or JobFailed.  It may emit a jobEvent()
void
Moodbar::slotJobEvent( KURL url, int newState )
{
    // Is this job for us?
    if( !JOB_PENDING( m_state )  ||  url != m_bundle->url() )
      return;

    bool success = ( newState == JobStateSucceeded );

    // We don't really care about this, but our listeners might
    if( newState == JobStateRunning )
      {
        m_state = JobRunning;
        goto out;
      }

    m_mutex.lock();

    // Disconnect the signal for efficiency's sake
    MoodServer::instance()->disconnect( this, SLOT( slotJobEvent( KURL, int ) ) );

    if( !success )
      {
        m_state = JobFailed;
        m_mutex.unlock();
        goto out;
      }

    if( readFile() )
      {
        // m_state is now Loaded
        m_mutex.unlock();
        goto out;
      }

    // If we get here it means the analyzer job went wrong, but
    // somehow the MoodServer didn't know about it
    debug() << "WARNING: Failed to open file " << moodFilename( m_bundle->url() )
            << " -- something is very wrong" << endl;
    m_state = JobFailed;
    m_mutex.unlock();

 out:
    emit jobEvent( newState );
    // This is a cheat for PlaylistItem so it doesn't have to
    // use signals
    m_bundle->moodbarJobEvent( newState );
}


// Draw the moodbar onto a pixmap of the given dimensions and return
// it.  This is mostly Gav's original code, cut and pasted from
// various places.  This will not change the state.
QPixmap
Moodbar::draw( int width, int height )
{
    if( m_state != Loaded  ||  !AmarokConfig::showMoodbar() )  // Naughty caller!
      return QPixmap();

    m_mutex.lock();

    // Do we have to repaint, or can we use the cache?
    if( m_pixmap.width() == width  &&  m_pixmap.height() == height )
      {
        m_mutex.unlock();
        return m_pixmap;
      }

    m_pixmap = QPixmap( width, height );
    QPainter paint( &m_pixmap );

    // First average the moodbar samples that will go into each
    // vertical bar on the screen.

    if( m_data.size() == 0 ) // Play it safe -- see below
      return QPixmap();

    ColorList screenColors;
    QColor bar;
    float r, g, b;
    int h, s, v;

    for( int i = 0; i < width; i++ )
      {
        r = 0.f;  g = 0.f;  b = 0.f;

        // m_data.size() needs to be at least 1 for this not to crash!
        uint start = i * m_data.size() / width;
        uint end   = (i + 1) * m_data.size() / width;
        if( start == end )
          end = start + 1;

        for( uint j = start; j < end; j++ )
          {
            r += m_data[j].red();
            g += m_data[j].green();
            b += m_data[j].blue();
          }

        uint n = end - start;
        bar =  QColor( int( r / float( n ) ),
                       int( g / float( n ) ),
                       int( b / float( n ) ), QColor::Rgb );

        /* Snap to the HSV values for later */
        bar.getHsv(&h, &s, &v);
        bar.setHsv(h, s, v);

        screenColors.push_back( bar );
      }

    // Paint the bars.  This is Gav's painting code -- it breaks up the
    // monotony of solid-color vertical bars by playing with the saturation
    // and value.

    for( int x = 0; x < width; x++ )
      {
        screenColors[x].getHsv( &h, &s, &v );

        for( int y = 0; y <= height / 2; y++ )
          {
            float coeff = float(y) / float(height / 2);
            float coeff2 = 1.f - ((1.f - coeff) * (1.f - coeff));
            coeff = 1.f - (1.f - coeff) / 2.f;
            coeff2 = 1.f - (1.f - coeff2) / 2.f;
            paint.setPen( QColor( h,
                CLAMP( 0, int( float( s ) * coeff ), 255 ),
                CLAMP( 0, int( 255.f - (255.f - float( v )) * coeff2), 255 ),
                QColor::Hsv ) );
            paint.drawPoint(x, y);
            paint.drawPoint(x, height - 1 - y);
          }
      }

    m_mutex.unlock();

    return m_pixmap;
}


#define NUM_HUES 12

// Read the .mood file.  Returns true if the read was successful
// and changes the state to Loaded; returns false and leaves the
// state untouched otherwise.
//
// This is based on Gav's original code.  We do the mood altering
// (AmarokConfig::AlterMood()) here, as well as calculating the
// hue-based sort.  All displayed moodbars will be reset() when
// the config is changed, so there's no harm in doing it here.
//
// This method must be called with the instance locked.
bool
Moodbar::readFile( void )
{
    if( !AmarokConfig::showMoodbar() )
      return false;

    if( m_state == Loaded )
      return true;

    QString path = moodFilename( m_bundle->url() );
    if( path.isEmpty() )
      return false;

    debug() << "Moodbar::readFile: Trying to read " << path << endl;

    QFile moodFile( path );

    if( !QFile::exists( path )  ||
        !moodFile.open( IO_ReadOnly ) )
      {
        // If the user has changed his/her preference about where to
        // store the mood files, he/she might have the .mood file
        // in the other place, so we should check there before giving
        // up.

        QString path2 = moodFilename( m_bundle->url(),
                                      !AmarokConfig::moodsWithMusic() );
        moodFile.setName( path2 );

        if( !QFile::exists( path2 )  ||
            !moodFile.open( IO_ReadOnly ) )
          return false;

        debug() << "Moodbar::readFile: Found a file at " << path2
                << " instead, using that and copying." << endl;

        moodFile.close();
        if( !copyFile( path2, path ) )
          return false;
        moodFile.setName( path );
        if( !moodFile.open( IO_ReadOnly ) )
          return false;
      }

    int r, g, b, samples = moodFile.size() / 3;
    debug() << "Moodbar::readFile: File " << path
            << " opened. Proceeding to read contents... s=" << samples << endl;

    // This would be bad.
    if( samples == 0 )
      {
        debug() << "Moodbar::readFile: File " << moodFile.name()
                << " is corrupted, removing." << endl;
        moodFile.remove();
        return false;
      }

    int huedist[360], mx = 0; // For alterMood
    int modalHue[NUM_HUES];   // For m_hueSort
    int h, s, v;

    memset( modalHue, 0, sizeof( modalHue ) );
    memset( huedist, 0, sizeof( huedist ) );

    // Read the file, keeping track of some histograms
    for( int i = 0; i < samples; i++ )
      {
        r = moodFile.getch();
        g = moodFile.getch();
        b = moodFile.getch();

        m_data.push_back( QColor( CLAMP( 0, r, 255 ),
                                  CLAMP( 0, g, 255 ),
                                  CLAMP( 0, b, 255 ), QColor::Rgb ) );

        // Make a histogram of hues
        m_data.last().getHsv( &h, &s, &v );
        modalHue[CLAMP( 0, h * NUM_HUES / 360, NUM_HUES - 1 )] += v;

        if( h < 0 ) h = 0;  else h = h % 360;
        huedist[h]++;
      }

    // Make moodier -- copied straight from Gav Wood's code
    // Here's an explanation of the algorithm:
    //
    // The "input" hue for each bar is mapped to a hue between
    // rangeStart and (rangeStart + rangeDelta).  The mapping is
    // determined by the hue histogram, huedist[], which is calculated
    // above by putting each sample into one of 360 hue bins.  The
    // mapping is such that if your histogram is concentrated on a few
    // hues that are close together, then these hues are separated,
    // and the space between spikes in the hue histogram is
    // compressed.  Here we consider a hue value to be a "spike" in
    // the hue histogram if the number of samples in that bin is
    // greater than the threshold variable.
    //
    // As an example, suppose we have 100 samples, and that
    //    threshold = 10  rangeStart = 0  rangeDelta = 288
    // Suppose that we have 10 samples at each of 99,100,101, and 200.
    // Suppose that there are 20 samples < 99, 20 between 102 and 199,
    // and 20 above 201, with no spikes.  There will be five hues in
    // the output, at hues 0, 72, 144, 216, and 288, containing the
    // following number of samples:
    //     0:   20 + 10 = 30   (range 0   - 99 )
    //     72:            10   (range 100 - 100)
    //     144:           10   (range 101 - 101)
    //     216: 10 + 20 = 30   (range 102 - 200)
    //     288:           20   (range 201 - 359)
    // The hues are now much more evenly distributed.
    //
    // After the hue redistribution is calculated, the saturation and
    // value are scaled by sat and val, respectively, which are percentage
    // values.

    if( AmarokConfig::makeMoodier() )
      {
        // Explanation of the parameters:
        //
        //   threshold: A hue value is considered to be a "spike" in the
        //     histogram if it's above this value.  Setting this value
        //     higher will tend to make the hue distribution more uniform
        //
        //   rangeStart, rangeDelta: output hues will be more or less
        //     evenly spaced between rangeStart and (rangeStart + rangeDelta)
        //
        //   sat, val: the saturation and value are scaled by these integral
        //     percentage values

        int threshold, rangeStart, rangeDelta, sat, val;
        int total = 0;
        memset( modalHue, 0, sizeof( modalHue ) );  // Recalculate this

        switch( AmarokConfig::alterMood() )
          {
          case 1: // Angry
            threshold  = samples / 360 * 9;
            rangeStart = 45;
            rangeDelta = -45;
            sat        = 200;
            val        = 100;
            break;

          case 2: // Frozen
            threshold  = samples / 360 * 1;
            rangeStart = 140;
            rangeDelta = 160;
            sat        = 50;
            val        = 100;
            break;

          default: // Happy
            threshold  = samples / 360 * 2;
            rangeStart = 0;
            rangeDelta = 359;
            sat        = 150;
            val        = 250;
          }

        debug() << "ReadMood: Applying filter t=" << threshold
                << ", rS=" << rangeStart << ", rD=" << rangeDelta
                << ", s=" << sat << "%, v=" << val << "%" << endl;

        // On average, huedist[i] = samples / 360.  This counts the
        // number of samples over the threshold, which is usually
        // 1, 2, 9, etc. times the average samples in each bin.
        // The total determines how many output hues there are,
        // evenly spaced between rangeStart and rangeStart + rangeDelta.
        for( int i = 0; i < 360; i++ )
          if( huedist[i] > threshold )
            total++;

        if( total < 360 && total > 0 )
          {
            // Remap the hue values to be between rangeStart and
            // rangeStart + rangeDelta.  Every time we see an input hue
            // above the threshold, increment the output hue by
            // (1/total) * rangeDelta.
            for( int i = 0, n = 0; i < 360; i++ )
              huedist[i] = ( ( huedist[i] > threshold ? n++ : n )
                             * rangeDelta / total + rangeStart ) % 360;

            // Now huedist is a hue mapper: huedist[h] is the new hue value
            // for a bar with hue h

            for(uint i = 0; i < m_data.size(); i++)
              {
                m_data[i].getHsv( &h, &s, &v );
                if( h < 0 ) h = 0;  else h = h % 360;
                m_data[i].setHsv( CLAMP( 0, huedist[h], 359 ),
                                  CLAMP( 0, s * sat / 100, 255 ),
                                  CLAMP( 0, v * val / 100, 255 ) );

                modalHue[CLAMP(0, huedist[h] * NUM_HUES / 360, NUM_HUES - 1)]
                  += (v * val / 100);
              }
          }
      }

    // Calculate m_hueSort.  This is a 3-digit number in base NUM_HUES,
    // where the most significant digit is the first strongest hue, the
    // second digit is the second strongest hue, and the third digit
    // is the third strongest.  This code was written by Gav Wood.

    m_hueSort = 0;
    mx = 0;
    for( int i = 1; i < NUM_HUES; i++ )
      if( modalHue[i] > modalHue[mx] )
        mx = i;
    m_hueSort = mx * NUM_HUES * NUM_HUES;
    modalHue[mx] = 0;

    mx = 0;
    for( int i = 1; i < NUM_HUES; i++ )
      if( modalHue[i] > modalHue[mx] )
        mx = i;
    m_hueSort += mx * NUM_HUES;
    modalHue[mx] = 0;

    mx = 0;
    for( int i = 1; i < NUM_HUES; i++ )
      if( modalHue[i] > modalHue[mx] )
        mx = i;
    m_hueSort += mx;


    debug() << "Moodbar::readFile: All done." << endl;

    moodFile.close();
    m_state = Loaded;

    return true;
}


// Returns where the mood file for this bundle should be located,
// based on the user preferences.  If no location can be determined,
// return QString::null.

QString
Moodbar::moodFilename( const KURL &url )
{
  return moodFilename( url, AmarokConfig::moodsWithMusic() );
}

QString
Moodbar::moodFilename( const KURL &url, bool withMusic )
{
    // No need to lock the object

    QString path;

    if( withMusic )
      {
        path = url.path();
        path.truncate(path.findRev('.'));

        if (path.isEmpty())  // Weird...
          return QString();

        path += ".mood";
        int slash = path.findRev('/') + 1;
        QString dir  = path.left(slash);
        QString file = path.right(path.length() - slash);
        path = dir + '.' + file;
      }

    else
      {
        // The moodbar file is {device id},{relative path}.mood}
        int deviceid = MountPointManager::instance()->getIdForUrl( url );
        KURL relativePath;
        MountPointManager::instance()->getRelativePath( deviceid,
            url, relativePath );
        path = relativePath.path();
        path.truncate(path.findRev('.'));

        if (path.isEmpty())  // Weird...
          return QString();

        path = QString::number( deviceid ) + ','
          + path.replace('/', ',') + ".mood";

        // Creates the path if necessary
        path = ::locateLocal( "data", "amarok/moods/" + path );
      }

    return path;
}


// Quick-n-dirty -->synchronous<-- file copy (the GUI needs its
// moodbars immediately!)
bool
Moodbar::copyFile( const QString &srcPath, const QString &dstPath )
{
  QFile file( srcPath );
  if( !file.open( IO_ReadOnly ) )
    return false;
  QByteArray contents = file.readAll();
  file.close();
  file.setName( dstPath );
  if( !file.open( IO_WriteOnly | IO_Truncate ) )
    return false;
  bool res = ( uint( file.writeBlock( contents ) ) == contents.size() );
  file.close();
  return res;
}



// Can we find the moodbar program?
bool
Moodbar::executableExists( void )
{
  return !(KStandardDirs::findExe( "moodbar" ).isNull());
}


#include "moodbar.moc"

