/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
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

#ifndef PLASMA_ABSTRACTRUNNER_H
#define PLASMA_ABSTRACTRUNNER_H

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QStringList>

#include <KDE/KConfigGroup>
#include <KDE/KService>

#include <plasma/plasma_export.h>
#include <plasma/runnercontext.h>
#include <plasma/querymatch.h>
#include <plasma/version.h>

class KCompletion;

namespace Plasma
{

class Package;
class RunnerScript;
class QueryMatch;
class AbstractRunnerPrivate;

/**
 * @class AbstractRunner plasma/abstractrunner.h <Plasma/AbstractRunner>
 *
 * @short An abstract base class for Plasma Runner plugins.
 *
 * Be aware that runners have to be thread-safe. This is due to the fact that
 * each runner is executed in its own thread for each new term. Thus, a runner
 * may be executed more than once at the same time. See match() for details.
 */
class PLASMA_EXPORT AbstractRunner : public QObject
{
    Q_OBJECT

    public:
        /** Specifies a nominal speed for the runner */
        enum Speed {
            SlowSpeed,
            NormalSpeed
        };

        /** Specifies a priority for the runner */
        enum Priority {
            LowestPriority = 0,
            LowPriority,
            NormalPriority,
            HighPriority,
            HighestPriority
        };

        /** An ordered list of runners */
        typedef QList<AbstractRunner*> List;

        virtual ~AbstractRunner();

        /**
         * This is the main query method. It should trigger creation of
         * QueryMatch instances through RunnerContext::addMatch and 
         * RunnerContext::addMatches. It is called internally by performMatch().
         *
         * If the runner can run precisely the requested term (RunnerContext::query()),
         * it should create an exact match by setting the type to RunnerContext::ExactMatch.
         * The first runner that creates a QueryMatch will be the
         * default runner. Other runner's matches will be suggested in the
         * interface. Non-exact matches should be offered via RunnerContext::PossibleMatch.
         *
         * The match will be activated via run() if the user selects it.
         *
         * Each runner is executed in its own thread. Whenever the user input changes this
         * method is called again. Thus, it needs to be thread-safe. Also, all matches need
         * to be reported once this method returns. Asyncroneous runners therefore need
         * to make use of a local event loop to wait for all matches.
         *
         * It is recommended to use local status data in async runners. The simplest way is 
         * to have a separate class doing all the work like so:
         *
         * \code
         * void MyFancyAsyncRunner::match( RunnerContext& context )
         * {
         *     QEventLoop loop;
         *     MyAsyncWorker worker( context );
         *     connect( &worker, SIGNAL(finished()),
         *              &loop, SLOT(quit()) );
         *     worker.work();
         *     loop.exec();
         * }
         * \endcode
         *
         * Here MyAsyncWorker creates all the matches and calls RunnerContext::addMatch
         * in some internal slot. It emits the finished() signal once done which will
         * quit the loop and make the match() method return. The local status is kept
         * entirely in MyAsyncWorker which makes match() trivially thread-safe.
         *
         * @caution This method needs to be thread-safe since KRunner will simply
         * start a new thread for each new term.
         *
         * @caution Returning from this method means to end execution of the runner.
         *
         * @sa run(), RunnerContext::addMatch, RunnerContext::addMatches, QueryMatch
         */
        // trueg: why is this method not protected?
        virtual void match(Plasma::RunnerContext &context);

        /**
         * Triggers a call to match. This will call match() internally.
         *
         * @arg context the search context used in executing this match.
         */
        void performMatch(Plasma::RunnerContext &context);

        /**
         * If the runner has options that the user can interact with to modify
         * what happens when run or one of the actions created in fillMatches
         * is called, the runner should return true
         */
        bool hasRunOptions();

        /**
         * If hasRunOptions() returns true, this method may be called to get
         * a widget displaying the options the user can interact with to modify
         * the behaviour of what happens when a given match is selected.
         *
         * @param widget the parent of the options widgets.
         */
        virtual void createRunOptions(QWidget *widget);

        /**
         * Called whenever an exact or possible match associated with this
         * runner is triggered.
         *
         * @param context The context in which the match is triggered, i.e. for which
         *                the match was created.
         * @param match The actual match to run/execute.
         */
        virtual void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match);

        /**
         * The nominal speed of the runner.
         * @see setSpeed
         */
        Speed speed() const;

        /**
         * The priority of the runner.
         * @see setPriority
         */
        Priority priority() const;

        /**
         * Returns the OR'ed value of all the Information types (as defined in RunnerContext::Type)
         * this runner is not interested in.
         * @return OR'ed value of black listed types
         */
        RunnerContext::Types ignoredTypes() const;

        /**
         * Sets the types this runner will ignore
         * @param types OR'ed listed of ignored types
         */
        void setIgnoredTypes(RunnerContext::Types types);

        /**
          * Returns the user visible engine name for the Runner
          */
        QString name() const;

        /**
          * Returns an id string for the Runner
          */
        QString id() const;

        /**
          * Returns the description of this Runner
          */
        QString description() const;

        /**
         * Accessor for the associated Package object if any.
         *
         * Note that the returned pointer is only valid for the lifetime of
         * the runner.
         *
         * @return the Package object, or 0 if none
         **/
        const Package *package() const;

        /**
         * Signal runner to reload its configuration.
         */
        virtual void reloadConfiguration();

    protected:
        friend class RunnerManager;
        friend class RunnerManagerPrivate;

        /**
         * Constructs a Runner object. Since AbstractRunner has pure virtuals,
         * this constructor can not be called directly. Rather a subclass must
         * be created
         */
        explicit AbstractRunner(QObject *parent = 0, const QString &serviceId = QString());
        AbstractRunner(QObject *parent, const QVariantList &args);

        /**
         * Provides access to the runner's configuration object.
         */
        KConfigGroup config() const;

        /**
         * Sets whether or not the runner has options for matches
         */
        void setHasRunOptions(bool hasRunOptions);

        /**
         * Sets the nominal speed of the runner. Only slow runners need
         * to call this within their constructor because the default
         * speed is NormalSpeed. Runners that use DBUS should call
         * this within their constructors.
         */
        void setSpeed(Speed newSpeed);

        /**
         * Sets the priority of the runner. Lower priority runners are executed
         * only after higher priority runners.
         */
        void setPriority(Priority newPriority);

        /**
         * A blocking method to do queries of installed Services which can provide
         * a measure of safety for runners running their own threads. This should
         * be used instead of calling KServiceTypeTrader::query(..) directly.
         *
         * @arg serviceType a service type like "Plasma/Applet" or "KFilePlugin"
         * @arg constraint a constraint to limit the choices returned.
         * @see KServiceTypeTrader::query(const QString&, const QString&)
         *
         * @return a list of services that satisfy the query.
         */
        KService::List serviceQuery(const QString &serviceType,
                                    const QString &constraint = QString()) const;

        QMutex *bigLock() const;

    protected Q_SLOTS:
        void init();

    private:
        AbstractRunnerPrivate *const d;
};

} // Plasma namespace

#define K_EXPORT_PLASMA_RUNNER( libname, classname )     \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("plasma_runner_" #libname)) \
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#define K_EXPORT_RUNNER_CONFIG( name, classname )     \
K_PLUGIN_FACTORY(ConfigFactory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(ConfigFactory("kcm_krunner_" #name)) \
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)

#endif
