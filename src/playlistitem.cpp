/***************************************************************************
                        playlistitem.cpp  -  description
                           -------------------
  begin                : Die Dez 3 2002
  copyright            : (C) 2002 by Mark Kretschmann
  email                : markey@web.de
  copyright            : (C) 2005 by Gav Wood
  email                : gav@kde.org
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
#include "metabundle.h"
#include "playlist.h"
#include "playlistitem.h"
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
#include <kiconloader.h>
#include <kiconeffect.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>

QColor  PlaylistItem::glowText = Qt::white;
QColor  PlaylistItem::glowBase = Qt::white;
bool    PlaylistItem::s_pixmapChanged = false;

/// These are untranslated and used for storing/retrieving XML playlist
const QString PlaylistItem::columnName( int c ) //static
{
    switch( c ) {
        case Filename:   return "Filename";
        case Title:      return "Title";
        case Artist:     return "Artist";
        case Album:      return "Album";
        case Year:       return "Year";
        case Comment:    return "Comment";
        case Genre:      return "Genre";
        case Track:      return "Track";
        case Directory:  return "Directory";
        case Length:     return "Length";
        case Bitrate:    return "Bitrate";
        case Score:      return "Score";
        case Rating:     return "Rating";
        case Type:       return "Type";
        case LastPlayed: return "LastPlayed";
        case Playcount:  return "Playcount";
        case Moodbar:    return "Moodbar";
    }
    return "<ERROR>";
}


//statics
QString PlaylistItem::stringStore[STRING_STORE_SIZE];


PlaylistItem::PlaylistItem( QListView *listview, QListViewItem *item )
        : KListViewItem( listview, item )
{
    KListViewItem::setVisible( false );
}

PlaylistItem::PlaylistItem( const MetaBundle &bundle, QListViewItem *lvi )
        : KListViewItem( lvi->listView(), lvi->itemAbove(), filename( bundle.url() ) )
        , m_url( bundle.url() )
        , m_year( 0 )
        , m_track( 0 )
        , m_length( 0 )
        , m_bitrate( 0 )
        , m_score( -2 )
        , m_rating( -2 )
        , m_playCount( -2 )
        , m_lastPlay( 2 )
        , m_missing( false )
        , m_enabled( true )
        , m_proxyForMoods( 0 )
{
    setDragEnabled( true );

    if( AmarokConfig::showMoodbar() )
        checkMood();

    setText( bundle );

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
    }

    listView()->setFilterForItem( listView()->m_filter, this );

    listView()->countChanged();
}

PlaylistItem::PlaylistItem( QDomNode node, QListViewItem *item )
        : KListViewItem( item->listView(), item->itemAbove() )
        , m_url( node.toElement().attribute( "url" ) )
        , m_year( 0 )
        , m_track( 0 )
        , m_length( 0 )
        , m_bitrate( 0 )
        , m_score( -2 )
        , m_rating( -2 )
        , m_playCount( -2 )
        , m_lastPlay( 2 )
        , m_missing( false )
        , m_enabled( true )
        , m_proxyForMoods( 0 )
{
    setDragEnabled( true );

    //NOTE we use base versions to speed this up (this function is called 100s of times during startup)
    for( uint x = 1, n = listView()->columns(); x < n; ++x ) {
        const QString text = node.namedItem( columnName( x ) ).toElement().text();

        switch( x ) {
        case Title:
        case Comment:
            KListViewItem::setText( x, text );
            continue;
        case Artist:
        case Album:
        case Genre:
            KListViewItem::setText( x, attemptStore( text ) );
            continue;
        case Year:
            m_year = text.toInt();
            continue;
        case Track:
            m_track = text.toInt();
            continue;
        case Length:
            m_length = text.toInt();
            continue;
        case Bitrate:
            m_bitrate = text.toInt();
            continue;
        case Score:
        case Rating:
        case Playcount:
        case LastPlayed:
            continue;
        }
    }

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
    }

    listView()->setFilterForItem( listView()->m_filter, this );

    listView()->countChanged();

    if( AmarokConfig::showMoodbar() )
        checkMood();
}

PlaylistItem::~PlaylistItem()
{
    if( m_url.isEmpty() ) //constructed with the generic constructor, for PlaylistLoader's marker item
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
    }

    listView()->countChanged();

    if( listView()->m_hoveredRating == this )
        listView()->m_hoveredRating = 0;

    delete m_proxyForMoods;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

int PlaylistItem::score() const
{
    if( m_score == -2 )
        //const_cast is ugly, but other option was mutable, and then we lose const correctness checking
        //everywhere else
        *const_cast<int*>(&m_score) = CollectionDB::instance()->getSongPercentage( m_url.path() );
    return m_score;
}

int PlaylistItem::rating() const
{
    if( m_rating == -2 )
        *const_cast<int*>(&m_rating) = CollectionDB::instance()->getSongRating( m_url.path() );
    return m_rating;
}

int PlaylistItem::playCount() const
{
    if( m_playCount == -2 )
        *const_cast<int*>(&m_playCount) = CollectionDB::instance()->getPlayCount( m_url.path() );
    return m_playCount;
}

uint PlaylistItem::lastPlay() const
{
    if( m_lastPlay == 2 )
        *const_cast<uint*>(&m_lastPlay) = CollectionDB::instance()->getLastPlay( m_url.path() ).toTime_t();
    return m_lastPlay;
}

void PlaylistItem::setText( const MetaBundle &bundle )
{
    setTitle( bundle.title() );
    setArtist( bundle.artist() );
    setAlbum( bundle.album() );
    setYear( bundle.year() );
    setComment( bundle.comment() );
    setGenre( bundle.genre() );
    setTrack( bundle.track() );
    setLength( bundle.length() );
    setBitrate( bundle.bitrate() );

    m_missing = !bundle.exists();
}


void PlaylistItem::setText( int column, const QString &newText )
{
    switch( column )
    {
        case Title:      setTitle(     newText );         break;
        case Artist:     setArtist(    newText );         break;
        case Album:      setAlbum(     newText );         break;
        case Year:       setYear(      newText.toInt() ); break;
        case Genre:      setGenre(     newText );         break;
        case Comment:    setComment(   newText );         break;
        case Track:      setTrack(     newText.toInt() ); break;
        case Length:     setLength(    newText.toInt() ); break;
        case Bitrate:    setBitrate(   newText.toInt() ); break;
        case Score:      setScore(     newText.toInt() ); break;
        case Rating:     setRating(    newText.toInt() ); break;
        case Playcount:  setPlaycount( newText.toInt() ); break;
        case LastPlayed: setLastPlay(  newText.toInt() ); break;
        default: warning() << "Tried to set the text of an immutable or nonexistent column! [" << column << endl;
   }
}

QString PlaylistItem::exactText( int column ) const
{
    switch( column )
    {
        case Filename:   return filename();
        case Title:      return title();
        case Artist:     return artist();
        case Album:      return album();
        case Year:       return QString::number( year() );
        case Comment:    return comment();
        case Genre:      return genre();
        case Track:      return QString::number( track() );
        case Directory:  return directory();
        case Length:     return QString::number( length() );
        case Bitrate:    return QString::number( bitrate() );
        case Score:      return QString::number( score() );
        case Rating:     return QString::number( rating() );
        case Type:       return type();
        case Playcount:  return QString::number( playCount() );
        case LastPlayed: return QString::number( lastPlay() );
        case Moodbar:    return QString::null;
        default: warning() << "Tried to get the text of a nonexistent column! [" << column << endl;
    }

    return KListViewItem::text( column ); //shouldn't happen
}

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
        else if(AmarokConfig::calculateMoodOnQueue())
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
    if( m_url.isLocalFile() )
    {
        m_proxyForMoods = new MoodProxyObject( this );
        ReadMood *c = new ReadMood( m_proxyForMoods );
        ThreadWeaver::instance()->queueJob( c );
    }
}

QString PlaylistItem::text( int column ) const
{
    //if there is no text set for the title, return a pretty version of the filename

    static const QString editing = i18n( "Writing tag..." );

    switch( column )
    {
        case Filename:   return filename( m_url );
        case Title:      return ( title().isEmpty() && listView()->columnWidth( Filename ) == 0 )
                                ? MetaBundle::prettyTitle( filename() )
                                : title();
        case Artist:     return artist();
        case Album:      return album();
        case Year:       return year() == -4623894 ? editing : year() ? QString::number( year() ) : QString::null;
        case Comment:    return comment();
        case Genre:      return genre();
        case Track:      return track() == -1 ? editing : track() ? QString::number( track() ) : QString::null;
        case Directory:  return m_url.isEmpty()     ? QString()
                              : m_url.isLocalFile() ? m_url.directory() : m_url.upURL().prettyURL();
        case Length:     return length() == -1 ? editing : MetaBundle::prettyLength( length(), true );
        case Bitrate:    return bitrate() == -1 ? editing : MetaBundle::prettyBitrate( bitrate() );
        case Score:      return score() == -1 ? editing : QString::number( score() );
        case Rating:     return rating() == -1 ? editing : QString::number( rating() );
        case Type:       return m_url.isEmpty() ? QString() : ( m_url.protocol() == "http" ) ? i18n( "Stream" )
                                : filename().mid( filename().findRev( '.' ) + 1 );
        case Playcount:  return playCount() == -1 ? editing : QString::number( playCount() );
        case LastPlayed: return lastPlay() == 1 ? editing : amaroK::verboseTimeSince( lastPlay() );
        case Moodbar:    return "";
        default: warning() << "Tried to get the text of a nonexistent column!" << endl;
    }

    return KListViewItem::text( column ); //shouldn't happen
}

void PlaylistItem::setEnabled( bool enabled )
{
    m_enabled = enabled;
    setDropEnabled( enabled ); // this forbids items to be dropped into a history queue.

    repaint();
}

void PlaylistItem::setSelected( bool selected )
{
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
    }
    else if( !prevVisible && isVisible() )
    {
        listView()->m_visCount++;
        listView()->m_visLength += length();
        listView()->countChanged();
    }
}

void PlaylistItem::setEditing( int column )
{
    const QString editing = i18n( "Writing tag..." );
    switch( column )
    {
        case Title:
        case Artist:
        case Album:
        case Genre:
        case Comment:
            KListViewItem::setText( column, editing );
            break;
        case Year:       m_year      = -4623894; break;
        case Track:      m_track     = -1; break;
        case Length:     m_length    = -1; break;
        case Bitrate:    m_bitrate   = -1; break;
        case Score:      m_score     = -1; break;
        case Rating:     m_rating    = -1; break;
        case Playcount:  m_playCount = -1; break;
        case LastPlayed: m_lastPlay  =  1; break;
        default: warning() << "Tried to set the text of an immutable or nonexistent column!" << endl;
    }

    update();
}

bool PlaylistItem::isEditing( int column ) const
{
    const QString editing = i18n( "Writing tag..." );
    switch( column )
    {
        case Title:
        case Artist:
        case Album:
        case Genre:
        case Comment: //FIXME fix this hack!
            return KListViewItem::text( column ) == editing;
        case Year:       return m_year == -4623894;
        case Track:      return m_track     == -1;
        case Length:     return m_length    == -1;
        case Bitrate:    return m_bitrate   == -1;
        case Score:      return m_score     == -1;
        case Rating:     return m_rating    == -1;
        case Playcount:  return m_playCount == -1;
        case LastPlayed: return m_lastPlay  ==  1;
        default: return false;
    }
}

QPixmap *PlaylistItem::star( int type ) const
{
    static const int h = listView()->fontMetrics().height() + listView()->itemMargin() * 2 - 4
                         + ( ( listView()->fontMetrics().height() % 2 ) ? 1 : 0 );
    static QImage img = QImage( locate( "data", "amarok/images/star.png" ) ).smoothScale( h, h, QImage::ScaleMin );
    static QPixmap normal( img );
    static QPixmap grayed;
    static bool asdf = true;
    if( asdf )
    {
        KIconEffect::toGray( img, 1.0 );
        grayed.convertFromImage( img );
        asdf = false;
    }

    if( type == DrawGrayed )
        return &grayed;
    return &normal;
}

int PlaylistItem::ratingAtPoint( int x ) const
{
    x -= listView()->header()->sectionPos( Rating );
    return kClamp( ( x - 1 ) / ( star()->width() + listView()->itemMargin() ) + 1, 1, 5 );
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
        case Playcount:  return cmp( playCount(), i->playCount() );
        case LastPlayed: return cmp( lastPlay(),  i->lastPlay() );
        case Bitrate:    return cmp( bitrate(),   i->bitrate() );
        case Moodbar:    return cmp( theHueOrder, i->theHueOrder );
        case Year:
            if( year() == i->year() )
                return compare( i, Artist, ascending );
            return cmp( year(), i->year() );
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
                return compare( i, Track, true ) * (ascending ? 1 : -1);
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
    if( column == Moodbar && theArray.size() )
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

        if( isCurrent && !isSelected() )
        {
            static paintCacheItem paintCache[NUM_COLUMNS];

            // Convert QColor to string for use as key in QMap
            const QString colorKey =
                QString::number( glowBase.red() ) +
                QString::number( glowBase.green() ) +
                QString::number( glowBase.blue() );

            const bool cacheValid =
                paintCache[column].width == width &&
                paintCache[column].height == height() &&
                paintCache[column].text == colText &&
                paintCache[column].font == painter->font() &&
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

                const QColor bg = isAlternate() ? listView()->alternateBackground() :
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

                const float colorize1 = 0.6;
                const float colorize2 = 0.44;

                // Left part
                if( column == listView()->m_firstColumn ) {
                    QImage tmpImage = currentTrackLeft.smoothScale( currentTrackLeft.width(), height() );
                    KIconEffect::colorize( tmpImage, cg.highlight(), colorize1 );
                    KIconEffect::colorize( tmpImage, glowBase, colorize2 );
                    p.drawImage( 0, 0, tmpImage );
                    leftOffset = currentTrackLeft.width();
                    margin += 6;
                }

                // Right part
                else
                if( column == Playlist::instance()->mapToLogicalColumn( Playlist::instance()->visibleColumns() - 1 ) )
                {
                    QImage tmpImage = currentTrackRight.smoothScale( currentTrackRight.width(), height() );
                    KIconEffect::colorize( tmpImage, cg.highlight(), colorize1 );
                    KIconEffect::colorize( tmpImage, glowBase, colorize2 );
                    p.drawImage( width - currentTrackRight.width(), 0, tmpImage );
                    rightOffset = currentTrackRight.width();
                    margin += 6;
                }

                // Middle part
                // Here we scale the one pixel wide middel image to stretch to the full column width.
                QImage tmpImage = currentTrackMid.copy();
                KIconEffect::colorize( tmpImage, cg.highlight(), colorize1 );
                KIconEffect::colorize( tmpImage, glowBase, colorize2 );
                tmpImage = tmpImage.smoothScale( width - leftOffset - rightOffset, height() );
                p.drawImage( leftOffset, 0, tmpImage );


                // Draw the pixmap, if present
                int leftMargin = margin;
                if ( pixmap( column ) ) {
                    p.drawPixmap( leftMargin, height() / 2 - pixmap( column )->height() / 2, *pixmap( column ) );
                    leftMargin += pixmap( column )->width();
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
                    font.setItalic( true );
                    p.setFont( font );
                    p.setPen( cg.highlightedText() );
//                  paint.setPen( glowText );
                    const int _width = width - leftMargin - margin + minbearing - 1; // -1 seems to be necessary
                    const QString _text = KStringHandler::rPixelSqueeze( colText, painter->fontMetrics(), _width );
                    p.drawText( leftMargin, 0, _width, height(), align, _text );
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
    const bool repeat      = AmarokConfig::repeatTrack() && isCurrent;

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
    QPixmap *pix = star();
    int i = 1, x = 1;
    for(; i <= rating(); ++i )
    {
        bitBlt( p->device(), x, ( this == listView()->m_currentTrack ) ? pix->height() / 2 : 2, pix );
        x += pix->width() + listView()->itemMargin();
    }
    if( this == listView()->m_hoveredRating || ( isSelected() && listView()->m_selCount > 1 &&
        listView()->m_hoveredRating && listView()->m_hoveredRating->isSelected() ) )
    {
        pix = star( DrawGrayed );
        const int pos = listView()->viewportToContents( listView()->viewport()->mapFromGlobal( QCursor::pos() ) ).x();
        for( int n = ratingAtPoint( pos ); i <= n; ++i )
        {
            bitBlt( p->device(), x, ( this == listView()->m_currentTrack ) ? pix->height() / 2 : 2, pix );
            x += pix->width() + listView()->itemMargin();
        }
    }
}


void PlaylistItem::setup()
{
    KListViewItem::setup();

    if( this == listView()->currentTrack() )
        setHeight( listView()->fontMetrics().height() * 2 );
}


void PlaylistItem::paintFocus( QPainter* p, const QColorGroup& cg, const QRect& r )
{
    if( this != listView()->currentTrack() )
        KListViewItem::paintFocus( p, cg, r );
}

//this works because QString is implicitly shared
const QString &PlaylistItem::attemptStore( const QString &candidate ) //static
{
    //principal is to cause collisions at reasonable rate to reduce memory
    //consumption while not using such a big store that it is mostly filled with empty QStrings
    //because collisions are so rare

    if( candidate.isEmpty() ) return candidate; //nothing to try to share

    const uchar hash = candidate[0].unicode() % STRING_STORE_SIZE;


    if( stringStore[hash] != candidate ) //then replace
    {
        stringStore[hash] = candidate;
    }

    return stringStore[hash];
}

