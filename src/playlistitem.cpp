/***************************************************************************
                        playlistitem.cpp  -  description
                           -------------------
  begin                : Die Dez 3 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
  copyright            : (C) 2005 by Gav Wood
  email                : gav@kde.org
  copyright            : (C) 2005 by Alexandre Oliveira
  email                : aleprj@gmail.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "PlaylistItem"

#include <config.h>
#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"
#include "playlist.h"
#include "sliderwidget.h"
#include "moodbar.h"

#include <qcursor.h>
#include <qheader.h>
#include <qimage.h>
#include <qmutex.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qrect.h>

#include <kdeversion.h>
#include <kfilemetainfo.h>
#include <kglobal.h>
#include <kiconeffect.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>

#include "playlistitem.h"

double  PlaylistItem::glowIntensity;
QColor  PlaylistItem::glowText = Qt::white;
QColor  PlaylistItem::glowBase = Qt::white;
bool    PlaylistItem::s_pixmapChanged = false;


PlaylistItem::PlaylistItem( QListView *listview, QListViewItem *item )
        : KListViewItem( listview, item )
{
    KListViewItem::setVisible( false );
}

PlaylistItem::PlaylistItem( const MetaBundle &bundle, QListViewItem *lvi, bool enabled )
        : MetaBundle( bundle ), KListViewItem( lvi->listView(), lvi->itemAbove() )
        , m_proxyForMoods( 0 )
{
    setDragEnabled( true );

    if( AmarokConfig::showMoodbar() )
        checkMood();

    refAlbum();

    listView()->m_totalCount++;
    listView()->m_totalLength += length();
    if( isSelected() )
    {
        listView()->m_selCount++;
        listView()->m_selLength += length();
    }
    if( isVisible() )
    {
        listView()->m_visCount++;
        listView()->m_visLength += length();
        incrementTotals();
    }

    filter( listView()->m_filter );

    listView()->countChanged();
    setEnabled( enabled );
}

PlaylistItem::~PlaylistItem()
{
    if( isEmpty() ) //constructed with the generic constructor, for PlaylistLoader's marker item
        return;

    listView()->m_totalCount--;
    listView()->m_totalLength -= length();
    if( isSelected() )
    {
        listView()->m_selCount--;
        listView()->m_selLength -= length();
    }
    if( isVisible() )
    {
        listView()->m_visCount--;
        listView()->m_visLength -= length();
        decrementTotals();
    }

    derefAlbum();

    listView()->countChanged();

    if( listView()->m_hoveredRating == this )
        listView()->m_hoveredRating = 0;

    delete m_proxyForMoods;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////


void PlaylistItem::setArray(const QValueVector<QColor> array)
{
    QMutexLocker lock(&theArrayLock);
    theArray = array;
}

class PlaylistItem::ReadMood : public ThreadWeaver::DependentJob
{
    QString thePath;
    QValueVector<QColor> theArray;
public:
    ReadMood( MoodProxyObject *o ): DependentJob(o, "ReadMood"), thePath( o->item->url().path()) {}
    virtual bool doJob();
    virtual void completeJob();
};

void PlaylistItem::ReadMood::completeJob()
{
    if(MoodProxyObject *o = dynamic_cast<MoodProxyObject *>(dependent()))
    {
        if(theArray.size())
        {
            o->item->setArray(theArray);
            o->item->refreshMood();
        }
        #ifdef HAVE_EXSCALIBAR
        else
        {
            amaroK::CreateMood *c = new amaroK::CreateMood( thePath );
            Playlist::instance()->connect(c, SIGNAL(completed(const QString)), SLOT(fileHasMood( const QString )));
            ThreadWeaver::instance()->queueJob( c );
        }
        #endif
        o->item->m_proxyForMoods = 0;
        o->deleteLater();
    }
}

bool PlaylistItem::ReadMood::doJob()
{
    // attempt to open .mood file:
    theArray = amaroK::readMood(thePath);
    return true;
}

void PlaylistItem::refreshMood()
{
    theMoodbar.resize( 1, height() );
    repaint();
}

void PlaylistItem::checkMood()
{
    Playlist *pl = Playlist::instance();
    if( url().isLocalFile() && (pl && pl->columnWidth( Mood ) > 0) )
    {
        m_proxyForMoods = new MoodProxyObject( this );
        ReadMood *c = new ReadMood( m_proxyForMoods );
        ThreadWeaver::instance()->queueJob( c );
    }
}

void PlaylistItem::setText( int column, const QString &text )
{
    if( column == Rating )
        setExactText( column, QString::number( int( text.toFloat() * 2 ) ) );
    else
        setExactText( column, text );
}

QString PlaylistItem::text( int column ) const
{
    if( column == Title && listView()->header()->sectionSize( Filename ) ) //don't show the filename twice
        return exactText( column );
    else switch ( column )
    {
        case Artist:
        case Composer:
        case Album:
        case Genre:
        case Comment:
            return exactText( column ); //HACK
        case Rating:
            return isEditing( column ) ? exactText( column ) : prettyText( column );
        default:
        {
            if( column != Title && isEditing( column ) )
                return editingText();
            else
                return prettyText( column );
        }
    }
}

void PlaylistItem::aboutToChange( const QValueList<int> &columns )
{
    bool totals = false, ref = false;
    for( int i = 0, n = columns.count(); i < n; ++i )
        switch( columns[i] )
        {
            case Artist: case Album: ref = true; //note, no breaks
            case Track: case Rating: case Score: case LastPlayed: totals = true;
        }
    if( totals )
        decrementTotals();
    if( ref )
        derefAlbum();
}

void PlaylistItem::reactToChanges( const QValueList<int> &columns )
{
    bool totals = false, ref = false;
    for( int i = 0, n = columns.count(); i < n; ++i )
        switch( columns[i] )
        {
            case Artist: case Album: ref = true; //note, no breaks
            case Track: case Rating: case Score: case LastPlayed: totals = true;
            default: updateColumn( columns[i] );
        }
    if( ref )
        refAlbum();
    if( totals )
        incrementTotals();
}

void PlaylistItem::filter( const QString &expression )
{
    setVisible( matchesExpression( expression, listView()->visibleColumns() ) );
}

bool PlaylistItem::isCurrent() const
{
    return this == listView()->currentTrack();
}

bool PlaylistItem::isQueued() const
{
    return queuePosition() != -1;
}

int PlaylistItem::queuePosition() const
{
    return listView()->m_nextTracks.findRef( this );
}

void PlaylistItem::setEnabled( bool enabled )
{
    m_enabled = enabled;
    setDropEnabled( enabled ); // this forbids items to be dropped into a history queue.

    repaint();
}

void PlaylistItem::setSelected( bool selected )
{
    if( isEmpty() )
        return;

    if( isVisible() )
    {
        const bool prevSelected = isSelected();
        KListViewItem::setSelected( selected );
        if( prevSelected && !isSelected() )
        {
            listView()->m_selCount--;
            listView()->m_selLength -= length();
            listView()->countChanged();
        }
        else if( !prevSelected && isSelected() )
        {
            listView()->m_selCount++;
            listView()->m_selLength += length();
            listView()->countChanged();
        }
    }
}

void PlaylistItem::setVisible( bool visible )
{
    if( isEmpty() )
        return;

    if( !visible && isSelected() )
    {
        listView()->m_selCount--;
        listView()->m_selLength -= length();
        KListViewItem::setSelected( false );
        listView()->countChanged();
    }

    const bool prevVisible = isVisible();
    KListViewItem::setVisible( visible );
    if( prevVisible && !isVisible() )
    {
        listView()->m_visCount--;
        listView()->m_visLength -= length();
        listView()->countChanged();
        decrementTotals();
    }
    else if( !prevVisible && isVisible() )
    {
        listView()->m_visCount++;
        listView()->m_visLength += length();
        listView()->countChanged();
        incrementTotals();
    }
}

void PlaylistItem::setEditing( int column )
{
    switch( column )
    {
        case Title:
        case Artist:
        case Composer:
        case Album:
        case Genre:
        case Comment:
            setExactText( column, editingText() );
            break;
        case Year:       m_year       = -1; break;
        case DiscNumber: m_discNumber = -1; break;
        case Track:      m_track      = -1; break;
        case Length:     m_length     = -1; break;
        case Bitrate:    m_bitrate    = -1; break;
        case SampleRate: m_sampleRate = -1; break;
        case Score:      m_score      = -1; break;
        case Rating:     m_rating     = -1; break;
        case PlayCount:  m_playCount  = -1; break;
        case LastPlayed: m_lastPlay   =  1; break;
        default: warning() << "Tried to set the text of an immutable or nonexistent column!" << endl;
    }

    update();
}

bool PlaylistItem::isEditing( int column ) const
{
    switch( column )
    {
        case Title:
        case Artist:
        case Composer:
        case Album:
        case Genre:
        case Comment: //FIXME fix this hack!
            return exactText( column ) == editingText();
        case Year:       return m_year       == -1;
        case DiscNumber: return m_discNumber == -1;
        case Track:      return m_track      == -1;
        case Length:     return m_length     == -1;
        case Bitrate:    return m_bitrate    == -1;
        case SampleRate: return m_sampleRate == -1;
        case Score:      return m_score      == -1;
        case Rating:     return m_rating     == -1;
        case PlayCount:  return m_playCount  == -1;
        case LastPlayed: return m_lastPlay   ==  1;
        default: return false;
    }
}

int PlaylistItem::ratingAtPoint( int x ) //static
{
    Playlist* const pl = Playlist::instance();
    x -= pl->header()->sectionPos( Rating );
    return kClamp( ( x - 1 ) / ( star()->width() + pl->itemMargin() ) + 1, 1, 5 ) * 2;
}

int PlaylistItem::ratingColumnWidth() //static
{
    return star()->width() * 5 + Playlist::instance()->itemMargin() * 6;
}

void PlaylistItem::update() const
{
    listView()->repaintItem( this );
}

void PlaylistItem::updateColumn( int column ) const
{
    const QRect r = listView()->itemRect( this );
    if( !r.isValid() )
        return;

    listView()->viewport()->update( listView()->header()->sectionPos( column ) - listView()->contentsX() + 1,
                                    r.y() + 1,
                                    listView()->header()->sectionSize( column ) - 2, height() - 2 );
}

bool
PlaylistItem::operator== ( const PlaylistItem & item ) const
{
    return item.url() == this->url();
}

bool
PlaylistItem::operator< ( const PlaylistItem & item ) const
{
    return item.url() < this->url();
}

PlaylistItem*
PlaylistItem::nextInAlbum() const
{
    const int index = m_album->tracks.findRef( this );
    if( index == int(m_album->tracks.count() - 1) )
        return 0;
    if( index != -1 )
        return m_album->tracks.at( index + 1 );
    if( track() )
        for( int i = 0, n = m_album->tracks.count(); i < n; ++i )
            if( m_album->tracks.at( i )->track() > track() )
                return m_album->tracks.at( i );
    else
        for( QListViewItemIterator it( const_cast<PlaylistItem*>(this), QListViewItemIterator::Visible ); *it; ++it )
            #define pit static_cast<PlaylistItem*>( *it )
            if( pit != this && pit->m_album == m_album && !pit->track() )
                return pit;
            #undef pit
    return 0;
}

PlaylistItem*
PlaylistItem::prevInAlbum() const
{
    const int index = m_album->tracks.findRef( this );
    if( index == 0 )
        return 0;
    if( index != -1 )
        return m_album->tracks.at( index - 1 );
    if( track() )
        for( int i = m_album->tracks.count() - 1; i >= 0; --i )
            if( m_album->tracks.at( i )->track() && m_album->tracks.at( i )->track() < track() )
                return m_album->tracks.at( i );
    else
        for( QListViewItemIterator it( const_cast<PlaylistItem*>(this), QListViewItemIterator::Visible ); *it; --it )
            #define pit static_cast<PlaylistItem*>( *it )
            if( pit != this && pit->m_album == m_album && !pit->track() )
                return pit;
            #undef pit
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

int
PlaylistItem::compare( QListViewItem *i, int col, bool ascending ) const
{
    //damn C++ and its lack of operator<=>
    #define cmp(a,b) ( (a < b ) ? -1 : ( a > b ) ? 1 : 0 )
    #define i static_cast<PlaylistItem*>(i)
    switch( col )
    {
        case Track:      return cmp( track(),     i->track() );
        case Score:      return cmp( score(),     i->score() );
        case Rating:     return cmp( rating(),    i->rating() );
        case Length:     return cmp( length(),    i->length() );
        case PlayCount:  return cmp( playCount(), i->playCount() );
        case LastPlayed: return cmp( lastPlay(),  i->lastPlay() );
        case Bitrate:    return cmp( bitrate(),   i->bitrate() );
        case Filesize:   return cmp( filesize(),  i->filesize() );
        case Mood:       return cmp( theHueOrder, i->theHueOrder );
        case Year:
            if( year() == i->year() )
                return compare( i, Artist, ascending );
            return cmp( year(), i->year() );
        case DiscNumber:
            if( discNumber() == i->discNumber() )
                return compare( i, Track, true ) * (ascending ? 1 : -1);
            return cmp( discNumber(), i->discNumber() );
    }
    #undef cmp
    #undef i

    QString a =    text( col ).lower();
    QString b = i->text( col ).lower();

    switch( col )
    {
        case Type:
            a = a.rightJustify( b.length(), '0' );
            b = b.rightJustify( a.length(), '0' );
            break;

        case Artist:
            if( a == b ) //if same artist, try to sort by album
                return compare( i, Album, ascending );
            break;

        case Album:
            if( a == b ) //if same album, try to sort by track
                //TODO only sort in ascending order?
                return compare( i, DiscNumber, true ) * (ascending ? 1 : -1);
            break;
    }

    return QString::localeAwareCompare( a, b );
}

void PlaylistItem::paintCell( QPainter *painter, const QColorGroup &cg, int column, int width, int align )
{
    //TODO add spacing on either side of items
    //p->translate( 2, 0 ); width -= 3;

    // Don't try to draw if width or height is 0, as this crashes Qt
    if( !painter || !listView() || width <= 0 || height() == 0 )
        return;

    static const QImage currentTrackLeft  = locate( "data", "amarok/images/currenttrack_bar_left.png" );
    static const QImage currentTrackMid   = locate( "data", "amarok/images/currenttrack_bar_mid.png" );
    static const QImage currentTrackRight = locate( "data", "amarok/images/currenttrack_bar_right.png" );

    const QString colText = text( column );
    const bool isCurrent = this == listView()->currentTrack();

    QPixmap buf( width, height() );
    QPainter p( &buf, true );

    QMutexLocker lock(&theArrayLock);
    if( column == Mood && theArray.size() && length() > 0 )
    {
        if(theMoodbar.width() != width || theMoodbar.height() != height())
        {
            theMoodbar.resize(width, height());
            QPainter paint(&theMoodbar);
            // paint the moodbar
            int samples = width;
            int aSize = theArray.size() * 180 / length();
            int quantise = 12;
            QMemArray<int> modalHue(quantise);
            for(int i = 0; i < quantise; i++) modalHue[i] = 0;
            for(int x = 0; x < width; x++)
            {
                uint a = x * aSize / samples, aa = ((x + 1) * aSize / samples);
                if(a == aa) aa = a + 1;
                float r = 0., g = 0., b = 0.;
                for(uint j = a; j < aa; j++)
                    if(j < theArray.size())
                    {
                        r += theArray[j].red();
                        g += theArray[j].green();
                        b += theArray[j].blue();
                    }
                    else
                    {
                        r += 220;
                        g += 220;
                        b += 220;
                    }
                int h, s, v;
                QColor(CLAMP(0, int(r / float(aa - a)), 255), CLAMP(0, int(g / float(aa - a)), 255), CLAMP(0, int(b / float(aa - a)), 255), QColor::Rgb).getHsv(&h, &s, &v);
                modalHue[CLAMP(0, h * quantise / 360, quantise - 1)] += v;
                for(int y = 0; y <= height() / 2; y++)
                {
                    float coeff = float(y) / float(height() / 2);
                    float coeff2 = 1.f - ((1.f - coeff) * (1.f - coeff));
                    coeff = 1.f - (1.f - coeff) / 2.f;
                    coeff2 = 1.f - (1.f - coeff2) / 2.f;
                    paint.setPen( QColor(h, CLAMP(0, int(s * coeff), 255), CLAMP(0, int(255 - (255 - v) * coeff2), 255), QColor::Hsv) );
                    paint.drawPoint(x, y);
                    paint.drawPoint(x, height() - 1 - y);
                }
            }
            int mx = 0;
            for(int i = 1; i < quantise; i++) if(modalHue[i] > modalHue[mx]) mx = i;
            theHueOrder = mx * quantise * quantise;
            modalHue[mx] = 0;
            for(int i = 0; i < quantise; i++) if(modalHue[i] > modalHue[mx]) mx = i;
            theHueOrder += mx * quantise;
            modalHue[mx] = 0;
            for(int i = 0; i < quantise; i++) if(modalHue[i] > modalHue[mx]) mx = i;
            theHueOrder += mx;
        }
        p.drawPixmap( 0, 0, theMoodbar );
    }
    else
    {
        if( isCurrent )
        {
            static paintCacheItem paintCache[NUM_COLUMNS];

            // Convert intensity to string, so we can use it as a key
            const QString colorKey = QString::number( glowIntensity );

            const bool cacheValid =
                paintCache[column].width == width &&
                paintCache[column].height == height() &&
                paintCache[column].text == colText &&
                paintCache[column].font == painter->font() &&
                paintCache[column].color == glowBase &&
                paintCache[column].selected == isSelected() &&
                !s_pixmapChanged;

            // If any parameter changed, we must regenerate all pixmaps
            if ( !cacheValid )
            {
                for( int i = 0; i < NUM_COLUMNS; ++i)
                    paintCache[i].map.clear();
                s_pixmapChanged = false;
            }

            // Determine if we need to repaint the pixmap, or paint from cache
            if ( paintCache[column].map.find( colorKey ) == paintCache[column].map.end() )
            {
                // Update painting cache
                paintCache[column].width = width;
                paintCache[column].height = height();
                paintCache[column].text = colText;
                paintCache[column].font = painter->font();
                paintCache[column].color = glowBase;
                paintCache[column].selected = isSelected();

                QColor bg;
                if( isSelected() )
                    bg = listView()->colorGroup().highlight();
                else
                    bg = isAlternate() ? listView()->alternateBackground() :
                                         listView()->viewport()->backgroundColor();

                buf.fill( bg );

                // Draw column divider line
                p.setPen( listView()->viewport()->colorGroup().mid() );
                p.drawLine( width - 1, 0, width - 1, height() - 1 );

                // Here we draw the background bar graphics for the current track:
                //
                // Illustration of design, L = Left, M = Middle, R = Right:
                // <LMMMMMMMMMMMMMMMR>

                int leftOffset  = 0;
                int rightOffset = 0;
                int margin      = listView()->itemMargin();

                const float  colorize  = 0.8;
                const double intensity = 1.0 - glowIntensity * 0.021;

                // Left part
                if( column == listView()->m_firstColumn ) {
                    QImage tmpImage = currentTrackLeft.smoothScale( 1, height(), QImage::ScaleMax );
                    KIconEffect::colorize( tmpImage, glowBase, colorize );
                    imageTransparency( tmpImage, intensity );
                    p.drawImage( 0, 0, tmpImage, 0, 0, tmpImage.width() - 1 ); //HACK
                    leftOffset = tmpImage.width() - 1; //HACK Subtracting 1, to work around the black line bug
                    margin += 6;
                }

                // Right part
                else
                if( column == Playlist::instance()->mapToLogicalColumn( Playlist::instance()->numVisibleColumns() - 1 ) )
                {
                    QImage tmpImage = currentTrackRight.smoothScale( 1, height(), QImage::ScaleMax );
                    KIconEffect::colorize( tmpImage, glowBase, colorize );
                    imageTransparency( tmpImage, intensity );
                    p.drawImage( width - tmpImage.width(), 0, tmpImage );
                    rightOffset = tmpImage.width();
                    margin += 6;
                }

                // Middle part
                // Here we scale the one pixel wide middel image to stretch to the full column width.
                QImage tmpImage = currentTrackMid.copy();
                KIconEffect::colorize( tmpImage, glowBase, colorize );
                imageTransparency( tmpImage, intensity );
                tmpImage = tmpImage.smoothScale( width - leftOffset - rightOffset, height() );
                p.drawImage( leftOffset, 0, tmpImage );


                // Draw the pixmap, if present
                int leftMargin = margin;
                if ( pixmap( column ) ) {
                    p.drawPixmap( leftMargin, height() / 2 - pixmap( column )->height() / 2, *pixmap( column ) );
                    leftMargin += pixmap( column )->width() + 2;
                }

                if( align != Qt::AlignCenter )
                   align |= Qt::AlignVCenter;

                if( column != Rating )
                {
                    // Draw the text
                    static QFont font;
                    static int minbearing = 1337 + 666;
                    if( minbearing == 2003 || font != painter->font() )
                    {
                        font = painter->font();
                        minbearing = painter->fontMetrics().minLeftBearing()
                                     + painter->fontMetrics().minRightBearing();
                    }
                    const bool italic = font.italic();
                    font.setItalic( !italic );
                    p.setFont( font );
                    p.setPen( cg.highlightedText() );
//                  paint.setPen( glowText );
                    const int _width = width - leftMargin - margin + minbearing - 1; // -1 seems to be necessary
                    const QString _text = KStringHandler::rPixelSqueeze( colText, painter->fontMetrics(), _width );
                    p.drawText( leftMargin, 0, _width, height(), align, _text );
                    font.setItalic( italic );
                    p.setFont( font );
                }

                paintCache[column].map[colorKey] = buf;
            }
            else
                p.drawPixmap( 0, 0, paintCache[column].map[colorKey] );
            if( column == Rating )
                drawRating( &p );
        }
        else
        {
            const QColorGroup _cg = ( !exists() || !isEnabled() )
                                  ? listView()->palette().disabled()
                                  : listView()->palette().active();

            QColor bg = isSelected()  ? _cg.highlight()
                        : isAlternate() ? listView()->alternateBackground()
                        : listView()->viewport()->backgroundColor();
            #if KDE_IS_VERSION( 3, 3, 91 )
            if( listView()->shadeSortColumn() && !isSelected() && listView()->columnSorted() == column )
            {
                /* from klistview.cpp
                  Copyright (C) 2000 Reginald Stadlbauer <reggie@kde.org>
                  Copyright (C) 2000,2003 Charles Samuels <charles@kde.org>
                  Copyright (C) 2000 Peter Putzer */
                if ( bg == Qt::black )
                    bg = QColor(55, 55, 55);  // dark gray
                else
                {
                    int h,s,v;
                    bg.hsv(&h, &s, &v);
                    if ( v > 175 )
                        bg = bg.dark(104);
                    else
                       bg = bg.light(120);
                }
            }
            #endif

            const QColor textc = isSelected() ? _cg.highlightedText()
                               : _cg.text();

            buf.fill( bg );

            // Draw column divider line
            if( !isSelected() )
            {
                p.setPen( listView()->viewport()->colorGroup().mid() );
                p.drawLine( width - 1, 0, width - 1, height() - 1 );
            }

            // Draw the pixmap, if present
            int margin = listView()->itemMargin(), leftMargin = margin;
            if ( pixmap( column ) ) {
                p.drawPixmap( leftMargin, height() / 2 - pixmap( column )->height() / 2, *pixmap( column ) );
                leftMargin += pixmap( column )->width();
            }

            if( align != Qt::AlignCenter )
                align |= Qt::AlignVCenter;

            if( column == Rating )
                drawRating( &p );
            else
            {
                // Draw the text
                static QFont font;
                static int minbearing = 1337 + 666; //can be 0 or negative, 2003 is less likely
                if( minbearing == 2003 || font != painter->font() )
                {
                    font = painter->font(); //getting your bearings can be expensive, so we cache them
                    minbearing = painter->fontMetrics().minLeftBearing()
                                 + painter->fontMetrics().minRightBearing();
                }
                p.setFont( font );
                p.setPen( textc );
                const int _width = width - leftMargin - margin + minbearing - 1; // -1 seems to be necessary
                const QString _text = KStringHandler::rPixelSqueeze( colText, painter->fontMetrics(), _width );
                p.drawText( leftMargin, 0, _width, height(), align, _text );
            }
        }
    }
    /// Track action symbols
    const int  queue       = listView()->m_nextTracks.findRef( this ) + 1;
    const bool stop        = ( this == listView()->m_stopAfterTrack );
    const bool repeat      = amaroK::repeatTrack() && isCurrent;

    const uint num = ( queue ? 1 : 0 ) + ( stop ? 1 : 0 ) + ( repeat ? 1 : 0 );

    static const QPixmap pixstop   = amaroK::getPNG(  "currenttrack_stop_small"  ),
                         pixrepeat = amaroK::getPNG( "currenttrack_repeat_small" );

    //figure out if we are in the actual physical first column
    if( column == listView()->m_firstColumn && num )
    {
        //margin, height
        const uint m = 2, h = height() - m;

        const QString str = QString::number( queue );

        const uint qw = painter->fontMetrics().width( str ), sw = pixstop.width(),  rw = pixrepeat.width(),
                   qh = painter->fontMetrics().height(),     sh = pixstop.height(), rh = pixrepeat.height();

        //maxwidth
        const uint mw = kMax( qw, kMax( rw, sw ) );

        //width of first & second column of pixmaps
        const uint w1 = ( num == 3 ) ? kMax( qw, rw )
                      : ( num == 2 && isCurrent ) ? kMax( repeat ? rw : 0, kMax( stop ? sw : 0, queue ? qw : 0 ) )
                      : ( num == 2 ) ? qw
                      : queue ? qw : repeat ? rw : stop ? sw : 0,
                   w2 = ( num == 3 ) ? sw
                      : ( num == 2 && !isCurrent ) ? sw
                      : 0; //phew

        //ellipse width, total width
        const uint ew = 16, tw = w1 + w2 + m * ( w2 ? 2 : 1 );
        p.setBrush( cg.highlight() );
        p.setPen( cg.highlight().dark() ); //TODO blend with background color
        p.drawEllipse( width - tw - ew/2, m / 2, ew, h );
        p.drawRect( width - tw, m / 2, tw, h );
        p.setPen( cg.highlight() );
        p.drawLine( width - tw, m/2 + 1, width - tw, h - m/2 );

        int x = width - m - mw, y = height() / 2, tmp = 0;
        const bool multi = ( isCurrent && num >= 2 );
        if( queue )
        {
            //draw the shadowed inner text
            //NOTE we can't set an arbituary font size or family, these settings are already optional
            //and user defaults should also take presidence if no playlist font has been selected
            //const QFont smallFont( "Arial", (playNext > 9) ? 9 : 12 );
            //p->setFont( smallFont );
            //TODO the shadow is hard to do well when using a dark font color
            //TODO it also looks cluttered for small font sizes
            //p->setPen( cg.highlightedText().dark() );
            //p->drawText( width - w + 2, 3, w, h-1, Qt::AlignCenter, str );

            if( !multi )
                tmp = -(qh / 2);
            y += tmp;
            p.setPen( cg.highlightedText() );
            p.drawText( x, y, -x + width, multi ? h/2 : qh, Qt::AlignCenter, str );
            y -= tmp;
            if( isCurrent )
                y -= height() / 2;
            else
                x -= m + w2;
        }
        if( repeat )
        {
            if( multi )
                tmp = (h/2 - rh)/2 + ( num == 2 && stop ? 0 : 1 );
            else
                tmp = -(rh / 2);
            y += tmp;
            p.drawPixmap( x, y, pixrepeat );
            y -= tmp;
            if( num == 3 )
            {
                x -= m + w2 + 2;
                y = height() / 2;
            }
            else
                y -= height() / 2;
        }
        if( stop )
        {
            if( multi && num != 3 )
                tmp = m + (h/2 - sh)/2;
            else
                tmp = -(sh / 2);
            y += tmp;
            p.drawPixmap( x, y, pixstop );
            y -= tmp;
        }
    }

    if( this != listView()->currentTrack() && !isSelected() )
    {
        p.setPen( QPen( cg.mid(), 0, Qt::SolidLine ) );
        p.drawLine( width - 1, 0, width - 1, height() - 1 );
    }

    p.end();
    painter->drawPixmap( 0, 0, buf );
}

