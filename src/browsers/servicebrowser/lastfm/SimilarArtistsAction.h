/***************************************************************************
 *   Copyright (c) 2008  Dan Meltzer <hydrogen@notyetimplemented.com        *
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

#ifndef LASTFMSIMILARARTISTSACTION_H
#define LASTFMSIMILARARTISTSACTION_H

#include "context/popupdropper/PopupDropperAction.h"

#include "amarok_export.h"
#include "meta/Meta.h"


class AMAROK_EXPORT SimilarArtistsAction : public PopupDropperAction
{
    Q_OBJECT
public:
    SimilarArtistsAction( QObject *parent, Meta::Artist *artist );

    private slots:
        void slotTriggered();

    private:
        Meta::Artist *m_artist;
};

#endif
