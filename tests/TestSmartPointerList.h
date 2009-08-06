/***************************************************************************
 *   Copyright (c) 2009 Max Howell <max@last.fm>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef TESTSMARTPOINTERLIST_H
#define TESTSMARTPOINTERLIST_H

#include <QStringList>
#include <QtTest>

class TestSmartPointerList : public QObject
{
Q_OBJECT

public:
    TestSmartPointerList( QStringList testArgumentList );

private slots:
    void testCount();
    void testCopy();
    void testCopyAndThenDelete();
    void testRemove();
    void testRemoveAt();
    void testMultipleOrgasms();
    void testForeach();
    void testOperatorPlus();
    void testOperatorPlusEquals();
};

#endif // TESTSMARTPOINTERLIST_H
