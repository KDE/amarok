/***************************************************************************
                           track.h  -  description
                             -------------------
    begin                : 2001-07-24
    copyright            : (C) 2001 by Shaun Jackman (sjackman@debian.org)
    modify by            : Andres Oton 
    email                : andres.oton@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __track_h__
#define __track_h__


// qt
#include <qstring.h>

// libnjb
#include <libnjb.h>

#include "metabundle.h"

class NjbMediaItem;

class NjbTrack {
    friend class NjbMediaItem;
    public:
        NjbTrack()
            : m_id( 0 )
            , m_bundle()
        { }
        NjbTrack( njb_songid_t* song );
        ~NjbTrack();

        bool operator==( const NjbTrack& second ) const { return m_id == second.m_id; }

    public:
        unsigned int id() const { return m_id; }
        MetaBundle* bundle() { return new MetaBundle( m_bundle ); }
        const MetaBundle & getBundle() { return m_bundle; }
        void setBundle( MetaBundle &bundle );
        void addItem( const NjbMediaItem *item );
        bool removeItem( const NjbMediaItem * );
        void setId( int id ) { m_id = id; }
        void writeToSongid( njb_songid_t *songid );
        njb_songid_t *newSongid();
    private:
        unsigned int m_id;
        MetaBundle m_bundle;
        QPtrList<NjbMediaItem> ItemList;
};

class trackValueList: public QValueList<NjbTrack *>
{
    public:
        trackValueList::iterator findTrackByName( const QString& );
        trackValueList::const_iterator findTrackByName( const QString& ) const;
        trackValueList::iterator findTrackById( unsigned );
        trackValueList::const_iterator findTrackById( unsigned ) const;

        int readFromDevice( void );
};

#endif
