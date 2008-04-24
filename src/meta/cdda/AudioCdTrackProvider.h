/* This file is part of the KDE project
   Copyright (C) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef CDDAMANAGER_H
#define CDDAMANAGER_H

#include "meta.h"

#include <QObject>
#include <QStringList>

/**
 * This manager handles audio cds. Audio CDs require special treatment because
 * we are not operating on files, instead we tell the engine to play the cd directly.
 * Therefore CollectionManager::trackForUrl cannot be used.
 */
class CDDAManager : public QObject
{
    Q_OBJECT
    public:
        CDDAManager( QObject *parent );
        ~CDDAManager();

        QStringList audioCdUdis() const;
        QString audioCdName( const QString &udi ) const;
        void playAudioCd( const QString &udi ) const;

    private:
        class Private;
        Private * const d;
};

#endif
