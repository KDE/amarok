// Author: Max Howell (C) Copyright 2004
// (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>
// See COPYING file that comes with this distribution
//

#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include "debug.h"
#include <qevent.h>   //baseclass
#include <qguardedptr.h>
#include <qmap.h>
#include <qobject.h>
#include <qthread.h>
#include <qvaluelist.h>
#include <qmutex.h>
#include "debug.h"

#define DISABLE_GENERATED_MEMBER_FUNCTIONS_3( T ) \
    T( const T& ); \
    T &operator=( const T& ); \
    bool operator==( const T& ) const;

#define DISABLE_GENERATED_MEMBER_FUNCTIONS_4( T ) \
    T(); \
    DISABLE_GENERATED_MEMBER_FUNCTIONS_3( T )


/**
 * @class ThreadManager
 * @author Max Howell <max.howell@methylblue.com>
 * @short ThreadManager is designed to encourage you to use threads and to make their use easy.
 *
 * You create Jobs on the heap and ThreadManager allows you to easily queue them,
 * abort them, ensure only one runs at once, ensure that bad data is never acted
 * on and even cleans up for you should the class that wants the Job's results
 * get deleted while a thread is running.
 *
 * You also will (soon) get thread-safe error handling and thread-safe progress
 * reporting.
 *
 * This is a typical use:
 *
    class MyJob : public ThreadManager::Job
    {
    public:
        MyJob( QObject *dependent ) : Job( dependent, "MyJob" ) {}

        virtual bool doJob() {
            //do some work in thread...

            return success;
        }

        virtual void completeJob {
            //do completion work in the GUI thread...
        }
    };

    SomeClass::someFunction()
    {
        ThreadManager::instance()->queueJob( new MyJob( this ) );
    }

 *
 * That's it! The queue is fifo, there's one queue per job-type, the
 * ThreadManager takes ownership of the Job, and the Manager calls
 * Job::completeJob() on completion which you reimplement to do whatever you
 * need done.
 *
 * BEWARE! None of the functions are thread-safe, only call them from the GUI
 * thread or your application WILL crash!
 *
 * @see ThreadManager::Job
 * @see ThreadManager::DependentJob
 */


/// This class is because moc "is really good" (no nested Q_OBJECT classes)
class JobBase : public QObject {
Q_OBJECT
protected:    JobBase() : QObject(), m_aborted( false ) {}
public slots: void abort() { m_aborted = true; }
protected:    bool m_aborted;
};

class ThreadManager : public QObject
{
public:
    class Thread;
    friend class Thread;
    typedef QValueList<Thread*> ThreadList;
    class Job;
    friend class Job;
    typedef QValueList<Job*> JobList;

    static ThreadManager *instance();
    static void deleteInstance();

    static volatile uint getNewThreadId();
    static QMutex *threadIdMutex;
    static volatile uint threadIdCounter;

    /**
     * If the ThreadManager is already handling a job of this type then the job
     * will be queued, otherwise the job will be processed immediately. Allocate
     * the job on the heap, and ThreadManager will delete it for you.
     *
     * This is not thread-safe - only call it from the GUI-thread!
     *
     * @return number of jobs in the queue after the call
     * @see ThreadManager::Job
     */
    int queueJob( Job* );

    /**
     * Queue multiple jobs simultaneously, you should use this to avoid the race
     * condition where the first job finishes before you can queue the next one.
     * This isn't a fatal condition, but it does cause wasteful thread deletion
     * and re-creation. The only valid usage, is when the jobs are the same type!
     *
     * This is not thread-safe - only call it from the GUI-thread!
     *
     * @return number of jobs in the queue after the call
     */
    int queueJobs( const JobList& );

    /**
     * If there are other jobs of the same type running, they will be aborted,
     * then this one will be started afterwards. Aborted jobs will not have
     * completeJob() called for them.
     *
     * This is not thread-safe - only call it from the GUI-thread!
     */
    void onlyOneJob( Job* );

