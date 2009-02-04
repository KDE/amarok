/*
    Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MYDIRLISTER_H
#define MYDIRLISTER_H

//TODO wait for lister to finish, if there are no files shown, but there are
//     media files in that directory show a longMessage (preferably one that disappears when given a signal)

#include "EngineController.h"

#include <kdirlister.h>
#include <kfileitem.h>

class MyDirLister : public KDirLister
{
    Q_OBJECT

public:
    MyDirLister( bool delayedMimeTypes ) : KDirLister() { setDelayedMimeTypes( delayedMimeTypes ); setAutoUpdate( true ); }
    ~MyDirLister();

protected:
    virtual bool matchesFilter( const KFileItem &item ) const;
};

#endif
