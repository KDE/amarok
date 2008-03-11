/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/


#include "debug.h"
#include "meta/MetaUtility.h"
#include "AmarokMimeData.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistGraphicsView.h"
#include "PlaylistDropVis.h"
#include "PlaylistModel.h"
#include "PlaylistTextItem.h"
#include "SvgTinter.h"
#include "tagdialog.h"
#include "TheInstances.h"
#include "CoverManager.h"
#include "meta/SourceInfoCapability.h"

#include "KStandardDirs"

#include <QBrush>
#include <QDrag>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QPen>
#include <QPixmapCache>
#include <QRadialGradient>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

#include <KLocale>

struct Playlist::GraphicsItem::ActiveItems
{
    ActiveItems()
    : foreground( 0 )
    , bottomLeftText( 0 )
    , bottomRightText( 0 )
    , topLeftText( 0 )
    , topRightText( 0 )
    , lastWidth( -5 )
    , groupedTracks ( 0 )
    , collapsible( true )
     { }

    ~ActiveItems()
    {
        delete bottomLeftText;
        delete bottomRightText;
        delete foreground;
        delete topLeftText;
        delete topRightText;

    }

    QGraphicsPixmapItem* foreground;
    Playlist::TextItem* bottomLeftText;
    Playlist::TextItem* bottomRightText;
    Playlist::TextItem* topLeftText;
    Playlist::TextItem* topRightText;

    QColor overlayGradientStart;
    QColor overlayGradientEnd;

    int lastWidth;
    int groupedTracks;
    bool collapsible;

    QRectF preDragLocation;
    Meta::TrackPtr track;
};


const qreal Playlist::GraphicsItem::ALBUM_WIDTH = 50.0;
const qreal Playlist::GraphicsItem::MARGIN = 4.0;
QFontMetricsF* Playlist::GraphicsItem::s_fm = 0;
QSvgRenderer * Playlist::GraphicsItem::s_svgRenderer = 0;


Playlist::GraphicsItem::GraphicsItem()
    : QGraphicsItem()
    , m_items( 0 )
    , m_height( -1 )
    , m_groupMode( -1 )
    , m_groupModeChanged ( false )
    , m_collapsible ( true )
    , m_dataChanged( false )
{
    setZValue( 1.0 );
    if( !s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
        m_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 ) + 2 * MARGIN;
    }

    if ( !s_svgRenderer ) {

        QString file = KStandardDirs::locate( "data","amarok/images/playlist_items.svg" );
        QString svg_source =  The::svgTinter()->tint( file );
        
        s_svgRenderer = new QSvgRenderer( svg_source.toAscii() );
        if ( ! s_svgRenderer->isValid() )
            debug() << "svg is kaputski";
    }

    setFlag( QGraphicsItem::ItemIsSelectable );
    setFlag( QGraphicsItem::ItemIsMovable );
    setAcceptDrops( true );
   // setHandlesChildEvents( true ); // don't let drops etc hit the text items, doing stupid things
}

Playlist::GraphicsItem::~GraphicsItem()
{
    delete m_items;
}

