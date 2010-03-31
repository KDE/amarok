/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#define DEBUG_PREFIX "APG::ConstraintTestWrapper"

#include "ConstraintTester.h"

#include "ConstraintNode.h"
#include "ConstraintTestWrapper.h"
#include "Preset.h"
#include "PresetModel.h"

#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <threadweaver/ThreadWeaver.h>

APG::ConstraintTestWrapper::ConstraintTestWrapper( QObject* parent ) : QObject( parent ) { }

APG::ConstraintTestWrapper::~ConstraintTestWrapper() { }

void
APG::ConstraintTestWrapper::runTest()
{
    Collections::Collection* c = CollectionManager::instance()->primaryCollection();
    ConstraintNode* root = PresetModel::instance()->activePreset()->constraintTreeRoot();
    ConstraintTester* tester = new ConstraintTester( c, root );
    connect( tester, SIGNAL( readyToRun() ), this, SLOT( queueTester() ) );
    connect( tester, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT ( jobDone( ThreadWeaver::Job* ) ) );
}

void
APG::ConstraintTestWrapper::queueTester()
{
    ThreadWeaver::Weaver::instance()->enqueue( static_cast<ConstraintTester*>( sender() ) );
}

void
APG::ConstraintTestWrapper::jobDone( ThreadWeaver::Job* j )
{
    j->deleteLater();
}
