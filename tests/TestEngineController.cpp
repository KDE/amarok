/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestEngineController.h"

#include "EngineController.h"
#include "core/support/Components.h"

#include <QTest>
#include <QSignalSpy>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Queue>


QTEST_MAIN( TestEngineController )

class CallSupportedMimeTypesJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT

    protected:
        void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
        {
            Q_UNUSED(self);
            Q_UNUSED(thread);
            EngineController *ec = Amarok::Components::engineController();
            QVERIFY( ec );
            QStringList types = ec->supportedMimeTypes();
            QVERIFY( !types.isEmpty() );
        }

        void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
        {
            Q_EMIT started(self);
            ThreadWeaver::Job::defaultBegin(self, thread);
        }

        void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
        {
            ThreadWeaver::Job::defaultEnd(self, thread);
            if (!self->success()) {
                Q_EMIT failed(self);
            }
            Q_EMIT done(self);
        }

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

};

void
TestEngineController::init()
{
    // the test depend on EngineController being used for the first time
    QVERIFY( Amarok::Components::engineController() == 0 );
    Amarok::Components::setEngineController( new EngineController() );
}

void
TestEngineController::cleanup()
{
    // we cannot simply call WeaverInterface::finish(), it stops event loop
    QSignalSpy spy( ThreadWeaver::Queue::instance(), &ThreadWeaver::Queue::finished );
    if( !ThreadWeaver::Queue::instance()->isIdle() )
        QVERIFY2( spy.wait( 5000 ), "threads did not finish in timeout" );

    delete Amarok::Components::setEngineController( 0 );
}

void
TestEngineController::testSupportedMimeTypesInMainThread()
{
    EngineController *ec = Amarok::Components::engineController();
    QVERIFY( ec );
    QStringList types = ec->supportedMimeTypes();
    QVERIFY( !types.isEmpty() );
}

void
TestEngineController::testSupportedMimeTypesInAnotherThread()
{
    ThreadWeaver::JobPointer job = QSharedPointer<ThreadWeaver::Job>(new CallSupportedMimeTypesJob());
    ThreadWeaver::Queue::instance()->enqueue( job );
}

#include "TestEngineController.moc"
