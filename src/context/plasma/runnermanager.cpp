/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2007 Ryan P. Bitanga <ryan.bitanga@gmail.com>
 *   Copyright (2) 2008 Jordi Polo <mumismo@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "runnermanager.h"

#include <QMutex>
#include <QTimer>
#include <QCoreApplication>

#include <KDebug>
#include <KPluginInfo>
#include <KServiceTypeTrader>
#include <KStandardDirs>

#include <Solid/Device>
#include <Solid/DeviceInterface>

#include <threadweaver/DebuggingAids.h>
#include <ThreadWeaver/Thread>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/QueuePolicy>
#include <ThreadWeaver/Weaver>

#include "querymatch.h"

using ThreadWeaver::Weaver;
using ThreadWeaver::Job;

namespace Plasma
{

/*****************************************************
*  RunnerRestrictionPolicy class
* Restricts simultaneous jobs of the same type
* Similar to ResourceRestrictionPolicy but check the object type first
******************************************************/
class RunnerRestrictionPolicy : public ThreadWeaver::QueuePolicy
{
public:
    ~RunnerRestrictionPolicy();

    static RunnerRestrictionPolicy &instance();

    void setCap(int cap)
    {
        m_cap = cap;
    }
    int cap() const
    {
        return m_cap;
    }

    bool canRun(Job *job);
    void free(Job *job);
    void release(Job *job);
    void destructed(Job *job);
private:
    RunnerRestrictionPolicy();

    int m_count;
    int m_cap;
    QMutex m_mutex;
};

RunnerRestrictionPolicy::RunnerRestrictionPolicy()
    : QueuePolicy(),
      m_count(0),
      m_cap(2)
{
}

RunnerRestrictionPolicy::~RunnerRestrictionPolicy()
{
}

RunnerRestrictionPolicy &RunnerRestrictionPolicy::instance()
{
    static RunnerRestrictionPolicy policy;
    return policy;
}

bool RunnerRestrictionPolicy::canRun(Job *job)
{
    Q_UNUSED(job)
    QMutexLocker l(&m_mutex);
    if (m_count > m_cap) {
        return false;
    } else {
        ++m_count;
        return true;
    }
}

void RunnerRestrictionPolicy::free(Job *job)
{
    Q_UNUSED(job)
    QMutexLocker l(&m_mutex);
    --m_count;
}

void RunnerRestrictionPolicy::release(Job *job)
{
    free(job);
}

void RunnerRestrictionPolicy::destructed(Job *job)
{
    Q_UNUSED(job)
}

/*****************************************************
*  FindMatchesJob class
* Class to run queries in different threads
******************************************************/
class FindMatchesJob : public Job
{
public:
    FindMatchesJob(Plasma::AbstractRunner *runner,
                   Plasma::RunnerContext *context, QObject *parent = 0);

    int priority() const;
    Plasma::AbstractRunner *runner() const;

protected:
    void run();
private:
    Plasma::RunnerContext *m_context;
    Plasma::AbstractRunner *m_runner;
};

FindMatchesJob::FindMatchesJob(Plasma::AbstractRunner *runner,
                               Plasma::RunnerContext *context, QObject *parent)
    : ThreadWeaver::Job(parent),
      m_context(context),
      m_runner(runner)
{
    if (runner->speed() == Plasma::AbstractRunner::SlowSpeed) {
        assignQueuePolicy(&RunnerRestrictionPolicy::instance());
    }
}

void FindMatchesJob::run()
{
//     kDebug() << "Running match for " << m_runner->objectName()
//              << " in Thread " << thread()->id() << endl;
    m_runner->performMatch(*m_context);
}

int FindMatchesJob::priority() const
{
    return m_runner->priority();
}

Plasma::AbstractRunner *FindMatchesJob::runner() const
{
    return m_runner;
}

/*****************************************************
*  RunnerManager::Private class
*
*****************************************************/
class RunnerManagerPrivate
{
public:

    RunnerManagerPrivate(RunnerManager *parent)
      : q(parent),
        deferredRun(0)
    {
        matchChangeTimer.setSingleShot(true);
        QObject::connect(&matchChangeTimer, SIGNAL(timeout()), q, SLOT(matchesChanged()));
        QObject::connect(&context, SIGNAL(matchesChanged()), q, SLOT(scheduleMatchesChanged()));
    }

