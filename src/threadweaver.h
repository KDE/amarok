// Author: Max Howell (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef THREADWEAVER_H
#define THREADWEAVER_H

#include <qevent.h>   //baseclass
#include <qguardedptr.h>
#include <qmap.h>
#include <qobject.h>
#include <qthread.h>
#include <qvaluelist.h>


//not all these are always generated, but the idea is just to prevent it
#define DISABLE_GENERATED_MEMBER_FUNCTIONS( T ) \
    T(); \
    T( const T& ); \
    T &operator=( const T& ); \
    bool operator==( const T& ) const;


/**
 * @class ThreadWeaver
 * @author Max Howell <max.howell@methylblue.com>
 * @short ThreadWeaver is here to encourage you to use threads and to make their use easy.
 *
 * You create Jobs on the heap and ThreadWeaver allows you to easily queue them, abort them,
 * ensure only one runs at once, ensure that bad data is never acted on, and even cleans up for
 * you should the class that wants the Job's results get deleted while a thread is running.
 *
 * You also will (soon) get thread-safe error handling and thread-safe progress reporting.
 *
 * Usual use:
 *
 * ThreadWeaver::instance()->queueJob( new MyJob( DataToProcess ) );
 *
 * That's it! The queue is fifo, the ThreadWeaver takes ownership of the Job, and the Weaver
 * calls Job::completeJob() on completion which you reimplement to do whatever you need done.
 *
 * @see ThreadWeaver::Job
 */

class ThreadWeaver : public QObject
{
    Q_OBJECT

public:
   class Job;
   typedef QValueList<Job*> JobList;

   enum EventType { JobEvent = 2000, JobStartedEvent = JobEvent, JobFinishedEvent, DeleteThreadEvent };

   static ThreadWeaver *instance();

   /**
    * Will queue a job for processing.
    * If the ThreadWeaver is already handling a job of this type then the job
    * will be queued, otherwise the job will be processed immediately.
    * Allocate the job on the heap, and ThreadWeaver will delete it for you.
    * This is not thread-safe!
    * @see ThreadWeaver::Job
    */
   int queueJob( Job* );

   /**
    * Queue multiple jobs simultaneously, you should use this to avoid the race
    * condition where the first job finishes before you can queue the next one.
    * The only valid usage, is when the jobs are the same type!
    * This is not thread-safe!
    */
   int queueJobs( const JobList& );

   /**
    * If there are other jobs running, it will cancel them
    * and start this one afterwards.
    */
   void onlyOneJob( Job* );


   /**
    * All the named jobs will be halted and deleted
    * You cannot use any data from the jobs reliably after this point.
    * You will not receive any completion notification from these jobs.
    * This is not thread-safe!
    * @return -1 if there is no thread that operates on the named job, otherwise how many jobs were aborted
    */
   int abortAllJobsNamed( const QCString &name );

private slots:
   void dependentAboutToBeDestroyed();

private:
    ThreadWeaver();
   ~ThreadWeaver();

    virtual void customEvent( QCustomEvent* );


    /**
     * Class Thread
     */
    class Thread : public QThread
    {
    public:
        Thread( const char* );

        virtual void run();

        void runJob( Job *job );

        JobList pendingJobs;
        Job *runningJob;

        char const * const name;

        void abortAllJobs();

    protected:
        DISABLE_GENERATED_MEMBER_FUNCTIONS( Thread );

    private:
        friend void ThreadWeaver::customEvent( QCustomEvent* );

        ~Thread();
    };


    /// safe disposal for threads that may not have finished
    void dispose( Thread* );

    /// returns the thread that handles Jobs that use @name
    inline Thread *findThread( const QCString &name );

    typedef QValueList<Thread*> ThreadList;

    ThreadList m_threads;

public:
    /**
     * @class Job
     * @short You do some work in the background with these
     */
    class Job : public QCustomEvent
    {
        friend class ThreadWeaver;
        friend class ThreadWeaver::Thread;

    public:
        /**
         * The name is important, all like-named jobs are queued and run FIFO.
         * Always allocate Jobs on the heap, ThreadWeaver will take ownership of the memory.
         */
        Job( const char *name );
        virtual ~Job();

        const char *name() const { return m_name; }

        /**
         * If this is true then in the worst case the entire amaroK UI is frozen waiting for
         * your Job to abort! You should check for this often, but not so often that your
         * code's readability suffers as a result.
         * @note if your thread was aborted pending jobs will not have completeJob() called for them
         *       even if you return true from doJob()
         *
         * @note it is safe to abort a thread yourself, if you just want to abort one job, return false from
         *       doJob()
         */

        //TODO run a separate thread to wait on aborting threads

        bool isAborted() const { return m_aborted; }

    protected:
        /**
         * Executed inside the thread, this should be reimplemented to do the job's work.
         */
        virtual bool doJob() = 0;

        /**
         * This is executed in the GUI thread after doJob() has completed
         */
        virtual void completeJob() = 0;

    private:
        char const * const m_name;
        bool m_aborted;

    protected:
        DISABLE_GENERATED_MEMBER_FUNCTIONS( Job )
    };


    /**
     * @class DependentJob
     * @short A Job that depends on the existence of a QObject
     *
     * This Job type is dependent on a QObject class,
     * if that class is deleted this Job will promptly abort and be deleted.
     * completeJob() is implemented to send a JobCompletionEvent to the
     * dependent. You can still override this if you want no handling in your
     * dependent class.
     *
     * The dependent is a QGuardedPointer, so you can reference the pointer
     * safely if you always test for 0 first, however it is definately safest
     * to not rely on that pointer, pass required data-members with the job,
     * only operate on the dependent in completeJob() which is only called if
     * the dependent is still instantiated!
     *
     * It is only safe to have one dependent, if you depend on multiple objects
     * that might get deleted while you are running you should instead try to make
     * the multiple objects children of one QObject and depend on the top-most parent
     * or best of all would be to make copies of the data you need instead of being
     * dependent
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
        DISABLE_GENERATED_MEMBER_FUNCTIONS( DependentJob );
    };

private:
    friend DependentJob::DependentJob( QObject*, const char* );

    void registerDependent( QObject*, const char* );
};

inline ThreadWeaver*
ThreadWeaver::instance()
{
    static ThreadWeaver instance;

    return &instance;
}

#endif