void PlaylistItem::drawRating( QPainter *p )
{
    int gray = 0;
    if( this == listView()->m_hoveredRating || ( isSelected() && listView()->m_selCount > 1 &&
        listView()->m_hoveredRating && listView()->m_hoveredRating->isSelected() ) )
    {
        const int pos = listView()->viewportToContents( listView()->viewport()->mapFromGlobal( QCursor::pos() ) ).x();
        gray = ratingAtPoint( pos );
    }

    drawRating( p, ( rating() + 1 ) / 2, gray / 2, rating() % 2 );
}

void PlaylistItem::drawRating( QPainter *p, int stars, int graystars, bool half )
{
    int i = 1, x = 1;
    const int y = height() / 2 - star()->height() / 2;
    if( half )
        i++;
    for(; i <= stars; ++i )
    {
        bitBlt( p->device(), x, y, star() );
        x += star()->width() + listView()->itemMargin();
    }
    if( half )
    {
        bitBlt( p->device(), x, y, smallStar() );
        x += star()->width() + listView()->itemMargin();
    }
    for(; i <= graystars; ++i )
    {
        bitBlt( p->device(), x, y, grayedStar() );
        x += star()->width() + listView()->itemMargin();
    }
}

void PlaylistItem::setup()
{
    KListViewItem::setup();

    // We make the current track item a bit taller than ordinary items
    if( this == listView()->currentTrack() )
        setHeight( int( float( listView()->fontMetrics().height() ) * 1.53 ) );
}