    void scheduleMatchesChanged()
    {
        matchChangeTimer.start(0);
    }

    void matchesChanged()
    {
        emit q->matchesChanged(context.matches());
    }

    void loadConfiguration(KConfigGroup &conf)
    {
        config = conf;

        //The number of threads used scales with the number of processors.
        const int numProcs =
            qMax(Solid::Device::listFromType(Solid::DeviceInterface::Processor).count(), 1);
        //This entry allows to define a hard upper limit independent of the number of processors.
        const int maxThreads = config.readEntry("maxThreads", 16);
        const int numThreads = qMin(maxThreads, 2 + ((numProcs - 1) * 2));
        //kDebug() << "setting up" << numThreads << "threads for" << numProcs << "processors";
        Weaver::instance()->setMaximumNumberOfThreads(numThreads);

        //Preferred order of execution of runners
        //prioritylist = config.readEntry("priority", QStringList());

        //If set, this list defines which runners won't be used at runtime
        //blacklist = config.readEntry("blacklist", QStringList());
    }

    void loadRunners()
    {
        KService::List offers = KServiceTypeTrader::self()->query("Plasma/Runner");

        bool loadAll = config.readEntry("loadAll", false);
        KConfigGroup conf(&config, "Plugins");

        foreach (const KService::Ptr &service, offers) {
            //kDebug() << "Loading runner: " << service->name() << service->storageId();
            QString tryExec = service->property("TryExec", QVariant::String).toString();
            kDebug() << "tryExec is" << tryExec;
            if (!tryExec.isEmpty() && KStandardDirs::findExe(tryExec).isEmpty()) {
                // we don't actually have this application!
                continue;
            }

            KPluginInfo description(service);
            QString runnerName = description.pluginName();
            description.load(conf);

            bool loaded = runners.contains(runnerName);
            bool selected = loadAll || description.isPluginEnabled();

            if (selected) {
                if (!loaded) {
                    QString api = service->property("X-Plasma-API").toString();
                    QString error;
                    AbstractRunner *runner = 0;

                    if (api.isEmpty()) {
                        QVariantList args;
                        args << service->storageId();
                        if (Plasma::isPluginVersionCompatible(KPluginLoader(*service).pluginVersion())) {
                            runner = service->createInstance<AbstractRunner>(q, args, &error);
                        }
                    } else {
                        //kDebug() << "got a script runner known as" << api;
                        runner = new AbstractRunner(q, service->storageId());
                    }

                    if (runner) {
                        kDebug() << "loading runner:" << service->name();
                        runners.insert(runnerName, runner);
                    } else {
                        kDebug() << "failed to load runner:" << service->name()
                                 << ". error reported:" << error;
                    }
                }
            } else if (loaded) {
                //Remove runner
                AbstractRunner *runner = runners.take(runnerName);
                kDebug() << "Removing runner: " << runnerName;
                delete runner;
            }
        }

        //kDebug() << "All runners loaded, total:" << runners.count();
    }

    void jobDone(ThreadWeaver::Job *job)
    {
        FindMatchesJob *runJob = static_cast<FindMatchesJob*>(job);
        if (deferredRun.isEnabled() && runJob->runner() == deferredRun.runner()) {
            //kDebug() << "job actually done, running now **************";
            deferredRun.run(context);
            deferredRun = QueryMatch(0);
        }
        searchJobs.removeAll(runJob);
        delete runJob;
    }

