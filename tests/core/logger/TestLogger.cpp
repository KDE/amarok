/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "TestLogger.h"

#include "core/logger/Logger.h"

#include "mocks/MockLogger.h"

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Job>

#include <QCoreApplication>

#include <KJob>

#include <gmock/gmock.h>

QTEST_GUILESS_MAIN( TestLogger )

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;
using ::testing::Mock;


class DummyJob : public KJob
{
public:
    void start() override {}
};

TestLogger::TestLogger()
{
    int argc = 1;
    char **argv = (char **) malloc(sizeof(char *));
    argv[0] = strdup( QCoreApplication::applicationName().toLocal8Bit().data() );
    ::testing::InitGoogleMock( &argc, argv );
    free( argv );
}

void
TestLogger::init()
{
}

void
TestLogger::cleanup()
{
}

class ProgressJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
public:
    ProgressJob() : deleteJob( false ), deleteObject( false ) {}
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread) override
    {
        Q_UNUSED(self);
        Q_UNUSED(thread);
        KJob *job = new DummyJob();
        QObject *obj = new QObject();
        Amarok::Logger::newProgressOperation( job, QString( "foo" ), obj );

        if( deleteJob ) delete job;
        if( deleteObject ) delete obj;
    }

    bool deleteJob;
    bool deleteObject;

    protected:
    void defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override
    {
        Q_EMIT started(self);
        ThreadWeaver::Job::defaultBegin(self, thread);
    }

    void defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread) override
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
TestLogger::testDoNotForwardDeletedJob()
{
    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperationImpl( An<KJob*>(), _, _, _, _ ) ).Times( 0 );

    ProgressJob *job = new ProgressJob();
    job->deleteJob = true;
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );

    QTest::qSleep( 10 ); //ensure that the job has time to run
    QTest::qWait( 20 ); //give the Logger-internal timer time to fire

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestLogger::testDoNotForwardDeletedSlot()
{
    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperationImpl( An<KJob*>(), _, nullptr, _, _ ) ).Times( 1 ).WillOnce( Return() );

    ProgressJob *job = new ProgressJob();
    job->deleteObject = true;
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );

    QTest::qSleep( 10 ); //ensure that the job has time to run
    QTest::qWait( 20 ); //give the Logger-internal timer time to fire

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestLogger::testForwardLongMessage()
{
    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, longMessageImpl( _, _ ) ).Times( 1 ).WillOnce( Return() );

    Amarok::Logger::longMessage( "foo", Amarok::Logger::Information );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestLogger::testForwardProgressOperation()
{
    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperationImpl( An<KJob*>(), _, _, _, _ ) ).Times( 1 ).WillOnce( Return() );

    Amarok::Logger::newProgressOperation( new DummyJob(), QString( "foo" ) );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestLogger::testForwardShortMessage()
{
    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, shortMessageImpl( _ ) ).Times( 1 ).WillOnce( Return() );

    Amarok::Logger::shortMessage( "foo" );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

#include "TestLogger.moc"