    /**
     * All the named jobs will be halted and deleted. You cannot use any data
     * from the jobs reliably after this point. Job::completeJob() will not be
     * called for any of these jobs.
     *
     * This is not thread-safe - only call it from the GUI-thread!
     *
     * @return how many jobs were aborted, or -1 if no thread was found
     */
    int abortAllJobsNamed( const QCString &name );

    /**
     * @return true if a Job with name is queued or is running
     */
    bool isJobPending( const QCString &name ) { return jobCount( name ) > 0; }

    /**
     * @return the number of jobs running, pending, aborted and otherwise.
     */
    uint jobCount( const QCString &name );

private:
    ThreadManager();
   ~ThreadManager();

    enum EventType { JobEvent = 20202, OverrideCursorEvent, RestoreOverrideCursorEvent };

    virtual bool event( QEvent* );

    /// checks the pool for an available thread, creates a new one if required
    Thread *gimmeThread();

    /// safe disposal for threads that may not have finished
    void dispose( Thread* );

    /// all pending and running jobs
    JobList m_jobs;

    /// a thread-pool, ready for use or running jobs currently
    ThreadList m_threads;

public:
    /**
     * Class Thread
     */
    class Thread : public QThread
    {
    public:
        Thread();

        virtual void run();

        void runJob( Job* );
        void msleep( int ms ) { QThread::msleep( ms ); } //we need to make this public for class Job

        Job *job() const { return m_job; }

        static QThread* getRunning();
        static QString  threadId();
        const uint localThreadId() const { return m_threadId; }

    private:
        Job *m_job;

        uint m_threadId;

        //private so I don't break something in the distant future
        ~Thread();

        //we can delete threads here only
        friend bool ThreadManager::event( QEvent* );

    protected:
        DISABLE_GENERATED_MEMBER_FUNCTIONS_3( Thread )
    };

    /**
     * @class Job
     * @short A small class for doing work in a background thread
     *
     * Derive a job, do the work in doJob(), do GUI-safe operations in
     * completeJob(). If you return false from doJob() completeJob() won't be
     * called. Name your Job well as like-named Jobs are queued together.
     *
     * Be sensible and pass data members to the Job, rather than operate on
     * volatile data members in the GUI-thread.
     *
     * Things can change while you are in a separate thread. Stuff in the GUI
     * thread may not be there anymore by the time you finish the job. @see
     * ThreadManager::dependentJob for a solution.
     *
     * Do your cleanup in the destructor not completeJob(), as completeJob()
     * doesn't have to be called.
     */

    class Job : public JobBase, public QCustomEvent
    {
        friend class ThreadManager;         //access to m_thread
        friend class ThreadManager::Thread; //access to m_aborted

    public:
        /**
         * Like-named jobs are queued and run FIFO. Always allocate Jobs on the
         * heap, ThreadManager will take ownership of the memory.
         */
        Job( const char *name );
        virtual ~Job();

        /**
         * These are used by @class DependentJob, but are made available for
         * your use should you need them.
         */
       enum EventType { JobFinishedEvent = ThreadManager::JobEvent, JobStartedEvent };

        const char *name() const { return m_name; }

        /**
         * If this returns true then in the worst case the entire Amarok UI is
         * frozen waiting for your Job to abort! You should check for this
         * often, but not so often that your code's readability suffers as a
         * result.
         *
         * Aborted jobs will not have completeJob() called for them, even if
         * they return true from doJob()
         */
        bool isAborted() const { return m_aborted; }

        ///convenience function
        bool wasSuccessful() const { return !m_aborted; }

        /**
         * Calls QThread::msleep( int )
         */
        void msleep( int ms ) { m_thread->msleep( ms ); }

        /**
         * You should set @param description if you set progress information
         * do this in the ctor, or it won't have an effect
         */
        void setDescription( const QString &description ) { m_description = description; }

        /**
         * If you set progress information, you should set this too, changing it when appropriate
         */
        void setStatus( const QString &status );

        /**
         * This shows the progressBar too, the user will be able to abort
         * the thread
         */
        void setProgressTotalSteps( uint steps );

