/***************************************************************************
                          playlistitem.h  -  description
                             -------------------
    begin                : Die Dez 3 2002
    copyright            : (C) 2002 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYLISTITEM_H
#define PLAYLISTITEM_H

#include <klistview.h>
#include <kurl.h> //KURL::List
#include <taglib/audioproperties.h>  //Tags

#include "playlistloader.h" //friendships

struct Tags
{
   Tags( const QString &t1, const QString &t2, const QString &t3, const QString &t4, const QString &t5, const QString &t6, const QString &t7, const QString &t8, const TagLib::AudioProperties *ap )
    : m_title( t1 ), m_artist( t2 ), m_album( t3 ), m_genre( t4 ), m_comment( t5 ), m_year( t6 ), m_track( t7 ), m_directory( t8 ),
      m_bitrate( 0 ), m_length( 0 ), m_sampleRate( 0 )
   {
      if( ap )
      {
         m_bitrate    = ap->bitrate();
         m_length     = ap->length();
         m_sampleRate = ap->sampleRate();
      }
   }
   Tags( const QString &title, uint length ) : m_title( title ), m_bitrate( 0 ), m_length( length ), m_sampleRate( 0 ) {}

   QString m_title;
   QString m_artist;
   QString m_album;
   QString m_genre;
   QString m_comment;
   QString m_year;
   QString m_track;
   QString m_directory;

   uint m_bitrate;
   int  m_length; //-1 no established length, eg streams
   uint m_sampleRate;
};



class QColor;
class QColorGroup;
class QListViewItem;
class QPainter;
class QRect;
class QString;

class PlayerApp;
extern PlayerApp *pApp;


class PlaylistItem : public KListViewItem
{
    public:
        PlaylistItem( QListView *, QListViewItem *, const KURL &, Tags * = 0, bool = false );
        virtual ~PlaylistItem();

        const Tags    *tags()     const { return m_tags; }
        const QString  filename() const { return text( 0 ); }
        const QString &title()    const { return m_tags->m_title; }
        const QString &artist()   const { return m_tags->m_artist; }
        const QString &album()    const { return m_tags->m_album; }
        const QString &genre()    const { return m_tags->m_genre; }
        const QString &comment()  const { return m_tags->m_comment; }
        const QString &year()     const { return m_tags->m_year; }
        const QString &track()    const { return m_tags->m_track; }
        const int     &seconds()  const { return m_tags->m_length; } //can be -1
        const uint    &bitrate()  const { return m_tags->m_bitrate; }
        const uint  &sampleRate() const { return m_tags->m_sampleRate; }

        const QString length( uint = 0 ) const; //Return track length as mm:ss

        const KURL &url()   const { return m_url; }
        const bool &isDir() const { return m_isDir; }        
        bool hasMetaInfo()  const { return ( m_tags != 0 ); }
        void setDir( bool b )     { m_isDir = b; }        
        void setMetaTitle();

        friend void TagReader::append( PlaylistItem * );
        friend void TagReader::TagReaderEvent::bindTags();

        //save memory, use a single static to represent these properties
        static QColor GlowColor;
        static PlaylistItem *GlowItem;

    private:
        void paintCell( QPainter*, const QColorGroup&, int, int, int );
        void paintFocus( QPainter*, const QColorGroup&, const QRect& );

        const KURL m_url;
        bool m_isDir;
        Tags *m_tags;
};

#endif
