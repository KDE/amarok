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

#include <KCmdLineArgs>
#include <KGlobal>
#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Job.h>

#include <QCoreApplication>

#include <qtest_kde.h>

#include <gmock/gmock.h>

QTEST_KDEMAIN_CORE( TestProxyLogger )

using ::testing::Return;
using ::testing::AnyNumber;
using ::testing::_;
using ::testing::Mock;

static Amarok::ProxyLogger *s_logger;

class DummyJob : public KJob
{
public:
    virtual void start() {}
};

TestProxyLogger::TestProxyLogger()
{
    KCmdLineArgs::init( KGlobal::activeComponent().aboutData() );
    ::testing::InitGoogleMock( &KCmdLineArgs::qtArgc(), KCmdLineArgs::qtArgv() );
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

class CreateJob : public ThreadWeaver::Job
{
public:
    void run() {
        s_logger = new Amarok::ProxyLogger();
    }
};

class ProgressJob : public ThreadWeaver::Job
{
public:
    ProgressJob() : deleteJob( false ), deleteObject( false ) {}
    void run() {
        KJob *job = new DummyJob();
        QObject *obj = new QObject();
        s_logger->newProgressOperation( job, QString( "foo" ), obj, "foo()" );

        if( deleteJob ) delete job;
        if( deleteObject ) delete obj;
    }

    bool deleteJob;
    bool deleteObject;
};

void
TestProxyLogger::testClassMovesToMainThread()
{
    CreateJob *job = new CreateJob();
    ThreadWeaver::Weaver::instance()->enqueue( job );
    QTest::kWaitForSignal( job, SIGNAL(done(ThreadWeaver::Job*)), 0 );
    QTest::qWait( 100 );
    QCOMPARE( s_logger->thread(), QCoreApplication::instance()->thread() );
}

void
TestProxyLogger::testDoNotForwardDeletedJob()
{
    s_logger = new Amarok::ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperation( _, _, _, _, _ ) ).Times( 0 );

    s_logger->setLogger( mock );

    ProgressJob *job = new ProgressJob();
    job->deleteJob = true;
    ThreadWeaver::Weaver::instance()->enqueue( job );

    QTest::qSleep( 10 ); //ensure that the job has time to run
    QTest::qWait( 20 ); //give the ProxyLogger-internal timer time to fire

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testDoNotForwardDeletedSlot()
{
    s_logger = new Amarok::ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperation( _, _, 0, 0, _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    ProgressJob *job = new ProgressJob();
    job->deleteObject = true;
    ThreadWeaver::Weaver::instance()->enqueue( job );

    QTest::qSleep( 10 ); //ensure that the job has time to run
    QTest::qWait( 20 ); //give the ProxyLogger-internal timer time to fire

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testForwardLongMessage()
{
    s_logger = new Amarok::ProxyLogger();

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
    s_logger = new Amarok::ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, newProgressOperation( _, _, _, _, _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    s_logger->newProgressOperation( new DummyJob(), QString( "foo" ) );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}

void
TestProxyLogger::testForwardShortMessage()
{
    s_logger = new Amarok::ProxyLogger();

    Amarok::MockLogger *mock = new Amarok::MockLogger();
    EXPECT_CALL( *mock, shortMessage( _ ) ).Times( 1 ).WillOnce( Return() );

    s_logger->setLogger( mock );

    s_logger->shortMessage( "foo" );

    QTest::qWait( 20 );

    QVERIFY( Mock::VerifyAndClearExpectations( &mock ) );
    delete mock;
}