    RunnerManager *q;
    QueryMatch deferredRun;
    RunnerContext context;
    QTimer matchChangeTimer;
    QHash<QString, AbstractRunner*> runners;
    QList<FindMatchesJob*> searchJobs;
//     QStringList prioritylist;
    bool loadAll;
    KConfigGroup config;
};

/*****************************************************
*  RunnerManager::Public class
*
*****************************************************/
RunnerManager::RunnerManager(QObject *parent)
    : QObject(parent),
      d(new RunnerManagerPrivate(this))
{
    KConfigGroup config(KGlobal::config(), "PlasmaRunnerManager");
    d->loadConfiguration(config);
    //ThreadWeaver::setDebugLevel(true, 4);
}

RunnerManager::RunnerManager(KConfigGroup &c, QObject *parent)
    : QObject(parent),
      d(new RunnerManagerPrivate(this))
{
    // Should this be really needed? Maybe d->loadConfiguration(c) would make
    // more sense.
    KConfigGroup config(&c, "PlasmaRunnerManager");
    d->loadConfiguration(config);
    //ThreadWeaver::setDebugLevel(true, 4);
}

RunnerManager::~RunnerManager()
{
    delete d;
}

void RunnerManager::reloadConfiguration()
{
    d->loadConfiguration(d->config);
    d->loadRunners();
}

AbstractRunner *RunnerManager::runner(const QString &name) const
{
    if (d->runners.isEmpty()) {
        d->loadRunners();
    }

    return d->runners.value(name);
}

RunnerContext *RunnerManager::searchContext() const
{
    return &d->context;
}

//Reordering is here so data is not reordered till strictly needed
QList<QueryMatch> RunnerManager::matches() const
{
    return d->context.matches();
}

void RunnerManager::run(const QString &id)
{
    run(d->context.match(id));
}

void RunnerManager::run(const QueryMatch &match)
{
    if (!match.isEnabled()) {
        return;
    }

    //TODO: this function is not const as it may be used for learning
    AbstractRunner *runner = match.runner();

    foreach (FindMatchesJob *job, d->searchJobs) {
        if (job->runner() == runner && !job->isFinished()) {
            //kDebug() << "!!!!!!!!!!!!!!!!!!! uh oh!";
            d->deferredRun = match;
            return;
        }
    }

    match.run(d->context);

    if (d->deferredRun.isValid()) {
        d->deferredRun = QueryMatch(0);
    }
}

void RunnerManager::launchQuery(const QString &term)
{
    launchQuery(term, QString());
}

void RunnerManager::launchQuery(const QString &term, const QString &runnerName)
{
    if (d->runners.isEmpty()) {
        d->loadRunners();
    }

    if (term.isEmpty()) {
        reset();
        return;
    }

    if (d->context.query() == term) {
        // we already are searching for this!
        return;
    }

    reset();
//    kDebug() << "runners searching for" << term << "on" << runnerName;
    d->context.setQuery(term);

    AbstractRunner::List runable;

    //if the name is not empty we will launch only the specified runner
    if (!runnerName.isEmpty()) {
        runable.append(runner(runnerName));
    } else {
        runable = d->runners.values();
    }

    foreach (Plasma::AbstractRunner *r, runable) {
        if ((r->ignoredTypes() & d->context.type()) == 0) {
//            kDebug() << "launching" << r->name();
            FindMatchesJob *job = new FindMatchesJob(r, &d->context, this);
            connect(job, SIGNAL(done(ThreadWeaver::Job*)), this, SLOT(jobDone(ThreadWeaver::Job*)));
            Weaver::instance()->enqueue(job);
            d->searchJobs.append(job);
        }
    }
}

bool RunnerManager::execQuery(const QString &term)
{
    return execQuery(term, QString());
}

bool RunnerManager::execQuery(const QString &term, const QString &runnerName)
{
    if (d->runners.isEmpty()) {
        d->loadRunners();
    }

    if (term.isEmpty()) {
        reset();
        return false;
    }

    if (d->context.query() == term) {
        // we already are searching for this!
        emit matchesChanged(d->context.matches());
        return false;
    }

    reset();
    //kDebug() << "executing query about " << term << "on" << runnerName;
    d->context.setQuery(term);
    AbstractRunner *r = runner(runnerName);

    if (!r) {
        //kDebug() << "failed to find the runner";
        return false;
    }

    if ((r->ignoredTypes() & d->context.type()) != 0) {
        //kDebug() << "ignored!";
        return false;
    }

    r->performMatch(d->context);
    //kDebug() << "succeeded with" << d->context.matches().count() << "results";
    emit matchesChanged(d->context.matches());
    return true;
}

QString RunnerManager::query() const
{
    return d->context.query();
}

void RunnerManager::reset()
{
    // If ThreadWeaver is idle, it is safe to clear previous jobs
    if (Weaver::instance()->isIdle()) {
        qDeleteAll(d->searchJobs);
        d->searchJobs.clear();
    } else {
        Weaver::instance()->dequeue();
    }

    if (d->deferredRun.isEnabled()) {
        //kDebug() << "job actually done, running now **************";
        d->deferredRun.run(d->context);
        d->deferredRun = QueryMatch(0);
    }

    d->context.reset();
}

} // Plasma namespace

#include "runnermanager.moc"
