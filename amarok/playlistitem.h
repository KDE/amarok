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

   QString m_title;
   QString m_artist;
   QString m_album;
   QString m_genre;
   QString m_comment;
   QString m_year;
   QString m_track;
   QString m_directory;

   uint m_bitrate;
   uint m_length;
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
        PlaylistItem( QListView* parent, const KURL &url );
        PlaylistItem( QListView* parent, QListViewItem* after, const KURL &url, Tags *tags = 0 );
        ~PlaylistItem();

        // These accessor methods obsolete public fields in next release
        QString title()   { return m_tags->m_title; }
        QString artist()  { return m_tags->m_artist; }
        QString album()   { return m_tags->m_album; }
        QString genre()   { return m_tags->m_genre; }
        QString comment() { return m_tags->m_comment; }
        QString year()    { return m_tags->m_year; }
        QString track()   { return m_tags->m_track; }
        uint    seconds() { return m_tags->m_length; }
        uint    bitrate() { return m_tags->m_bitrate; }
        uint samplerate() { return m_tags->m_sampleRate; }

        QString length( uint = 0 ); //Return track length as mm:ss

        KURL url() const { return m_url; }
        void setMetaTitle();
        bool hasMetaInfo();
        bool isDir();
        void setDir( bool on );

        bool isGlowing() const  { return m_bIsGlowing; }
        void setGlowing( bool b ) { m_bIsGlowing = b; }
        void setGlowCol( QColor col ) { m_glowCol = col; }

    private:
        QString zeroPad( const long digit );
        QString nameForUrl( const KURL &url ) const;
        void init();
        void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align );
        void paintFocus( QPainter*, const QColorGroup&, const QRect& );

        bool m_hasMetaInfo;
        KURL m_url;
        bool m_bIsGlowing;
        bool m_isDir;
        QString m_sPath;
        QColor m_glowCol;
        Tags *m_tags;
};
#endif
