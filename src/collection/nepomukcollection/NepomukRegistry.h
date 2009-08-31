/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef NEPOMUKREGISTRY_H
#define NEPOMUKREGISTRY_H

#include "NepomukAlbum.h"
#include "NepomukArtist.h"
#include "NepomukTrack.h"

#include "Meta.h"

#include <QHash>
#include <QTimer>

#include <Soprano/BindingSet>
#include <Soprano/Model>
#include <threadweaver/ThreadWeaver.h>

class NepomukCollection;
class QUrl;

class NepomukRegistry : public QObject
{
    Q_OBJECT
            
    public:
        NepomukRegistry( NepomukCollection *collection, Soprano::Model *model );

        ~NepomukRegistry();
        
        Meta::TrackPtr  trackForBindingSet( const Soprano::BindingSet &set );
        Meta::AlbumPtr albumForArtistAlbum( const QString &artist, const QString &album );
        Meta::ArtistPtr artistForArtistName( const QString &artist );

        // can be used to write no blocking to Nepomuk (should not be used for large jobs)
        void writeToNepomukAsync( Nepomuk::Resource &resource, const QUrl property,  const Nepomuk::Variant value ) const;
        void writeToNepomukAsync( const QString &resourceUri, const QUrl property,  const Nepomuk::Variant value ) const;

    private:
        QString albumId( QString artist, QString album ) const;
        QString createUuid() const;  // create real uuid  the qt one doesn't work (not on linux)
        
    private slots:
        void cleanHash();
        void nepomukUpdate( const Soprano::Statement &statement);
        void jobDone( ThreadWeaver::Job *job );
    
    private:
        QHash< QString, Meta::TrackPtr > m_tracks; // nepomukresource uri -> TrackPtr
        QHash< QString, Meta::TrackPtr > m_tracksFromId; // track uuid -> TrackPtr
        QHash< QString, Meta::AlbumPtr > m_albums; // albumId string -> AlbumPtr
        QHash< QString, Meta::ArtistPtr > m_artists; // artist name -> ArtistPtr
        
        NepomukCollection* m_collection;
        QTimer *m_timer;
        Soprano::Model *m_model;
        mutable ThreadWeaver::Weaver *m_weaver;
};

#endif