        /**
         * Does a thread-safe update of the progressBar
         */
        void setProgress( uint progress );
        void setProgress100Percent() { setProgress( m_totalSteps ); }

        /**
         * Convenience function, increments the progress by 1
         */
        void incrementProgress();

        /**
         * Sometimes you want to hide the progressBar etc. generally you
         * should show one, but perhaps you are a reimplemented class
         * that doesn't want one?
         */
        //void setVisible( bool );

        uint parentThreadId() { return m_parentThreadId; }

    protected:
        /**
         * Executed inside the thread, this should be reimplemented to do the
         * job's work. Be thread-safe! Don't interact with the GUI-thread.
         *
         * @return true if you want completeJob() to be called from the GUI
         * thread
         */
        virtual bool doJob() = 0;

        /**
         * This is executed in the GUI thread if doJob() returns true;
         */
        virtual void completeJob() = 0;

        /// be sure to call the base function in your reimplementation
        virtual void customEvent( QCustomEvent* );

    private:
        char const * const m_name;
        Thread *m_thread;

        protected: //FIXME
        uint m_percentDone;
        uint m_progressDone;
        uint m_totalSteps;
        uint m_parentThreadId;

        QString m_description;
        QString m_status;

    protected:
        DISABLE_GENERATED_MEMBER_FUNCTIONS_4( Job )
    };


    /**
     * @class DependentJob
     * @short A Job that depends on the existence of a QObject
     *
     * This Job type is dependent on a QObject instance, if that instance is
     * deleted, this Job will be aborted and safely deleted.
     *
     * ThreadManager::DependentJob (and Job, the baseclass) isa QCustomEvent,
     * and completeJob() is reimplemented to send the job to the dependent.
     * Of course you can still reimplement completeJob() yourself.
     *
     * The dependent will receive a JobStartedEvent just after the creation of
     * the Job (not after it has started unfortunately), and a JobFinishedEvent
     * after the Job has finished.
     *
     * The dependent is a QGuardedPtr, so you can reference the pointer returned
     * from dependent() safely provided you always test for 0 first. However
     * safest of all is to not rely on that pointer at all! Pass required
     * data-members with the job, only operate on the dependent in
     * completeJob(). completeJob() will not be called if the dependent no
     * longer exists
     *
     * It is only safe to have one dependent, if you depend on multiple objects
     * that might get deleted while you are running you should instead try to
     * make the multiple objects children of one QObject and depend on the
     * top-most parent or best of all would be to make copies of the data you
     * need instead of being dependent.
     */

    class DependentJob : public Job
    {
    public:
        DependentJob( QObject *dependent, const char *name );

        virtual void completeJob();

        QObject *dependent() { return m_dependent; }

    private:
        const QGuardedPtr<QObject> m_dependent;

    protected:
        DISABLE_GENERATED_MEMBER_FUNCTIONS_4( DependentJob )
    };

protected:
    ThreadManager( const ThreadManager& );
    ThreadManager &operator=( const ThreadManager& );
};

//useful debug thingy
#define DEBUG_THREAD_FUNC_INFO { Debug::mutex.lock(); kdDebug() << Debug::indent() << k_funcinfo << "thread: " << ThreadManager::Thread::threadId() << endl; Debug::mutex.unlock(); }

#define SHOULD_BE_GUI if( ThreadManager::Thread::getRunning() ) warning() \
    << __PRETTY_FUNCTION__ <<  " should not be Threaded, but is running in " << \
    ThreadManager::Thread::getRunning() <<endl;

inline ThreadManager*
ThreadManager::instance()
{
    static ThreadManager* instance = new ThreadManager();

    return instance;
}

inline void
ThreadManager::deleteInstance()
{
    delete instance();
}

inline volatile uint
ThreadManager::getNewThreadId()
{
    uint temp;
    threadIdMutex->lock();
    temp = threadIdCounter++;
    threadIdMutex->unlock();
    return temp;
}

#endif