void
Playlist::GraphicsItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
// ::paint RULES:
// 1) You do not talk about ::paint method
// 2) You DO NOT talk about ::paint method
// 3) Do not show or hide item that are already shown or hidden, respectively
// 4) Do not setBrush without making sure its hasn't already been set to that brush().
// 5) ::paint RULES apply to all methods called from ::paint as well ( duh! )
// 6) If this is your first night at ::paint method, you HAVE to paint.
    Q_UNUSED( painter ); Q_UNUSED( widget );

    debug() << "painting row: " << m_currentRow;
    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );

    if( m_dataChanged || !m_items || ( option->rect.width() != m_items->lastWidth ) || m_groupModeChanged )
    {

        debug() << "Data changed";

        if( !m_items )
        {
            Meta::TrackPtr track = index.data( ItemRole ).value< Playlist::Item* >()->track();
            m_items = new Playlist::GraphicsItem::ActiveItems();
            m_items->track = track;
            init( track );
        }
        m_groupModeChanged = false;
        m_dataChanged = false;
        resize( m_items->track, option->rect.width() );
    }

    bool isActiveTrack = index.data( ActiveTrackRole ).toBool();


    // call paint method based on type
    if ( m_groupMode == None ) {
        paintSingleTrack( painter, option, isActiveTrack );
    } else if ( m_groupMode == Head ) {
        paintHead( painter, option, isActiveTrack );
    } else if ( m_groupMode == Head_Collapsed ) {
        paintCollapsedHead( painter, option, isActiveTrack );
    } else if ( m_groupMode == Body ) {
        paintBody( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
    } else if ( m_groupMode == End ) {
        paintTail( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
    } else if ( m_groupMode == Collapsed ) {
        paintCollapsed( );
    }

    return;

}

void
Playlist::GraphicsItem::init( Meta::TrackPtr track )
{
    Q_UNUSED( track );
    QFont font;
    font.setPointSize( font.pointSize() - 1 );
    // Disabling the editable text for now as it is misleading.. (it doesn't do anything.)
    #define NewText( X ) \
        X = new Playlist::TextItem( this ); \
/*         X->setTextInteractionFlags( Qt::TextEditorInteraction );*/ \
        X->setFont( font );
    NewText( m_items->topLeftText )
    NewText( m_items->bottomLeftText )
    NewText( m_items->topRightText )
    NewText( m_items->bottomRightText )
    #undef NewText
}

void
Playlist::GraphicsItem::resize( Meta::TrackPtr track, int totalWidth )
{
    DEBUG_BLOCK



    //just for good meassure:

    QFont font = m_items->topRightText->font();
    font.setBold( false );
    m_items->topRightText->setFont( font );

    font = m_items->topLeftText->font();
    font.setBold( false );
    m_items->topLeftText->setFont( font );

    
    if( totalWidth == -1 /*|| totalWidth == m_items->lastWidth */) //no change needed
        return;
    if( m_items->lastWidth != -5 ) //this isn't the first "resize"
        prepareGeometryChange();
    m_items->lastWidth = totalWidth;

    QString prettyLength;

    if ( m_groupMode == Head_Collapsed )
    {
        uint seconds = 0;
        for( int i = m_currentRow; i < m_currentRow + m_items->groupedTracks; i++ )
            seconds += The::playlistModel()->itemList()[ i ]->track()->length();
        prettyLength = Meta::secToPrettyTime( seconds );
    }
    else
        if ( track->length() > 0 )
            prettyLength = Meta::secToPrettyTime( track->length() );
        else 
            prettyLength = QString();
        
    QString album;
    if( track->album() )
        album = track->album()->name();

    const qreal lineTwoY = m_height / 2 + MARGIN;
    const qreal textWidth = ( ( qreal( totalWidth ) - ALBUM_WIDTH ) / 2.0 );
    const qreal leftAlignX = ALBUM_WIDTH + MARGIN;
    qreal topRightAlignX;
    qreal bottomRightAlignX;

    {
        qreal middle = textWidth + ALBUM_WIDTH + ( MARGIN * 2.0 );
        qreal rightWidth = totalWidth - qMax( s_fm->width( album )
            , s_fm->width( prettyLength ) );
        topRightAlignX = qMax( middle, rightWidth );
    }

    //lets use all the horizontal space we can get for now..
    int lengthStringWidth = (int)(s_fm->width( prettyLength ));
    bottomRightAlignX = ( totalWidth - 4 * MARGIN ) - lengthStringWidth ;




    qreal spaceForTopLeft = totalWidth - ( totalWidth - topRightAlignX ) - leftAlignX;
    qreal spaceForBottomLeft = totalWidth - ( totalWidth - bottomRightAlignX ) - leftAlignX;

    if ( m_groupMode == Head_Collapsed ) {

        m_items->bottomLeftText->setEditableText( QString("%1 tracks").arg( QString::number( m_items->groupedTracks ) ) , spaceForBottomLeft );
        QFont f = m_items->bottomLeftText->font();
        f.setItalic( true );
        m_items->bottomLeftText->setFont( f );
        m_items->bottomRightText->setEditableText( prettyLength, totalWidth - bottomRightAlignX );

    } else {
        m_items->bottomLeftText->setFont( m_items->bottomRightText->font() );
        if ( track->trackNumber() > 0 )
            m_items->bottomLeftText->setEditableText( QString("%1 - %2").arg( QString::number( track->trackNumber() ), track->name() ) , spaceForBottomLeft );
        else
            m_items->bottomLeftText->setEditableText( track->name() , spaceForBottomLeft );
        
        m_items->bottomRightText->setEditableText( prettyLength, totalWidth - bottomRightAlignX );
    }
    if ( m_groupMode == None ) {

        m_items->topRightText->setPos( topRightAlignX, MARGIN );
        m_items->topRightText->setEditableText( album, totalWidth - ( topRightAlignX + 4 ) );

        {
            QString artist;
            if( track->artist() )
                artist = track->artist()->name();
            m_items->topLeftText->setEditableText( artist, spaceForTopLeft );
            m_items->topLeftText->setPos( leftAlignX, MARGIN );
        }

        m_items->bottomLeftText->setPos( leftAlignX, lineTwoY );
        m_items->bottomRightText->setPos( bottomRightAlignX, lineTwoY );
    } else if ( ( m_groupMode == Head ) || ( m_groupMode == Head_Collapsed ) ) {

        int headingCenter = (int)( MARGIN + ( ALBUM_WIDTH - s_fm->height()) / 2 );

        //make the artist and album lines two lines

        int firstLineYOffset = ( ( MARGIN + ALBUM_WIDTH ) -s_fm->height() * 2 ) / 2;
        int headTextWidth = qreal( totalWidth ) - ( ALBUM_WIDTH + MARGIN * 4 );

        font = m_items->topRightText->font();
        font.setBold( true );
        m_items->topRightText->setFont( font );

        album = s_fm->elidedText ( album, Qt::ElideRight, headTextWidth );

        int albumWidth = s_fm->width( album );
        
        int offsetX = MARGIN + ALBUM_WIDTH + ( ( headTextWidth - albumWidth ) / 2 );

        //album goes at the bottom
        m_items->topRightText->setPos( offsetX , firstLineYOffset + MARGIN + s_fm->height() );
        m_items->topRightText->setEditableText( album, albumWidth );

        {
            QString artist;
            //various artist handling:
            //if the album has no albumartist, use Various Artists, otherwise use the albumartist's name
            if( track->album()->albumArtist() )
                artist = track->album()->albumArtist()->name();
            else
            {
                artist = findArtistForCurrentAlbum();
                if( artist.isEmpty() )
                    artist = i18n( "Various Artists" );
            }

            font = m_items->topLeftText->font();
            font.setBold( true );
            m_items->topLeftText->setFont( font );

            
            artist = s_fm->elidedText ( artist, Qt::ElideRight, headTextWidth );
            int artistWidth = s_fm->width( artist );
            offsetX = MARGIN + ALBUM_WIDTH + ( ( headTextWidth - artistWidth ) / 2);
            
            m_items->topLeftText->setEditableText( artist, artistWidth );
            m_items->topLeftText->setPos( offsetX, firstLineYOffset );
        }

        int underImageY = (int)( MARGIN + ALBUM_WIDTH + 6 );
        
        m_items->bottomLeftText->setPos( MARGIN * 3, underImageY );
        m_items->bottomRightText->setPos( bottomRightAlignX, underImageY );

    } else {
        m_items->bottomLeftText->setPos( MARGIN * 3, 0 );
        m_items->bottomRightText->setPos( bottomRightAlignX, 0 );
    }

    //make sure the activ eitem overlay has the correct width



    if( m_items->foreground )
    {

        QRectF trackRect;
        if ( ( m_groupMode == Head ) || ( m_groupMode == Head_Collapsed ) ) {
            trackRect = QRectF( 0, ALBUM_WIDTH + 2 * MARGIN, totalWidth, s_fm->height() );
        } else {
            trackRect = QRectF( 0, 0, totalWidth, m_height );
            if ( ( m_groupMode != Body) && !( ( m_groupMode == Head ) ) )
                trackRect.setHeight( trackRect.height() - 2 ); // add a little space between items
        }

        debug() << "Resizing active track overlay";

        QString key = QString("active_overlay:%1x%2").arg(trackRect.width()).arg(trackRect.height());

        QPixmap background( (int)( trackRect.width() ), (int)( trackRect.height() ) );
        background.fill( Qt::transparent );


        if (!QPixmapCache::find(key, background)) {
            QPainter pt( &background );
            s_svgRenderer->render( &pt, "active_overlay",   QRectF( 0, 0, trackRect.width(), trackRect.height() ) );
            QPixmapCache::insert(key, background);
        }
        m_items->foreground->setPixmap( background );
        m_items->foreground->setZValue( 10.0 );

        debug() << "Done";

    }

    m_items->lastWidth = totalWidth;

    m_dataChanged = false;
}

QString
Playlist::GraphicsItem::findArtistForCurrentAlbum() const
{
    if( !( ( m_groupMode == Head ) || ( m_groupMode == Head_Collapsed ) ) )
        return QString();

    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );
    if( ! ( ( index.data( GroupRole ).toInt() == Head ) || ( index.data( GroupRole ).toInt() == Head_Collapsed ) ) ) 
    {
        return QString();
    }
    else
    {
        QString artist;
        Meta::TrackPtr currentTrack = index.data( TrackRole ).value< Meta::TrackPtr >();
        if( currentTrack->artist() )
            artist = currentTrack->artist()->name();
        else
            return QString();
        //it's an album group, and the current row is the head, so the next row is either Body or End
        //that means we have to execute the loop at least once
        QModelIndex idx;
        int row = m_currentRow + 1;
        do
        {
            idx = The::playlistModel()->index( row++, 0 );
            Meta::TrackPtr track = idx.data( TrackRole ).value< Meta::TrackPtr >();
            if( track->artist() )
            {
                if( artist != track->artist()->name() )
                    return QString();
            }
            else
            {
                return QString();
            }
        }
        while( idx.data( GroupRole ).toInt() == Body );

        return artist;
    }
}

