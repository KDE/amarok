/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

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
#ifndef NEPOMUKREGISTRY_H
#define NEPOMUKREGISTRY_H

#include "NepomukCollection.h"
#include "NepomukMeta.h"

#include <QHash>
#include <QTimer>

#include <Soprano/BindingSet>
#include <Soprano/Model>

class QUrl;

class NepomukRegistry : public QObject
{
    Q_OBJECT
            
    public:
        NepomukRegistry( NepomukCollection *collection, Soprano::Model *model );

        ~NepomukRegistry();
        
        Meta::TrackPtr  trackForBindingSet( const Soprano::BindingSet &set );

    private slots:
        void cleanHash();
        void nepomukUpdate( const Soprano::Statement &statement);
    
    private:
        QHash< QString, Meta::NepomukTrackPtr > m_tracks;
        NepomukCollection* m_collection;
        QTimer *m_timer;
        Soprano::Model *m_model;
};

#endif
