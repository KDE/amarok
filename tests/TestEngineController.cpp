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

#include <qtest_kde.h>
#include <ThreadWeaver/Weaver>
#include <ThreadWeaver/Job>

#include <QtTest>

QTEST_KDEMAIN_CORE( TestEngineController )

class CallSupportedMimeTypesJob : public ThreadWeaver::Job
{
    protected:
        void run()
        {
            EngineController *ec = Amarok::Components::engineController();
            QVERIFY( ec );
            QStringList types = ec->supportedMimeTypes();
            QVERIFY( !types.isEmpty() );
        }
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
    if( !ThreadWeaver::Weaver::instance()->isIdle() )
        QVERIFY2( QTest::kWaitForSignal( ThreadWeaver::Weaver::instance(),
                SIGNAL(finished()), 5000 ), "threads did not finish in timeout" );
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
    ThreadWeaver::Job *job = new CallSupportedMimeTypesJob();
    ThreadWeaver::Weaver::instance()->enqueue( job );
}