void PlaylistItem::paintFocus( QPainter* p, const QColorGroup& cg, const QRect& r )
{
    if( this != listView()->currentTrack() )
        KListViewItem::paintFocus( p, cg, r );
}

const QString &PlaylistItem::editingText()
{
    static const QString text = i18n( "Writing tag..." );
    return text;
}


/**
 * Changes the transparency (alpha component) of an image.
 * @param image Image to be manipulated. Must be true color (8 bit per channel).
 * @param factor > 1.0 == more transparency, < 1.0 == less transparency.
 */
void PlaylistItem::imageTransparency( QImage& image, float factor ) //static
{
    uint *data = reinterpret_cast<unsigned int *>( image.bits() );
    const int pixels = image.width() * image.height();
    uint table[256];
    register int c;

    // Precalculate lookup table
    for( int i = 0; i < 256; ++i ) {
        c = int( double( i ) * factor );
        if( c > 255 ) c = 255;
        table[i] = c;
    }

    // Process all pixels. Highly optimized.
    for( int i = 0; i < pixels; ++i ) {
        c = data[i]; // Memory access is slow, so do it only once
        data[i] = qRgba( qRed( c ), qGreen( c ), qBlue( c ), table[qAlpha( c )] );
    }
}

AtomicString PlaylistItem::artist_album() const
{
    static const AtomicString various_artist = QString( "Various Artists (INTERNAL) [ASDF!]" );
    if( compilation() == CompilationYes )
        return various_artist;
    else
        return artist();
}

