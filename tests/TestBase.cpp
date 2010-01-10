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

#include "TestBase.h"

#include <QStringList>

TestBase::TestBase( const QString &name, QObject *parent )
    : QObject( parent )
    , m_name( name )
{
}

TestBase::~TestBase()
{
}

bool
TestBase::addLogging( QStringList &args, const QString &logPath )
{
    if( logPath.isEmpty() )
        return false;

    const QString ext = args.contains( "-xml" ) ? ".xml" : ".log";
    args << QString( "-o" ) << QString( logPath + m_name + ext );
    return true;
}

#include "TestBase.moc"
