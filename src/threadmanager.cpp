// Author: Max Howell (C) Copyright 2004
// (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
// See COPYING file that comes with this distribution
//

// the asserts we use in this module prevent crashes, so best to abort the application if they fail
#define QT_FATAL_ASSERT
#define DEBUG_PREFIX "ThreadManager"

#include <kcursor.h>
#include <QApplication>
//Added by qt3to4:
#include <QCustomEvent>
#include <QByteArray>
#include <QEvent>

#include <pthread.h>//we're emulating features of Qt 4, so this can be removed for Amarok 2.0

#include "debug.h"
#include "statusbar.h"
#include "threadmanager.h"
#include "collectiondb.h"
#include "amarokconfig.h"

using Amarok::StatusBar;


class ThreadManager::JobCompletedEvent: public QCustomEvent
{
public:
    static const int Type = 1321;
    JobCompletedEvent( Job *j ): QCustomEvent( Type ), job( j ) { }
    Job *job;
};

ThreadManager::ThreadManager()
{
    startTimer( 5 * 60 * 1000 ); // prunes the thread pool every 5 minutes
}

ThreadManager::~ThreadManager()
{
    DEBUG_BLOCK

    for( ThreadList::Iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
    {
#ifdef HAVE_INOTIFY
        // we don't delete the thread's job as amarok is gone
        // and the Job dtor may expect amarok to be there etc.
        if ( (*it)->job() && (*it)->job()->name() == QByteArray( "INotify" ) )
        {
            debug() << "Forcibly terminating INotify thread...\n";
            (*it)->terminate();
            continue;
        }
#endif

        if( (*it)->job() && (*it)->job()->name() )
            debug() << "Waiting on thread " << (*it)->job()->name() << "...\n";
        else
            debug() << "Waiting on thread...\n";
        (*it)->wait();
    }
}

uint
ThreadManager::jobCount( const QByteArray &name )
{
    uint count = 0;

    for( JobList::Iterator it = m_jobs.begin(), end = m_jobs.end(); it != end; ++it )
        if ( name == (*it)->name() )
            count++;

    return count;
}

int
ThreadManager::queueJob( Job *job )
{
    SHOULD_BE_GUI

    if (!job)
        return -1;

    // this list contains all pending and running jobs
    m_jobs += job;

    const uint count = jobCount( job->name() );

    if ( count == 1 )
        gimmeThread()->runJob( job );

    return count;
}

int
ThreadManager::queueJobs( const JobList &jobs )
{
    SHOULD_BE_GUI

    if ( jobs.isEmpty() )
        return -1;

    m_jobs += jobs;

    const QByteArray name = jobs.front()->name();
    const uint count = jobCount( name );

    if ( count == jobs.count() )
        gimmeThread()->runJob( jobs.front() );

    return count;
}

void
ThreadManager::onlyOneJob( Job *job )
{
    SHOULD_BE_GUI

    const QByteArray name = job->name();

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
ThreadManager::abortAllJobsNamed( const QByteArray &name )
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

ThreadManager::Thread*
ThreadManager::gimmeThread()
{
    for( ThreadList::ConstIterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
        if ( !(*it)->isRunning() && (*it)->job() == 0 )
            return *it;

    Thread *thread = new Thread;
    m_threads += thread;
    return thread;
}

bool
ThreadManager::event( QEvent *e )
{
    switch( e->type() )
    {
    case JobCompletedEvent::Type: {
        Job *job = static_cast<JobCompletedEvent*>( e )->job;
        DebugStream d = debug() << "Job ";
        const QByteArray name = job->name();
        Thread *thread = job->m_thread;

        QApplication::postEvent(
                ThreadManager::instance(),
                new QCustomEvent( ThreadManager::RestoreOverrideCursorEvent ) );

        if ( !job->isAborted() ) {
            d << "completed";
            job->completeJob();
        }
        else d << "aborted";

        m_jobs.remove( job );
        delete job;

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
        QApplication::setOverrideCursor( Qt::BusyCursor );
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


/// @class ThreadManager::Thread

ThreadManager::Thread::Thread()
    : QThread()
{}

ThreadManager::Thread::~Thread()
{
    Q_ASSERT( isFinished() );
}

void
ThreadManager::Thread::runJob( Job *job )
{
    job->m_thread = this;

    if ( job->isAborted() )
        QApplication::postEvent( ThreadManager::instance(), new JobCompletedEvent( job ) );

    else {
        m_job = job;
        if( isRunning() )
            wait();
        start( Thread::IdlePriority );

        QApplication::postEvent(
                ThreadManager::instance(),
                new QCustomEvent( ThreadManager::OverrideCursorEvent ) );
    }
}

void
ThreadManager::Thread::run()
{
    // BE THREAD-SAFE!

    DEBUG_BLOCK

    //keep this first, before anything that uses the database, or SQLite may error out
    if ( AmarokConfig::databaseEngine().toInt() == DbConnection::sqlite )
        CollectionDB::instance()->releasePreviousConnection( this );

    if( m_job )
    {
        m_job->m_aborted |= !m_job->doJob();
        QApplication::postEvent( ThreadManager::instance(), new JobCompletedEvent( m_job ) );
    }

    // almost always the thread doesn't finish until after the
    // above event is already finished processing

}


/// @class ProgressEvent
/// @short Used by ThreadManager::Job internally

class ProgressEvent : public QCustomEvent {
public:
    ProgressEvent( int progress )
            : QCustomEvent( 30303 )
            , progress( progress ) {}

    const int progress;
};



/// @class ThreadManager::Job

ThreadManager::Job::Job( const char *name )
        : QCustomEvent( ThreadManager::JobEvent )
        , m_name( name )
        , m_thread( 0 )
        , m_percentDone( 0 )
        , m_progressDone( 0 )
        , m_totalSteps( 1 ) // no divide by zero
{}

ThreadManager::Job::~Job()
{
    /*if( m_thread->running() && m_thread->job() == this )
        warning() << "Deleting a job before its thread has finished with it!\n";*/
}

void
ThreadManager::Job::setProgressTotalSteps( uint steps )
{
    if ( steps == 0 ) {
        warning() << k_funcinfo << "You can't set steps to 0!\n";
        steps = 1;
    }

    m_totalSteps = steps;

    QApplication::postEvent( this, new ProgressEvent( -1 ) );
}

void
ThreadManager::Job::setProgress( uint steps )
{
    m_progressDone = steps;

    uint newPercent = uint( (100 * steps) / m_totalSteps);

    if ( newPercent != m_percentDone ) {
        m_percentDone = newPercent;
        QApplication::postEvent( this, new ProgressEvent( newPercent ) );
    }
}

void
ThreadManager::Job::setStatus( const QString &status )
{
    m_status = status;

    QApplication::postEvent( this, new ProgressEvent( -2 ) );
}

void
ThreadManager::Job::incrementProgress()
{
    setProgress( m_progressDone + 1 );
}

void
ThreadManager::Job::customEvent( QEvent *e )
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
                .setMaximum( 100 );
        break;

    default:
        StatusBar::instance()->setProgress( this, progress );
    }
}



ThreadManager::DependentJob::DependentJob( QObject *dependent, const char *name )
    : Job( name )
    , m_dependent( dependent )
{
    Q_ASSERT( dependent != this );
    connect( dependent, SIGNAL(destroyed()), SLOT(abort()) );

    QApplication::postEvent( dependent, new QCustomEvent( JobStartedEvent ) );
}

void
ThreadManager::DependentJob::completeJob()
{
    //synchronous, so we don't get deleted twice
    QApplication::sendEvent( m_dependent, this );
}

#include "threadmanager.moc"
#undef QT_FATAL_ASSERT //enable-final