void PlaylistItem::refAlbum()
{
    if( amaroK::entireAlbums() )
    {
        if( listView()->m_albums[artist_album()].find( album() ) == listView()->m_albums[artist_album()].end() )
            listView()->m_albums[artist_album()][album()] = new PlaylistAlbum;
        m_album = listView()->m_albums[artist_album()][album()];
        m_album->refcount++;
    }
}

void PlaylistItem::derefAlbum()
{
    if( amaroK::entireAlbums() )
    {
        m_album->refcount--;
        if( !m_album->refcount )
        {
            listView()->m_prevAlbums.removeRef( m_album );
            listView()->m_albums[artist_album()].remove( album() );
            if( listView()->m_albums[artist_album()].isEmpty() )
                listView()->m_albums.remove( artist_album() );
            delete m_album;
        }
    }
}

void PlaylistItem::incrementTotals()
{
    if( amaroK::entireAlbums() )
    {
        const uint prevCount = m_album->tracks.count();
        if( !track() || !m_album->tracks.count() || ( m_album->tracks.getLast()->track() && m_album->tracks.getLast()->track() < track() ) )
            m_album->tracks.append( this );
        else
            for( int i = 0, n = m_album->tracks.count(); i < n; ++i )
                if( m_album->tracks.at(i)->track() > track() || !m_album->tracks.at(i)->track() )
                {
                    m_album->tracks.insert( i, this );
                    break;
                }
        const Q_INT64 prevTotal = m_album->total;
        Q_INT64 total = m_album->total * prevCount;
        total += totalIncrementAmount();
        m_album->total = Q_INT64( double( total + 0.5 ) / m_album->tracks.count() );
        if( listView()->m_prevAlbums.findRef( m_album ) == -1 )
            listView()->m_total = listView()->m_total - prevTotal + m_album->total;
    }
    else if( listView()->m_prevTracks.findRef( this ) == -1 )
        listView()->m_total += totalIncrementAmount();
}

