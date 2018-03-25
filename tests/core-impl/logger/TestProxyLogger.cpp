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

#include "TestProxyLogger.h"

#include "core/interfaces/Logger.h"
#include "core-impl/logger/ProxyLogger.h"

#include "mocks/MockLogger.h"

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Job>

#include <QCoreApplication>

#include <gmock/gmock.h>

QTEST_GUILESS_MAIN( TestProxyLogger )

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;
using ::testing::Mock;

static ProxyLogger *s_logger;

class DummyJob : public KJob
{
public:
    virtual void start() {}
};

TestProxyLogger::TestProxyLogger()
{
    int argc = 1;
    char **argv = (char **) malloc(sizeof(char *));
    argv[0] = strdup( QCoreApplication::applicationName().toLocal8Bit().data() );
    ::testing::InitGoogleMock( &argc, argv );
    delete[] argv;
}

void
TestProxyLogger::init()
{
    s_logger = 0;
}

void
TestProxyLogger::cleanup()
{
    delete s_logger;
}

class ProgressJob : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
public:
    ProgressJob() : deleteJob( false ), deleteObject( false ) {}
    void run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
    {
        Q_UNUSED(self);
        Q_UNUSED(thread);
        KJob *job = new DummyJob();
        QObject *obj = new QObject();
        s_logger->newProgressOperation( job, QString( "foo" ), obj );

        if( deleteJob ) delete job;
        if( deleteObject ) delete obj;
    }

    bool deleteJob;
    bool deleteObject;

    protected:
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
TestProxyLogger::testDoNotForwardDeletedJob()
{
    s_logger = new ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperationImpl( An<KJob*>(), _, _, _, _ ) ).Times( 0 );

    s_logger->setLogger( mock );

    ProgressJob *job = new ProgressJob();
    job->deleteJob = true;
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );

    QTest::qSleep( 10 ); //ensure that the job has time to run
    QTest::qWait( 20 ); //give the ProxyLogger-internal timer time to fire

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testDoNotForwardDeletedSlot()
{
    s_logger = new ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperationImpl( An<KJob*>(), _, nullptr, _, _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    ProgressJob *job = new ProgressJob();
    job->deleteObject = true;
    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(job) );

    QTest::qSleep( 10 ); //ensure that the job has time to run
    QTest::qWait( 20 ); //give the ProxyLogger-internal timer time to fire

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testForwardLongMessage()
{
    s_logger = new ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, longMessage( _, _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    s_logger->longMessage( "foo", Amarok::Logger::Information );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testForwardProgressOperation()
{
    s_logger = new ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperationImpl( An<KJob*>(), _, _, _, _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    s_logger->newProgressOperation( new DummyJob(), QString( "foo" ) );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testForwardShortMessage()
{
    s_logger = new ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, shortMessage( _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    s_logger->shortMessage( "foo" );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

#include "TestProxyLogger.moc"
