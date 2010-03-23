/****************************************************************************************
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#ifndef LASTFMSIMILARARTISTSACTION_H
#define LASTFMSIMILARARTISTSACTION_H

#include "GlobalCollectionActions.h"

#include "core/meta/Meta.h"


class SimilarArtistsAction : public GlobalCollectionArtistAction
{
    Q_OBJECT
public:
    SimilarArtistsAction( QObject *parent );

    private slots:
        void slotTriggered();

};

#endif
