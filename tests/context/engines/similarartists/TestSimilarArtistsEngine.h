/****************************************************************************************
* Copyright (c) 2009 Manuel Campomanes <campomanes.manuel@gmail.com>                               *
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

#ifndef TESTSIMILARARTISTSENGINE_H
#define TESTSIMILARARTISTSENGINE_H

#include <tests/context/engines/TestDataEngine.h>
#include <TestBase.h>

#include <QtCore/QStringList>

class TestSimilarArtistsEngine : public TestBase, public TestDataEngine
{
    Q_OBJECT
    
    public:      
	TestSimilarArtistsEngine( const QStringList args, const QString &logPath );	
        
    private slots:
        void initTestCase();
        void testDataEngineMethod();

};

#endif // TESTSIMILARARTISTSENGINE_H