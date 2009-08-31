/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CDDAManager.h"
#include "CDDAManager_p.h"

CDDAManager::CDDAManager( QObject *parent )
    : QObject( parent )
    , d( new Private() )
{
}

CDDAManager::~CDDAManager()
{
    delete d;
}

QStringList
CDDAManager::audioCdUdis() const
{
    return QStringList();
}

QString
CDDAManager::audioCdName( const QString &udi ) const
{
    return QString();
}

void
CDDAManager::playAudioCd( const QString &udi ) const
{
}

#include "CDDAManager.moc"

