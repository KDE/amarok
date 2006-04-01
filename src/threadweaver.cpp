// Author: Max Howell (C) Copyright 2004
// (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
// See COPYING file that comes with this distribution
//

// the asserts we use in this module prevent crashes, so best to abort the application if they fail
#define QT_FATAL_ASSERT
#define DEBUG_PREFIX "ThreadWeaver"

#include <kcursor.h>
#include <qapplication.h>

#include <pthread.h>//we're emulating features of Qt 4, so this can be removed for amaroK 2.0

#include "debug.h"
#include "statusbar.h"
#include "threadweaver.h"
#include "collectiondb.h"
#include "amarokconfig.h"

using amaroK::StatusBar;

volatile uint ThreadWeaver::threadIdCounter = 1; //main thread grabs zero
QMutex* ThreadWeaver::threadIdMutex = new QMutex();

ThreadWeaver::ThreadWeaver()
{
    startTimer( 5 * 60 * 1000 ); // prunes the thread pool every 5 minutes
}

ThreadWeaver::~ThreadWeaver()
{
    Debug::Block block( __PRETTY_FUNCTION__ );

    for( ThreadList::Iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it ) {
        DebugStream d = debug() << "Waiting on thread...";
        (*it)->wait();
        d << "finished\n";
    }
}

uint
ThreadWeaver::jobCount( const QCString &name )
{
    uint count = 0;

    for( JobList::Iterator it = m_jobs.begin(), end = m_jobs.end(); it != end; ++it )
        if ( name == (*it)->name() )
            count++;

    return count;
}

int
ThreadWeaver::queueJob( Job *job )
{
    SHOULD_BE_GUI

    if ( !job )
        return -1;

    // this list contains all pending and running jobs
    m_jobs += job;

    const uint count = jobCount( job->name() );

    if ( count == 1 )
        gimmeThread()->runJob( job );

    return count;
}

int
ThreadWeaver::queueJobs( const JobList &jobs )
{
    SHOULD_BE_GUI

    if ( jobs.isEmpty() )
        return -1;

    m_jobs += jobs;

    const QCString name = jobs.front()->name();
    const uint count = jobCount( name );

    if ( count == jobs.count() )
        gimmeThread()->runJob( jobs.front() );

    return count;
}

void
ThreadWeaver::onlyOneJob( Job *job )
{
    SHOULD_BE_GUI

    const QCString name = job->name();

    // first cause all current jobs with this name to be aborted
    abortAllJobsNamed( name );

    // now queue this job.
    // if there is a running Job of its type this one will be
    // started when that one returns to the GUI thread.
    m_jobs += job;

    // if there weren't any jobs of this type running, we must
    // start this job.
    if ( jobCount( name ) == 1 )
        gimmeThread()->runJob( job );
}

int
ThreadWeaver::abortAllJobsNamed( const QCString &name )
{
    SHOULD_BE_GUI

    int count = 0;

    for( JobList::Iterator it = m_jobs.begin(), end = m_jobs.end(); it != end; ++it )
        if ( name == (*it)->name() ) {
            count++;
            (*it)->abort();
        }

    return count;
}

