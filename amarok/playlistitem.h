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

#include <kurl.h>

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
        PlaylistItem( QListView* parent, QListViewItem* after, const KURL &url );
        ~PlaylistItem();

        // These accessor methods obsolete public fields in next release
        QString title()   { return m_tagTitle; }
        QString artist()  { return m_tagArtist; }
        QString album()   { return m_tagAlbum; }
        QString genre()   { return m_tagGenre; }
        QString comment() { return m_tagComment; }
        QString year()    { return m_tagYear; }
        QString track()   { return m_tagTrack; }
        int     seconds() { return m_tagSeconds; }
        int     bitrate() { return m_tagBitrate; }
        int  samplerate() { return m_tagSamplerate; }
        // These are still here but will be declared private since 0.7
        QString m_tagTitle;
        QString m_tagArtist;
        QString m_tagAlbum;
        QString m_tagGenre;
        QString m_tagComment;
        QString m_tagYear;
        QString m_tagTrack;
        QString m_tagDirectory;
        int m_tagSeconds;
        int m_tagBitrate;
        int m_tagSamplerate;

        KURL url() const { return m_url; }
        void readMetaInfo();
        void setMetaTitle();
        bool hasMetaInfo();
        bool isDir();
        void setDir( bool on );

        bool isGlowing() const  { return m_bIsGlowing; }
        void setGlowing( bool b ) { m_bIsGlowing = b; }
        void setGlowCol( QColor col ) { m_glowCol = col; }

    private:
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
};
#endif
