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

#ifndef TESTENGINECONTROLLER_H
#define TESTENGINECONTROLLER_H

#include <QObject>

class TestEngineController : public QObject
{
    Q_OBJECT

    private Q_SLOTS:
        void init(); // called before each test
        void cleanup(); // called after each test

        void testSupportedMimeTypesInMainThread();
        void testSupportedMimeTypesInAnotherThread();
};

#endif // TESTENGINECONTROLLER_H
