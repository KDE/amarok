// Author: Max Howell (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

// the asserts we use in this module prevent crashes, so best to abort the application if they fail
#define QT_FATAL_ASSERT
#define DEBUG_PREFIX "ThreadWeaver"

#include "debug.h"
#include <kcursor.h>
#include <qapplication.h>
#include "statusbar.h"
#include "threadweaver.h"

class ProgressEvent : public QCustomEvent {
public:
    ProgressEvent( int progress )
        : QCustomEvent( 24788 )
        , progress( progress )
    {}

    const int progress;
};

using amaroK::StatusBar;


// if you need to make more than one queue for a jobtype ask max to ammend the api
// TODO how to handle shutdown
//      detailed error handling
//      allow setting of priority?
// NOTE for stuff that must be done on job finish whatever results do in job's dtor
// TODO check QGuradedPtr is thread-safe ish
// TODO run a separate thread to wait on aborting threads


ThreadWeaver::ThreadWeaver()
{}

ThreadWeaver::~ThreadWeaver()
{
    DEBUG_FUNC_INFO

    //TODO abort and wait on all running threads
    //will dependent threads be ok here?
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
        if ( !(*it)->running() )
            return *it;

    Thread *thread = new Thread;
    m_threads += thread;
    return thread;
}

void
ThreadWeaver::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
    case DeleteThreadEvent:
        delete (Thread*)e->data();
        break;

    case JobEvent: {
        Job *job = (Job*)e;
        debugstream d = debug() << "Job ";
        const QCString name = job->name();
        Thread *thread = job->m_thread;

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
                return;
            }

        // if execution reaches here, then there are no jobs of this type left
        // to process. We need to arrange for the disposal of the thread.

        debug() << "Threads in pool: " << m_threads.count() << endl;

        break;
    }

    case OverrideCursorEvent:
        // we have to do this for the PlaylistLoader case, as Qt uses the same
        // function for drag and drop operations.
        QApplication::setOverrideCursor( KCursor::workingCursor() );
        break;

    default:
        ;
    }
}


ThreadWeaver::Thread::Thread()
    : QThread()
{
    QApplication::postEvent( ThreadWeaver::instance(), new QCustomEvent( ThreadWeaver::OverrideCursorEvent ) );
}

ThreadWeaver::Thread::~Thread()
{
    DEBUG_FUNC_INFO

    Q_ASSERT( finished() );

    QApplication::restoreOverrideCursor();
}

void
ThreadWeaver::Thread::runJob( Job *job )
{
    if ( job->isAborted() )
        QApplication::postEvent( ThreadWeaver::instance(), job );

    else {
        m_job = job;
        m_job->m_thread = this;
        start( Thread::IdlePriority ); //will wait() first if necessary
    }
}

void
ThreadWeaver::Thread::run()
{
    // BE THREAD-SAFE!

    m_job->m_aborted |= !m_job->doJob();

    QApplication::postEvent( ThreadWeaver::instance(), m_job );

    m_job = 0;
}


/// @class ThreadWeaver::Job

ThreadWeaver::Job::Job( const char *name )
    : QCustomEvent( ThreadWeaver::JobEvent )
    , m_name( name )
    , m_thread( 0 )
    , m_percentDone( 0 )
    , m_progressDone( 0 )
    , m_totalSteps( 1 ) // no divide by zero
{
    debug() << "Job created: " << QCString( name ) << endl;
}

ThreadWeaver::Job::~Job()
{}

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

    if ( newPercent > m_percentDone ) {
        m_percentDone = newPercent;
        QApplication::postEvent( this, new ProgressEvent( newPercent ) );
    }
}

void
ThreadWeaver::Job::setStatus( const QString& )
{
    AMAROK_NOTIMPLEMENTED
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
//        StatusBar::instance()->setProgressStatus( this, m_status );
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