QRectF
Playlist::GraphicsItem::boundingRect() const
{
    // the viewport()->size() takes scrollbars into account
    return QRectF( 0.0, 0.0, The::playlistView()->viewport()->size().width(), m_height );
}

void
Playlist::GraphicsItem::play()
{
    The::playlistModel()->play( m_currentRow );
}

void
Playlist::GraphicsItem::showImage() const
{
    ( new CoverViewDialog( m_items->track->album(), The::playlistView() ) )->show(); 
}

void
Playlist::GraphicsItem::fetchImage()
{
/*    CoverFetcher *fetcher = The::coverFetcher();
    fetcher->manualFetch( m_items->track->album() );*/
}

void
Playlist::GraphicsItem::unsetImage()
{
    m_items->track->album()->removeImage();
}

void
Playlist::GraphicsItem::dataChanged()
{
    m_dataChanged = true;
    refresh();
}

void
Playlist::GraphicsItem::editTrackInformation()
{
    TagDialog *dialog = new TagDialog( m_items->track, Playlist::GraphicsView::instance() );
    dialog->show();
}

void
Playlist::GraphicsItem::mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event )
{
    if( m_items )
    {
        event->accept();
        play();
        return;
    }
    QGraphicsItem::mouseDoubleClickEvent( event );
}

