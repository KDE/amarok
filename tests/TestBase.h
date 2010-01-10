/***************************************************************************
 *   Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>            *
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

#ifndef AMAROK_TESTBASE_H
#define AMAROK_TESTBASE_H

#include <QObject>
#include <QString>

class TestBase : public QObject
{
    Q_OBJECT

public:
    TestBase( const QString &name, QObject *parent = 0 );
    ~TestBase() = 0;

protected:
    bool addLogging( QStringList &args, const QString &logPath );
    QString dataPath( const QString &relPath );

private:
    const QString m_name;
};

#endif /* AMAROK_TESTBASE_H */