ThreadWeaver::Thread*
ThreadWeaver::gimmeThread()
{
    for( ThreadList::ConstIterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
        if ( !(*it)->running() && (*it)->job() == 0 )
            return *it;

    Thread *thread = new Thread;
    m_threads += thread;
    return thread;
}

bool
ThreadWeaver::event( QEvent *e )
{
    switch( e->type() )
    {
    case JobEvent: {
        Job *job = static_cast<Job*>( e );
        DebugStream d = debug() << "Job ";
        const QCString name = job->name();
        Thread *thread = job->m_thread;

        QApplication::postEvent(
                ThreadWeaver::instance(),
                new QCustomEvent( ThreadWeaver::RestoreOverrideCursorEvent ) );

        if ( !job->isAborted() ) {
            d << "completed";
            job->completeJob();
        }
        else d << "aborted";

        m_jobs.remove( job );

        d << ": " << name;
        d << ". Jobs pending: " << jobCount( name );
        d << endl;

        for( JobList::ConstIterator it = m_jobs.begin(), end = m_jobs.end(); it != end; ++it )
            if ( name == (*it)->name() ) {
                thread->runJob( (*it) );
                return true;
            }

        // this thread is done
        thread->m_job = 0;

        break;
    }

    case QEvent::Timer:
        debug() << "Threads in pool: " << m_threads.count() << endl;

//         for( ThreadList::Iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
//             if ( (*it)->readyForTrash() ) {
//                 m_threads.remove( it );
//                 delete *it;
//                 break; // only delete 1 thread every 5 minutes
//             }
        break;

    case OverrideCursorEvent:
        // we have to do this for the PlaylistLoader case, as Qt uses the same
        // function for drag and drop operations.
        QApplication::setOverrideCursor( KCursor::workingCursor() );
        break;

    case RestoreOverrideCursorEvent:
        // we have to do this for the PlaylistLoader case, as Qt uses the same
        // function for drag and drop operations.
        QApplication::restoreOverrideCursor();
        break;

    default:
        return false;
    }

    return true;
}


//Taken from Qt 4 src/corelib/thread/qthread_unix.cpp
static pthread_once_t current_thread_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t current_thread_key;
static void create_current_thread_key()
{ debug() << "Creating pthread key, exit value is " << pthread_key_create(&current_thread_key, NULL) << endl; }
/// @class ThreadWeaver::Thread

ThreadWeaver::Thread::Thread()
    : QThread()
{
}

ThreadWeaver::Thread::~Thread()
{
    Q_ASSERT( finished() );
}

QThread*
ThreadWeaver::Thread::getRunning()
{
    pthread_once( &current_thread_key_once, create_current_thread_key );
    return reinterpret_cast<QThread *>( pthread_getspecific( current_thread_key ) );
}

QString
ThreadWeaver::Thread::threadId()
{
    if( !getRunning() )
        return "None";
    else
    {
        QString s;
        return s.sprintf( "%p", getRunning() );
    }
}

void
ThreadWeaver::Thread::runJob( Job *job )
{
    job->m_thread = this;
    job->m_parentThreadId = m_threadId;

    if ( job->isAborted() )
        QApplication::postEvent( ThreadWeaver::instance(), job );

    else {
        m_job = job;
        start( Thread::IdlePriority ); //will wait() first if necessary

        QApplication::postEvent(
                ThreadWeaver::instance(),
                new QCustomEvent( ThreadWeaver::OverrideCursorEvent ) );
    }
}

void
ThreadWeaver::Thread::run()
{
    // BE THREAD-SAFE!

    DEBUG_BLOCK

    //keep this first, before anything that uses the database, or SQLite may error out
    if ( AmarokConfig::databaseEngine().toInt() == DbConnection::sqlite )
        CollectionDB::instance()->releasePreviousConnection(this);

    //register this thread so that it can be returned in a static getRunning() function
    m_threadId = ThreadWeaver::getNewThreadId();
    pthread_once(&current_thread_key_once, create_current_thread_key);
    pthread_setspecific(current_thread_key, this);

    m_job->m_aborted |= !m_job->doJob();

    if( m_job )
        QApplication::postEvent( ThreadWeaver::instance(), m_job );

    // almost always the thread doesn't finish until after the
    // above event is already finished processing

}


/// @class ProgressEvent
/// @short Used by ThreadWeaver::Job internally

class ProgressEvent : public QCustomEvent {
public:
    ProgressEvent( int progress )
            : QCustomEvent( 30303 )
            , progress( progress ) {}

    const int progress;
};



/// @class ThreadWeaver::Job

ThreadWeaver::Job::Job( const char *name )
        : QCustomEvent( ThreadWeaver::JobEvent )
        , m_name( name )
        , m_thread( 0 )
        , m_percentDone( 0 )
        , m_progressDone( 0 )
        , m_totalSteps( 1 ) // no divide by zero
{}

ThreadWeaver::Job::~Job()
{
    if( m_thread->running() && m_thread->job() == this )
        warning() << "Deleting a job before its thread has finished with it!\n";
}

void
ThreadWeaver::Job::setProgressTotalSteps( uint steps )
{
    if ( steps == 0 ) {
        warning() << k_funcinfo << "You can't set steps to 0!\n";
        steps = 1;
    }

    m_totalSteps = steps;

    QApplication::postEvent( this, new ProgressEvent( -1 ) );
}

void
ThreadWeaver::Job::setProgress( uint steps )
{
    m_progressDone = steps;

    uint newPercent = uint( (100 * steps) / m_totalSteps);

    if ( newPercent != m_percentDone ) {
        m_percentDone = newPercent;
        QApplication::postEvent( this, new ProgressEvent( newPercent ) );
    }
}

void
ThreadWeaver::Job::setStatus( const QString &status )
{
    m_status = status;

    QApplication::postEvent( this, new ProgressEvent( -2 ) );
}

void
ThreadWeaver::Job::incrementProgress()
{
    setProgress( m_progressDone + 1 );
}

void
ThreadWeaver::Job::customEvent( QCustomEvent *e )
{
    int progress = static_cast<ProgressEvent*>(e)->progress;

    switch( progress )
    {
    case -2:
        StatusBar::instance()->setProgressStatus( this, m_status );
        break;

    case -1:
        StatusBar::instance()->newProgressOperation( this )
                .setDescription( m_description )
                .setAbortSlot( this, SLOT(abort()) )
                .setTotalSteps( 100 );
        break;

    default:
        StatusBar::instance()->setProgress( this, progress );
    }
}



ThreadWeaver::DependentJob::DependentJob( QObject *dependent, const char *name )
    : Job( name )
    , m_dependent( dependent )
{
    connect( dependent, SIGNAL(destroyed()), SLOT(abort()) );

    QApplication::postEvent( dependent, new QCustomEvent( JobStartedEvent ) );
}

void
ThreadWeaver::DependentJob::completeJob()
{
    //syncronous, so we don't get deleted twice
    QApplication::sendEvent( m_dependent, this );
}

#include "threadweaver.moc"
#undef QT_FATAL_ASSERT //enable-final