void
Playlist::GraphicsItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( event->buttons() & Qt::RightButton || !m_items )
    {
        event->ignore();
        return;
    }
    m_items->preDragLocation = mapToScene( boundingRect() ).boundingRect();

    //did we hit the collapse / expand button?
    if( m_collapsible )
    {
        QRectF rect( boundingRect().width() - ( 16 + MARGIN ), MARGIN, 16, 16 );
        if( rect.contains( event->pos() ) )
        {
            if ( m_groupMode == Head_Collapsed )
                The::playlistModel()->setCollapsed( m_currentRow, false );
            else if ( m_groupMode == Head )
                The::playlistModel()->setCollapsed( m_currentRow, true );
        }
    }

    QGraphicsItem::mousePressEvent( event );
}

// With help from QGraphicsView::mouseMoveEvent()
void
Playlist::GraphicsItem::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
{
    if( (event->buttons() & Qt::LeftButton) && ( flags() & QGraphicsItem::ItemIsMovable) && m_items )
    {
        QPointF scenePosition = event->scenePos();

        if( scenePosition.y() < 0 )
            return;

        bool dragOverOriginalPosition = m_items->preDragLocation.contains( scenePosition );

        //make sure item is drawn on top of other items
        setZValue( 2.0 );

        // Determine the list of selected items
        QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
        if( !isSelected() )
            selectedItems << this;
        // Move all selected items
        foreach( QGraphicsItem *item, selectedItems )
        {
            if( (item->flags() & QGraphicsItem::ItemIsMovable) && (!item->parentItem() || !item->parentItem()->isSelected()) )
            {
                Playlist::GraphicsItem *above = 0;
                QPointF diff;
                if( item == this && !dragOverOriginalPosition )
                {
                    diff = event->scenePos() - event->lastScenePos();
                    QList<QGraphicsItem*> collisions = scene()->items( event->scenePos() );
                    foreach( QGraphicsItem *i, collisions )
                    {
                        Playlist::GraphicsItem *c = dynamic_cast<Playlist::GraphicsItem *>( i );
                        if( c && c != this )
                        {
                            above = c;
                            break;
                        }
                    }
                }
                else
                {
                    diff = item->mapToParent( item->mapFromScene(event->scenePos()))
                                              - item->mapToParent(item->mapFromScene(event->lastScenePos()));
                }

                item->moveBy( 0, diff.y() );
                if( item->flags() & ItemIsSelectable )
                    item->setSelected( true );

                if( dragOverOriginalPosition )
                    Playlist::DropVis::instance()->show( m_items->preDragLocation.y() );
                else
                    Playlist::DropVis::instance()->show( above );
            }
        }
    }
    else
    {
        QGraphicsItem::mouseMoveEvent( event );
    }
}

