// Author: Max Howell (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

// the asserts we use in this module prevent crashes, so best to abort the application if they fail
#define QT_FATAL_ASSERT

#include <kcursor.h>
#include <kdebug.h>
#include <qapplication.h>
#include "threadweaver.h"


#ifdef NDEBUG
   static inline kndbgstream debug() { return kndbgstream(); }
#else
   static inline kdbgstream debug() { return kdbgstream( "[ThreadWeaver] ", 0, 0 ); }
#endif


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
    debug() << k_funcinfo << endl;

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

    debug() << "Queuing: " << name << endl;

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
    debug() << k_funcinfo << ": " <<  sender()->name() << " destroyed\n";
}

void
ThreadWeaver::customEvent( QCustomEvent *e )
{
    debug() << k_funcinfo << endl;

    switch( e->type() )
    {
    case DeleteThreadEvent:
        delete (Thread*)e->data();
        break;

    case JobEvent: {
        Job *job = (Job*)e;
        // is there any more jobs of this type?
        // if so start the next one
        Thread *thread = findThread( job->name() );

        if ( thread ) {
            if ( !job->isAborted() )
                job->completeJob();

            //run next job, if there is one
            thread->runJob( 0 );
        }
        else
            debug() << "Thread was deleted while processing this job: " << QCString(job->name()) << endl;
    }
    default:
        ;
    }
}



ThreadWeaver::Thread::Thread( const char *_name )
    : QThread()
    , name( _name )
{
    debug() << "Thread::Thread: " << QCString(_name) << endl;

    QApplication::setOverrideCursor( KCursor::workingCursor() );
}

ThreadWeaver::Thread::~Thread()
{
    debug() << "Thread::~Thread: " << QCString(name) << endl;

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

    debug() << "Running Job: " << QCString(job->name()) << endl;

    job->m_aborted |= !job->doJob();

    debug() << "Job Done: " << QCString(job->name()) << ". Aborted? " << job->m_aborted << endl;

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



ThreadWeaver::Job::Job( const char *name )
    : QCustomEvent( ThreadWeaver::JobEvent )
    , m_name( name )
    , m_aborted( false )
    , m_thread( 0 )
{
    debug() << "Job::Job: " << QCString(m_name) << endl;
}

ThreadWeaver::Job::~Job()
{
    debug() << "Job::~Job: " << QCString(m_name) << endl;
}



ThreadWeaver::DependentJob::DependentJob( QObject *dependent, const char *name )
    : Job( name )
    , m_dependent( dependent )
{
    ThreadWeaver::instance()->registerDependent( dependent, name );
}

void
ThreadWeaver::DependentJob::completeJob()
{
    //syncronous
    QApplication::sendEvent( m_dependent, this );
}

#include "threadweaver.moc"
