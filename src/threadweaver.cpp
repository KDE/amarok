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

    int progress;
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

inline ThreadWeaver::Thread*
ThreadWeaver::findThread( const QCString &name )
{
    for( ThreadList::ConstIterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
        if ( name == (*it)->name ) //is a string comparison, not a pointer comparison
            return (*it);

    return 0;
}

int
ThreadWeaver::queueJob( Job *job )
{
    const QCString name = job->name();
    Thread *thread = findThread( name );

    if ( !thread ) {
        thread = new Thread( job->name() );
        m_threads += thread;
    }

    thread->runJob( job );

    return thread->pendingJobs.count();
}

int
ThreadWeaver::queueJobs( const JobList &jobs )
{
    if ( jobs.isEmpty() )
        return -1;

    const char *name = jobs.front()->name();
    Thread *thread = findThread( name );
    if ( !thread ) {
        thread = new Thread( name );
        m_threads += thread;
    }

    thread->pendingJobs += jobs;
    if ( !thread->running() )
        thread->runJob( 0 ); //FIXME currently not wise to call runJob( 0 ) when running

    //you can't use this yet!
    return thread->jobCount();
}

void
ThreadWeaver::onlyOneJob( Job *job )
{
    const QCString name = job->name();
    Thread *thread = findThread( name );

    if ( thread )
        thread->abortAllJobs();
    else {
        thread = new Thread( job->name() );
        m_threads += thread;
    }

    thread->runJob( job );
}

int
ThreadWeaver::abortAllJobsNamed( const QCString &name )
{
    int count = -1;
    Thread *thread = findThread( name );

    if ( thread ) {
        count = thread->jobCount();
        dispose( thread );
    }

    return count;
}

bool
ThreadWeaver::isJobPending( const QCString &name )
{
    return findThread( name ) != 0;
}

void
ThreadWeaver::dispose( Thread *thread )
{
    // we use dispose to abort threads because it is possible that Jobs
    // are in the Qt eventloop, and they can't be deleted out from under
    // Qt or the application will crash, so we delay thread deletion

    thread->abortAllJobs();
    thread->wait(); //FIXME blocks the UI of course

    //get it out of the list of expected receivers now
    m_threads.remove( thread );

    QCustomEvent *e = new QCustomEvent( DeleteThreadEvent );
    e->setData( thread );

    QApplication::postEvent( this, e );
}

void
ThreadWeaver::registerDependent( QObject *dependent, const char *name )
{
    connect( dependent, SIGNAL(destroyed()), SLOT(dependentAboutToBeDestroyed()) );
}

void
ThreadWeaver::dependentAboutToBeDestroyed()
{
    debug() << "Dependent: " << sender()->name() << " destroyed\n";
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
        Job    *job    = (Job*)e;
        Thread *thread = findThread( job->name() );
        debugstream d   = debug() << "Job ";

        if ( !job->isAborted() ) {
            d << "completed";
            job->completeJob();
        }
        else d << "aborted";

        d << ": " << QCString(job->name());

        // is there any more jobs of this type?
        // if so start the next one


        if ( thread ) {
            d << ". Jobs pending: " << thread->pendingJobs.count();
            //run next job, if there is one
            thread->runJob( 0 ); }
        else
            warning() << "Thread is unexpectedly already deleted: " << QCString(job->name()) << endl;

        d << endl;

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


ThreadWeaver::Thread::Thread( const char *_name )
    : QThread()
    , name( _name )
{
    QApplication::postEvent( ThreadWeaver::instance(), new QCustomEvent( ThreadWeaver::OverrideCursorEvent ) );
}

ThreadWeaver::Thread::~Thread()
{
    //if we were aborted, this has already occurred
    ThreadWeaver::instance()->m_threads.remove( this );

    Q_ASSERT( finished() );
    Q_ASSERT( pendingJobs.isEmpty() );

    QApplication::restoreOverrideCursor();
}

void
ThreadWeaver::Thread::runJob( Job *job )
{
    if ( !job ) {
        if ( !pendingJobs.isEmpty() ) {
            runningJob = pendingJobs.front();
            pendingJobs.pop_front();
            start( IdlePriority ); //will wait for the current operation to complete, should be soon
        }
        else {
            wait(); //sometimes we get the finished event before the thread has finished
            delete this; //FIXME safer to use ThreadWeaver::dispose()
            return;
        }
    }
    else if( !running() ) {
       job->m_thread = this;
       runningJob = job;
       start( IdlePriority );
    }
    else
        pendingJobs += job;
}

void
ThreadWeaver::Thread::run()
{
    // BE THREAD-SAFE!

    Job *job = runningJob;

    job->m_aborted |= !job->doJob();

    QApplication::postEvent( ThreadWeaver::instance(), job );
}

void
ThreadWeaver::Thread::abortAllJobs()
{
    for( JobList::Iterator it = pendingJobs.begin(), end = pendingJobs.end(); it != end; ++it )
        delete *it;

    //this will be deleted when after it has been aborted
    runningJob->m_aborted = true;

    pendingJobs.clear();
}


/// @class ThreadWeaver::Job

ThreadWeaver::Job::Job( const char *name )
    : QCustomEvent( ThreadWeaver::JobEvent )
    , m_name( name )
    , m_thread( 0 )
    , m_percentDone( 0 )
    , m_progressDone( 0 )
    , m_totalSteps( 0 )
{}


ThreadWeaver::Job::~Job()
{}

void
ThreadWeaver::Job::setProgressTotalSteps( uint steps )
{
    m_totalSteps = steps;

    QApplication::postEvent( this, new ProgressEvent( -1 ) );
}

void
ThreadWeaver::Job::setProgress( uint steps )
{
    m_progressDone = steps;

    int newPercent = int( (100 * steps) / m_totalSteps);

    if ( newPercent > m_percentDone ) {
        m_percentDone = newPercent;
        QApplication::postEvent( this, new ProgressEvent( newPercent ) );
    }
}

void
ThreadWeaver::Job::setStatus( const QString &status )
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
    ThreadWeaver::instance()->registerDependent( dependent, name );

    QApplication::postEvent( dependent, new QCustomEvent( JobStartedEvent ) );
}

void
ThreadWeaver::DependentJob::completeJob()
{
    //syncronous, so we don't get deleted twice
    QApplication::sendEvent( m_dependent, this );
}

#include "threadweaver.moc"