void
Playlist::GraphicsItem::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    foreach( const QString &mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            event->accept();
            Playlist::DropVis::instance()->show( this );
            break;
        }
    }
}

void
Playlist::GraphicsItem::dropEvent( QGraphicsSceneDragDropEvent * event )
{
    event->accept();
    setZValue( 1.0 );
    The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, m_currentRow, 0, QModelIndex() );
    Playlist::DropVis::instance()->hide();
}

void
Playlist::GraphicsItem::refresh()
{

    if (m_items && m_items->track ) 
        resize( m_items->track,m_items->lastWidth );


}

void Playlist::GraphicsItem::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    bool dragOverOriginalPosition = m_items->preDragLocation.contains( event->scenePos() );
    if( dragOverOriginalPosition )
    {
        setPos( m_items->preDragLocation.topLeft() );
        Playlist::DropVis::instance()->hide();
        return;
    }

    Playlist::GraphicsItem *above = 0;
    QList<QGraphicsItem*> collisions = scene()->items( event->scenePos() );
    foreach( QGraphicsItem *i, collisions )
    {
        Playlist::GraphicsItem *c = dynamic_cast<Playlist::GraphicsItem *>( i );
        if( c && c != this )
        {
            above = c;
            break;
        }
    }
    // if we've dropped ourself ontop of another item, then we need to shuffle the tracks below down
    if( above )
    {
        setPos( above->pos() );
        The::playlistView()->moveItem( this, above );
    } else {
        //Don't just drop item into the void, make it the last item!

        The::playlistView()->moveItem( this, 0 );
        //setPos( above->pos() );
        //The::playlistView()->moveItem( this, above );

    }

    //make sure item resets its z value
    setZValue( 1.0 );
    Playlist::DropVis::instance()->hide();
}

void Playlist::GraphicsItem::setRow(int row)
{
    //DEBUG_BLOCK
    m_currentRow = row;

    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );

    //figure out our group state and set height accordingly
    int currentGroupState = index.data( GroupRole ).toInt();

    if ( currentGroupState != m_groupMode ) {

        debug() << "Group changed for row " << row;

        prepareGeometryChange();


        m_groupMode = currentGroupState;
        m_groupModeChanged = true;

        switch ( m_groupMode ) {

            case None:
                debug() << "None";
                m_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 ) + 2 * MARGIN + 2;
                break;
            case Head:
                debug() << "Head";
                m_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 ) + MARGIN + s_fm->height() + 6;
                break;
            case Head_Collapsed:
                debug() << "Collapsed head";
                m_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 ) + MARGIN * 2 + s_fm->height() + 4;
                if ( !m_items ) {
                    const Meta::TrackPtr track = index.data( ItemRole ).value< Playlist::Item* >()->track();
                    m_items = new Playlist::GraphicsItem::ActiveItems();
                    m_items->track = track;
                    init( track );
                }
                    m_items->groupedTracks = index.data( GroupedTracksRole ).toInt();
                break;
            case Body:
                debug() << "Body";
                m_height =  s_fm->height()/*+ 2 * MARGIN*/;
                break;
            case End:
                debug() << "End";
                m_height =  s_fm->height() + 6 /*+ 2 * MARGIN*/;
                break;
            case Collapsed:
                debug() << "Collapsed";
                m_height =  0;
                break;
            default:
                debug() << "ERROR!!??";
        }
    }
}

void Playlist::GraphicsItem::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event );
    //DEBUG_BLOCK

    /*if ( m_groupMode == Head_Collapsed )
       The::playlistModel()->setCollapsed( m_currentRow, false ); */
}