void PlaylistItem::decrementTotals()
{
    if( amaroK::entireAlbums() )
    {
        const Q_INT64 prevTotal = m_album->total;
        Q_INT64 total = m_album->total * m_album->tracks.count();
        m_album->tracks.removeRef( this );
        total -= totalIncrementAmount();
        m_album->total = Q_INT64( double( total + 0.5 ) / m_album->tracks.count() );
        if( listView()->m_prevAlbums.findRef( m_album ) == -1 )
            listView()->m_total = listView()->m_total - prevTotal + m_album->total;
    }
    else if( listView()->m_prevTracks.findRef( this ) == -1 )
        listView()->m_total -= totalIncrementAmount();
}

int PlaylistItem::totalIncrementAmount() const
{
    switch( AmarokConfig::favorTracks() )
    {
        case AmarokConfig::EnumFavorTracks::Off: return 0;
        case AmarokConfig::EnumFavorTracks::HigherScores: return score() ? score() : 50;
        case AmarokConfig::EnumFavorTracks::HigherRatings: return rating() ? rating() : 5; // 2.5
        case AmarokConfig::EnumFavorTracks::LessRecentlyPlayed:
        {
            if( lastPlay() )
                return listView()->m_startupTime_t - lastPlay();
            else if( listView()->m_oldestTime_t )
                return ( listView()->m_startupTime_t - listView()->m_oldestTime_t ) * 2;
            else
                return listView()->m_startupTime_t - 1058652000; //july 20, 2003, when amaroK was first released.
        }
        default: return 0;
    }
}

QPixmap *PlaylistItem::s_star = 0;
QPixmap *PlaylistItem::s_grayedStar = 0;
QPixmap *PlaylistItem::s_smallStar = 0;
