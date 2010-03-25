/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestPlaylistModels.h"

#include "Components.h"
#include "EngineController.h"

#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistModel.h"
#include "playlist/UndoCommands.h"

#include "mocks/MetaMock.h"
#include "mocks/MockTrack.h"

#include <KStandardDirs>

#include <QtDebug>
#include <QtTest/QTest>
#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestPlaylistModels )


TestPlaylistModels::TestPlaylistModels() 
    : QObject()
{
}

void TestPlaylistModels::initTestCase()
{
  
    //apparently the engine controller is needed somewhere, or we will get a crash...
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );
    bool invoked = QMetaObject::invokeMethod( controller, "initializePhonon", Qt::DirectConnection );
    Q_ASSERT( invoked );
  
    //we want to add a few tracks to the playlist so we can test sorting, filtering and so on. So first create a bunch of dummy tracks we can use.
    
    Meta::TrackList tracks;
    
    QVariantMap map1;
    map1.insert( Meta::Field::TITLE,  QString( "Cool as honey" ) );
    map1.insert( Meta::Field::ARTIST,  QString( "Bonzai Bees" ) );
    map1.insert( Meta::Field::ALBUM,  QString( "The Hive" ) );
    tracks << Meta::TrackPtr( new MetaMock( map1 ) );
    
    QVariantMap map2;
    map2.insert( Meta::Field::TITLE,  QString( "xTreme buzzing sound" ) );
    map2.insert( Meta::Field::ARTIST,  QString( "Bonzai Bees" ) );
    map2.insert( Meta::Field::ALBUM,  QString( "The Hive" ) );
    tracks << Meta::TrackPtr( new MetaMock( map2 ) );
    
    QVariantMap map3;
    map3.insert( Meta::Field::TITLE,  QString( "Alphabet soup" ) );
    map3.insert( Meta::Field::ARTIST,  QString( "Bonzai Bees" ) );
    map3.insert( Meta::Field::ALBUM,  QString( "The Hive" ) );
    tracks << Meta::TrackPtr( new MetaMock( map3 ) );
    
    QVariantMap map4;
    map4.insert( Meta::Field::TITLE,  QString( "Zlick" ) );
    map4.insert( Meta::Field::ARTIST,  QString( "Grumpy Grizzlies" ) );
    map4.insert( Meta::Field::ALBUM,  QString( "Nice Long Nap" ) );
    tracks << Meta::TrackPtr( new MetaMock( map4 ) );
    
    QVariantMap map5;
    map5.insert( Meta::Field::TITLE,  QString( "23 hours is not enough " ) );
    map5.insert( Meta::Field::ARTIST,  QString( "Grumpy Grizzlies" ) );
    map4.insert( Meta::Field::ALBUM,  QString( "Nice Long Nap" ) );
    tracks << Meta::TrackPtr( new MetaMock( map5 ) );
    
    QVariantMap map6;
    map6.insert( Meta::Field::TITLE,  QString( "1 song to rule them all" ) );
    map6.insert( Meta::Field::ARTIST,  QString( "Bonzai Bees" ) );
    map6.insert( Meta::Field::ALBUM,  QString( "Pretty Flowers" ) );
    tracks << Meta::TrackPtr( new MetaMock( map6 ) );
      
    Playlist::InsertCmdList insertCmds;
    int row = 0;
    foreach( Meta::TrackPtr t, tracks )
    {
        insertCmds.append( Playlist::InsertCmd( t, row ) );
	row++;
    }
    
    Playlist::Model * model = Playlist::ModelStack::instance()->bottom();
    model->insertTracksCommand( insertCmds );
    
    Playlist::AbstractModel * topModel = Playlist::ModelStack::instance()->top();
    
    QCOMPARE( topModel->trackAt( 3 )->prettyName(), QString( "Zlick" ) );
}

void TestPlaylistModels::testSorting()
{
}

void TestPlaylistModels::testFiltering()
{
}

void TestPlaylistModels::testSearching()
{
}