void Playlist::GraphicsItem::paintSingleTrack( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active )
{

    DEBUG_BLOCK
    QRectF trackRect = option->rect;
    trackRect.setHeight( trackRect.height() - 2 );
    painter->drawPixmap( 0, 0, getCachedSvg( "track", trackRect.width(), trackRect.height() ) );

    //paint cover
    QPixmap albumPixmap;
    if( m_items->track->album() )
    {
        if( !m_items->track->album()->hasImage( int( ALBUM_WIDTH ) ) )
        {
//             The::coverFetcher()->queueAlbum( m_items->track->album() );
        }
        albumPixmap =  m_items->track->album()->image( int( ALBUM_WIDTH ) );
    }
    
    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect( imageLocation().x() + offset.x(), imageLocation().y() + offset.y(), imageLocation().width() - offset.x() * 2, imageLocation().height() - offset.y() * 2 );
    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );


    //check if there is a emblem to display
    //does this track have the SourceInfoCapability?
    Meta::SourceInfoCapability *sic = m_items->track->as<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        debug() << "Got SourceInfoCapability, source: " << source;
        if ( !source.isEmpty() ) {
            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );
        }

        delete sic;

    }


    /*m_items->topRightText->setDefaultTextColor( Qt::black );
    m_items->topLeftText->setDefaultTextColor( Qt::black );
    */
        //and make sure the top text elements are shown
    if( !m_items->topRightText->isVisible() )
        m_items->topRightText->show();
    if( !m_items->topLeftText->isVisible() )
        m_items->topLeftText->show();



    //set overlay if item is active:
    //handleActiveOverlay( trackRect, active );

    if ( active ) {

        const qreal lineTwoY = m_height / 2 + MARGIN;
        
        painter->drawPixmap( ALBUM_WIDTH + MARGIN + 2, lineTwoY, getCachedSvg( "active_overlay", trackRect.width() - ( ALBUM_WIDTH + MARGIN + 4 ), s_fm->height() ) );
    }
    
    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        const qreal lineTwoY = m_height / 2 + MARGIN;
        painter->drawPixmap( ALBUM_WIDTH + MARGIN + 2, lineTwoY, getCachedSvg( "selection_left", s_fm->height() * 3, s_fm->height() ) );
        painter->drawPixmap( (int)trackRect.width() - ( s_fm->height() * 3 + 2 ), lineTwoY, getCachedSvg( "selection_right", s_fm->height() * 3, s_fm->height() ) );
    }

}

void Playlist::GraphicsItem::paintHead( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active )
{
    QRectF trackRect = QRectF( option->rect.x(), ALBUM_WIDTH + 2 * MARGIN+ 2, option->rect.width(), s_fm->height() /*+ MARGIN*/ );
    QRectF headRect = option->rect;

    painter->drawPixmap( 0, 0, getCachedSvg( "head", headRect.width(), headRect.height() ) );


    //paint collapse button
    QString collapseString;
    if ( m_groupMode == Head )
    {
        if ( m_collapsible )
            collapseString = "collapse_button";
        else
            collapseString = "collapse_button_grayed_out";
    }
    else
        collapseString = "expand_button";

    painter->drawPixmap( (int)( option->rect.width() - ( 16 + MARGIN ) ), (int)MARGIN, getCachedSvg( collapseString, 16, 16 ) );


    //paint cover
    QPixmap albumPixmap;
    if( m_items->track->album() )
    {
        if( !m_items->track->album()->hasImage( int( ALBUM_WIDTH ) ) )
        {
//             The::coverFetcher()->queueAlbum( m_items->track->album() );
        }
        albumPixmap =  m_items->track->album()->image( int( ALBUM_WIDTH ) );
    }

    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect( imageLocation().x() + offset.x(), imageLocation().y() + offset.y(), imageLocation().width() - offset.x() * 2, imageLocation().height() - offset.y() * 2 );
    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );

    //draw active track marker if needed
    if ( active )
        painter->drawPixmap( trackRect.x() + 5, trackRect.y(), getCachedSvg( "active_overlay", trackRect.width() - 10 , trackRect.height() ) );

    
    //and make sure the top text elements are shown
    if( !m_items->topRightText->isVisible() )
        m_items->topRightText->show();
    if( !m_items->topLeftText->isVisible() )
        m_items->topLeftText->show();


        //check if there is a emblem to display
    //does this track have the SourceInfoCapability?

    Meta::SourceInfoCapability *sic = m_items->track->as<Meta::SourceInfoCapability>();
    if( sic )
    {

        debug() << "Got SourceInfoCapability!!";
        //is the source defined
        QString source = sic->sourceName();
        if ( !source.isEmpty() ) {

            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );
        }

        delete sic;

    }


    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap( trackRect.x() + 2, trackRect.y() + 1, getCachedSvg( "selection_left", trackRect.height() * 3, trackRect.height() -1 ) );
        painter->drawPixmap( (int)trackRect.bottomRight().x() - (trackRect.height() * 3 + 2), (int)trackRect.top() + 1, getCachedSvg( "selection_right", trackRect.height() * 3, trackRect.height() - 1 ) );
    }



}

