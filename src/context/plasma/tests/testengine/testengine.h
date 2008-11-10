/*
 *   Copyright (C) 2008 Gilles CHAUVIN <gcnweb+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License either version 2, or
 *   (at your option) any later version as published by the Free Software
 *   Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 *  A data engine meant to test the Plasma data engine explorer.
 */

#ifndef __TESTDATAENGINE_H__
#define __TESTDATAENGINE_H__


#include "plasma/dataengine.h"


class TestEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    struct MyUserType {
        int     a;
        QString b;
    };

    TestEngine(QObject *parent, const QVariantList &args);
    ~TestEngine();

protected:
    void init();
    bool sourceRequestEvent(const QString &source);
};


K_EXPORT_PLASMA_DATAENGINE(testengine, TestEngine)


#endif // __TESTDATAENGINE_H__