void Playlist::GraphicsItem::paintCollapsedHead( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active )
{
    QRectF trackRect = QRectF( option->rect.x(), ALBUM_WIDTH + 2 * MARGIN * 2, option->rect.width(), s_fm->height() /*+ MARGIN*/ );
    QRectF headRect = QRectF( option->rect.x(), option->rect.y(), option->rect.width(), option->rect.height() - 2 );

    //paint background
    painter->drawPixmap( 0, 0, getCachedSvg( "collapsed_head", headRect.width(), headRect.height() ) );

    //paint collapse button
    QString collapseString;
    if ( m_groupMode == Head )
    {
        if ( m_collapsible )
            collapseString = "collapse_button";
        else
            collapseString = "collapse_button_grayed_out";
    }
    else
        collapseString = "expand_button";

    painter->drawPixmap( (int)( option->rect.width() - ( 16 + MARGIN ) ), (int)MARGIN, getCachedSvg( collapseString, 16, 16 ) );

    //paint cover:
    QPixmap albumPixmap;
    if( m_items->track->album() )
    {
        if( !m_items->track->album()->hasImage( int( ALBUM_WIDTH ) ) )
        {
//             The::coverFetcher()->queueAlbum( m_items->track->album() );
        }
        albumPixmap =  m_items->track->album()->image( int( ALBUM_WIDTH ) );
    }
    
    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect( imageLocation().x() + offset.x(), imageLocation().y() + offset.y(), imageLocation().width() - offset.x() * 2, imageLocation().height() - offset.y() * 2 );
    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );


    /*m_items->topRightText->setDefaultTextColor( Qt::white );
    m_items->topLeftText->setDefaultTextColor( Qt::white );
    */
        //and make sure the top text elements are shown
    if( !m_items->topRightText->isVisible() )
        m_items->topRightText->show();
    if( !m_items->topLeftText->isVisible() )
        m_items->topLeftText->show();


    //check if there is a emblem to display
    //does this track have the SourceInfoCapability?
    Meta::SourceInfoCapability *sic = m_items->track->as<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        if ( !source.isEmpty() ) {

            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );
        }

        delete sic;
    }



}

void Playlist::GraphicsItem::paintBody( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool alternate  )
{
    QRectF trackRect = option->rect;

    painter->drawPixmap( 0, 0, getCachedSvg( "body", trackRect.width(), trackRect.height() ) );


    //draw alternate background if needed
    if ( alternate )
        painter->drawPixmap( 5, 0, getCachedSvg( "body_background", trackRect.width() - 10, trackRect.height() ) );

    //draw active track marker if needed
    if ( active )
        painter->drawPixmap( 5, 0, getCachedSvg( "active_overlay", trackRect.width() - 10 , trackRect.height() ) );


    /*m_items->topRightText->setDefaultTextColor( Qt::black );
    m_items->topLeftText->setDefaultTextColor( Qt::black );
    */
    //make sure that the top text items are not shown
    if( m_items->topRightText->isVisible() )
        m_items->topRightText->hide();
    if( m_items->topLeftText->isVisible() )
        m_items->topLeftText->hide();
    if( !m_items->bottomRightText->isVisible() )
        m_items->bottomRightText->show();
    if( !m_items->bottomLeftText->isVisible() )
        m_items->bottomLeftText->show();

    //set selection marker if needed
    /*if( option->state & QStyle::State_Selected )
    {

        painter->drawPixmap( 2, (int)trackRect.top(), getCachedSvg( "selection_left", 40, trackRect.height() ) );
        painter->drawPixmap( (int)trackRect.width() - 42, (int)trackRect.top(), getCachedSvg( "selection_right", 40, trackRect.height() ) );
    }*/

    /*if ( active ) {

        m_items->bottomRightText->setDefaultTextColor( Qt::white );
        m_items->bottomLeftText->setDefaultTextColor( Qt::white );

    } else {

        m_items->bottomRightText->setDefaultTextColor( Qt::black );
        m_items->bottomLeftText->setDefaultTextColor( Qt::black );
    }*/

    //set overlay if item is active
    //handleActiveOverlay( trackRect, active );

    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap( trackRect.x() + 2, trackRect.y(), getCachedSvg( "selection_left", trackRect.height() * 3, trackRect.height() ) );
        painter->drawPixmap( (int)trackRect.bottomRight().x() - (trackRect.height() * 3 + 2), (int)trackRect.top(), getCachedSvg( "selection_right", trackRect.height() * 3, trackRect.height() ) );
    }


}

void Playlist::GraphicsItem::paintTail( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool alternate  )
{
   QRectF trackRect = option->rect;
   trackRect.setHeight( trackRect.height() - 2 ); // add a little space between items

   painter->drawPixmap( 0, 0, getCachedSvg( "tail", trackRect.width(), trackRect.height() ) );

    if ( alternate )
    {

        QRectF tempRect = trackRect;
        tempRect.setWidth( tempRect.width() - 10 );
        tempRect.setHeight( tempRect.height() - 4 );
        painter->drawPixmap( 5, 0, getCachedSvg( "body_background", tempRect.width(), tempRect.height() ) );

    }

    //draw active track marker if needed
    if ( active )
        painter->drawPixmap( 5, 0, getCachedSvg( "active_overlay", trackRect.width() - 10 , trackRect.height() - 3 ) );

    //make sure that the top text items are not shown
    if( m_items->topRightText->isVisible() )
        m_items->topRightText->hide();
    if( m_items->topLeftText->isVisible() )
        m_items->topLeftText->hide();
    if( !m_items->bottomRightText->isVisible() )
        m_items->bottomRightText->show();
    if( !m_items->bottomLeftText->isVisible() )
        m_items->bottomLeftText->show();

    //set selection marker if needed
   /* if( option->state & QStyle::State_Selected )
    {

        painter->drawPixmap( 2, (int)trackRect.top(), getCachedSvg( "selection_left", 40, trackRect.height() ) );
        painter->drawPixmap( (int)trackRect.width() - 42, (int)trackRect.top(), getCachedSvg( "selection_right", 40, trackRect.height() ) );
    }*/

    //set overlay if item is active
    //handleActiveOverlay( trackRect, active );

    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap( trackRect.x() + 2, trackRect.y(), getCachedSvg( "selection_left", trackRect.height() * 3, trackRect.height() - 4 ) );
        painter->drawPixmap( (int)trackRect.bottomRight().x() - (trackRect.height() * 3 + 2), (int)trackRect.top(), getCachedSvg( "selection_right", trackRect.height() * 3, trackRect.height() - 4 ) );
    }

}

void Playlist::GraphicsItem::paintCollapsed()
{

    // just make sure text items are hidden and then get the heck out of here...
    if( m_items->topRightText->isVisible() )
        m_items->topRightText->hide();
    if( m_items->topLeftText->isVisible() )
        m_items->topLeftText->hide();
    if( m_items->bottomRightText->isVisible() )
        m_items->bottomRightText->hide();
    if( m_items->bottomLeftText->isVisible() )
        m_items->bottomLeftText->hide();
}

QPixmap Playlist::GraphicsItem::getCachedSvg( QString name, int width, int height )
{
    QString key = QString("%1:%2x%3").arg( name ).arg( width ).arg( height );
    QPixmap pixmap( width, height );
    pixmap.fill( Qt::transparent );

    if( !QPixmapCache::find(key, pixmap) )
    {
        QPainter pt2( &pixmap );
        s_svgRenderer->render( &pt2, name, QRectF( 0, 0, width, height ) );
        QPixmapCache::insert(key, pixmap);
    }

    return pixmap;
}

void Playlist::GraphicsItem::handleActiveOverlay( QRectF rect, bool active )
{
    DEBUG_BLOCK
    if( active )
    {
        if( !m_items->foreground )
        {

            debug() << "Creating active track overlay";
            m_items->foreground = new QGraphicsPixmapItem( this );
            m_items->foreground->setPos( 0.0, rect.top() );
            //m_items->foreground->setZValue( 10.0 );

            m_items->foreground->setPixmap( getCachedSvg( "active_overlay", rect.width(), rect.height() ) );
            m_items->foreground->show();
            debug() << "Done";

        }
        if( !m_items->foreground->isVisible() )
            m_items->foreground->show();
    }
    else if( m_items->foreground && m_items->foreground->isVisible() )
        m_items->foreground->hide();
}

QPointF Playlist::GraphicsItem::centerImage(QPixmap pixmap, QRectF rect)
{

    qreal pixmapRatio = (qreal) pixmap.width() / (qreal) pixmap.height();

    debug() << "ratio: " << pixmapRatio;

    qreal moveByX = 0.0;
    qreal moveByY = 0.0;

    if( pixmapRatio >= 1 )
        moveByY = ( rect.height() - ( rect.width() / pixmapRatio ) ) / 2.0;
    else
        moveByX = ( rect.width() - ( rect.height() * pixmapRatio ) ) / 2.0;

    return QPointF( moveByX, moveByY );

}

bool Playlist::GraphicsItem::isCurrentTrack()
{
    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );
    return index.data( ActiveTrackRole ).toBool();
}

Meta::TrackPtr Playlist::GraphicsItem::internalTrack()
{
    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );
    return index.data( ItemRole ).value< Playlist::Item* >()->track();
}



