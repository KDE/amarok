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

#include "PlaylistGraphicsItem.h"

#include "App.h"
#include "Debug.h"
#include "PlaylistDropVis.h"
#include "PlaylistGraphicsView.h"
#include "PlaylistModel.h"
#include "PlaylistTextItem.h"
#include "SvgTinter.h"
#include "TagDialog.h"
#include "covermanager/CoverManager.h"
#include "meta/MetaUtility.h"
#include "meta/SourceInfoCapability.h"

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
    , collapsible( false )
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

    QList<QPointF> childPreDragPositions;
};


const qreal Playlist::GraphicsItem::ALBUM_WIDTH = 50.0;
const qreal Playlist::GraphicsItem::SINGLE_TRACK_ALBUM_WIDTH = 40.0;
const qreal Playlist::GraphicsItem::MARGIN = 3.0;
const qreal Playlist::GraphicsItem::MARGINH = 6;
QFontMetricsF* Playlist::GraphicsItem::s_nfm = 0;
QFontMetricsF* Playlist::GraphicsItem::s_bfm = 0;
QFontMetricsF* Playlist::GraphicsItem::s_ifm = 0;


Playlist::GraphicsItem::GraphicsItem()
    : QGraphicsItem()
    , m_items( 0 )
    , m_height( -1 )
    , m_groupMode( -1 )
    , m_groupModeChanged ( false )
    , m_collapsible ( false )
    , m_dataChanged( false )
{
    setZValue( 1.0 );
    QFont font;
    if( !s_nfm )
    {
        s_nfm = new QFontMetricsF( font );
        m_height =  qMax( ALBUM_WIDTH, s_nfm->height() * 2 ) + 2 * MARGIN;
    if( !s_bfm )
    {
        font.setBold( true );
        s_bfm = new QFontMetricsF( font );
        font.setBold( false );
    }
    if( !s_ifm )
    {
        font.setItalic( true );
        s_bfm = new QFontMetricsF( font );
        font.setItalic( false );
    }

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

qreal
Playlist::GraphicsItem::albumHeaderHeight() const
{
    return qMax( ALBUM_WIDTH, s_nfm->height() * 2 ) + MARGIN;
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

    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );

    if( m_dataChanged || !m_items || ( option->rect.width() != m_items->lastWidth ) || m_groupModeChanged )
    {
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
    if ( m_groupMode == None )
        paintSingleTrack( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
    else if ( m_groupMode == Head )
        paintHead( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
    /*else if ( m_groupMode == Head_Collapsed )
        paintCollapsedHead( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool()  );*/
    else if ( m_groupMode == Body )
        paintBody( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
    else if ( m_groupMode == End )
        paintBody( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
        //paintTail( painter, option, isActiveTrack, index.data( GroupedAlternateRole ).toBool() );
    else if ( m_groupMode == Collapsed )
        paintCollapsed( );

    return;
}

void
Playlist::GraphicsItem::init( Meta::TrackPtr track )
{
    Q_UNUSED( track );
    QFont font;
    //font.setPointSize( font.pointSize() - 1 );
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
    {
        if ( track->length() > 0 )
            prettyLength = Meta::secToPrettyTime( track->length() );
        else
            prettyLength = QString();
    }

    QString album;
    if( track->album() )
        album = track->album()->name();
//FIXME: Is this still needed?
//     const qreal lineTwoY = m_height / 2 + MARGIN;
    const qreal textWidth = ( ( qreal( totalWidth ) - ( ALBUM_WIDTH + 2 * MARGIN ) ) / 2.0 );
    const qreal leftAlignX = ALBUM_WIDTH + MARGIN;
    qreal topRightAlignX;
    qreal bottomRightAlignX;

    {
        const qreal middle = textWidth + ALBUM_WIDTH + ( MARGIN * 2.0 );
        topRightAlignX = qMax( middle, totalWidth - ( s_nfm->width( album ) + MARGIN * 2 ) );
    }

    //lets use all the horizontal space we can get for now..
    int lengthStringWidth = (int)(s_nfm->width( prettyLength ));
    bottomRightAlignX = ( totalWidth - 2 * MARGIN ) - lengthStringWidth ;


    qreal spaceForTopLeft = totalWidth - ( totalWidth - topRightAlignX ) - leftAlignX;
    qreal spaceForBottomLeft = totalWidth - ( totalWidth - bottomRightAlignX ) - leftAlignX;

    if ( m_groupMode == Head_Collapsed ) {

        m_items->bottomLeftText->setEditableText( QString("%1 tracks").arg( QString::number( m_items->groupedTracks ) ) , spaceForBottomLeft );
        QFont f = m_items->bottomLeftText->font();
        f.setItalic( true );
        m_items->bottomLeftText->setFont( f );
        m_items->bottomRightText->setEditableText( prettyLength, totalWidth - bottomRightAlignX );

    }
    else
    {
        m_items->bottomLeftText->setFont( m_items->bottomRightText->font() );

        if ( track->trackNumber() > 0 )
            m_items->bottomLeftText->setEditableText( QString("%1 - %2").arg(
                QString::number( track->trackNumber() ), track->prettyName() ) , spaceForBottomLeft );
        else
            m_items->bottomLeftText->setEditableText( track->prettyName() , spaceForBottomLeft );

        m_items->bottomRightText->setEditableText( prettyLength, totalWidth - bottomRightAlignX );
    } //ELSE HEAD_COLLAPSED

    if ( m_groupMode == None )
    {
        const qreal lineTwoYSingle = s_nfm->height() + MARGIN;

        m_items->topRightText->setPos( topRightAlignX, MARGIN );
        m_items->topRightText->setEditableText( album, totalWidth - topRightAlignX );

        {
            QString artist;
            if( track->artist() )
                artist = track->artist()->name();
            if( !artist.isEmpty() )
                artist += ':';
            m_items->topLeftText->setEditableText( artist, spaceForTopLeft );
            m_items->topLeftText->setPos( leftAlignX, MARGIN );
        }

        m_items->bottomLeftText->setPos( leftAlignX, lineTwoYSingle );
        m_items->bottomRightText->setPos( bottomRightAlignX, lineTwoYSingle );
    }
    else if ( ( m_groupMode == Head ) || ( m_groupMode == Head_Collapsed ) )
    {
//FIXME: Is this still needed?
//         int headingCenter = (int)( MARGIN + ( ALBUM_WIDTH - s_fm->height()) / 2 );

        //make the artist and album lines two lines

        int firstLineYOffset = (int)( ( MARGIN + ALBUM_WIDTH ) - s_nfm->height() * 2 ) / 2;
        int headTextWidth = static_cast<int>(qreal( totalWidth ) - ( ALBUM_WIDTH + MARGIN * 4 ));

        font = m_items->topRightText->font();
        font.setBold( true );
        m_items->topRightText->setFont( font );

        album = s_bfm->elidedText ( album, Qt::ElideRight, headTextWidth );

        int albumWidth = (int)s_bfm->width( album );

        int offsetX = static_cast<int>(MARGIN + ALBUM_WIDTH + ( ( headTextWidth - albumWidth ) / 2 ) );

        //album goes at the bottom
        m_items->topRightText->setPos( offsetX , firstLineYOffset + MARGIN + s_bfm->height() );
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
            if( !artist.isEmpty() )
                artist += ':';

            font = m_items->topLeftText->font();
            font.setBold( true );
            m_items->topLeftText->setFont( font );

            artist = s_bfm->elidedText ( artist, Qt::ElideRight, headTextWidth );
            int artistWidth = (int)s_bfm->width( artist );
            offsetX = static_cast<int>( MARGIN + ALBUM_WIDTH + ( ( headTextWidth - artistWidth ) / 2) );

            m_items->topLeftText->setEditableText( artist, artistWidth );
            m_items->topLeftText->setPos( offsetX, firstLineYOffset );
        }

        int underImageY = (int)( MARGIN + ALBUM_WIDTH + 4 );

        m_items->bottomLeftText->setPos( MARGIN, underImageY );
        m_items->bottomRightText->setPos( bottomRightAlignX, underImageY );

    }
    else
    {
        m_items->bottomLeftText->setPos( MARGIN, 0 );
        m_items->bottomRightText->setPos( bottomRightAlignX, 0 );
    }

    //make sure the active item overlay has the correct width

    if( m_items->foreground )
    {

        QRectF trackRect;
        if ( ( m_groupMode == Head ) || ( m_groupMode == Head_Collapsed ) )
        {
            trackRect = QRectF( 0, ALBUM_WIDTH + 2 * MARGIN, totalWidth, s_nfm->height() );
        }
        else
        {
            trackRect = QRectF( 0, 0, totalWidth, m_height );
            if ( ( m_groupMode != Body) && !( m_groupMode == Head ) )
                trackRect.setHeight( trackRect.height() - 2 ); // add a little space between items
        }

        //debug() << "Resizing active track overlay";

        QPixmap background = The::svgHandler()->renderSvg(
                                         
                                        "active_overlay",
                                        (int)trackRect.width(),
                                        (int)trackRect.height(),
                                        "active_overlay"
                                      );
        m_items->foreground->setPixmap( background );
        m_items->foreground->setZValue( 10.0 );

        //debug() << "Done";

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
            if( track && track->artist() )
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
    CoverFetcher *fetcher = The::coverFetcher();
    fetcher->manualFetch( m_items->track->album() );
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
    TagDialog *dialog = new TagDialog( m_items->track, The::playlistView() );
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

    if ( groupMode() != Playlist::Head )
        m_items->preDragLocation = mapToScene( boundingRect() ).boundingRect();
    else {

        if ( groupMode() == Playlist::Head ) {
            //update the org positions so we can determine if we are dragging anything on itself
            QPointF topLeft = boundingRect().topLeft();

            //get last element in this group:

            int lastInAlbum = The::playlistModel()->lastInGroup( m_currentRow );

            debug() << "last in current album: " << lastInAlbum;

            QRectF bottomRect = The::playlistView()->tracks()[lastInAlbum ]->boundingRect();
            QPointF bottomPos = The::playlistView()->tracks()[lastInAlbum ]->pos();
            
            QPointF bottomRight = QPoint( bottomPos.x() + bottomRect.width(), bottomPos.y() + bottomRect.height() );

            debug() << "Got points: " << topLeft << " and " << bottomRight;

            QRectF orgRect( topLeft.x(), topLeft.y(), bottomRight.x() - topLeft.x(), ( bottomRight.y() - topLeft.y() ) );

            debug() << "Album rect: " << orgRect;

            m_items->preDragLocation = mapToScene( orgRect ).boundingRect();

            for ( int i = m_currentRow; i <=lastInAlbum; i++ )
                m_items->childPreDragPositions << The::playlistView()->tracks()[i]->pos();
        }

        

    }

    //did we hit the collapse / expand button?
    /*if( m_collapsible )
    {
        QRectF rect( boundingRect().width() - ( 16 + MARGIN ), MARGIN, 16, 16 );
        if( rect.contains( event->pos() ) )
        {
            if ( m_groupMode == Head_Collapsed )
                The::playlistModel()->setCollapsed( m_currentRow, false );
            else if ( m_groupMode == Head )
                The::playlistModel()->setCollapsed( m_currentRow, true );
        }
    }*/

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

    if( m_items && m_items->track ) {
        debug() << "refreshing " << m_items->track->name();
        resize( m_items->track,m_items->lastWidth );
    }
}

void Playlist::GraphicsItem::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    DEBUG_BLOCK
    
    bool dragOverOriginalPosition = m_items->preDragLocation.contains( event->scenePos() );
    if( dragOverOriginalPosition )
    {

        if ( groupMode() == Playlist::Head ) {
           
            int lastInAlbum = The::playlistModel()->lastInGroup( m_currentRow );
            //restore original positions
            for ( int i = m_currentRow; i <=lastInAlbum; i++ )
                The::playlistView()->tracks()[i]->setPos( m_items->childPreDragPositions.takeFirst() );
        } else {
            setPos( m_items->preDragLocation.topLeft() );
            Playlist::DropVis::instance()->hide();
        }
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
    }
    else
    {
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

    if ( currentGroupState != m_groupMode )
    {
        //debug() << "Group changed for row " << row;

        prepareGeometryChange();

        m_groupMode = currentGroupState;
        m_groupModeChanged = true;

        switch ( m_groupMode )
        {
            case None:
                //debug() << "None";
                m_height =  qMax( SINGLE_TRACK_ALBUM_WIDTH, s_nfm->height() * 2 ) + 2 * MARGIN;
                //debug() << "Height for single track: " << m_height;
                break;
            case Head:
                //debug() << "Head";
                m_height =  qMax( ALBUM_WIDTH, s_nfm->height() * 2 ) + MARGIN + s_nfm->height() + 4;
                break;
            case Head_Collapsed:
                debug() << "Collapsed head";
                m_height =  qMax( ALBUM_WIDTH, s_nfm->height() * 2 ) + MARGIN * 2 + s_nfm->height() + 16;
                if ( !m_items )
                {
                    const Meta::TrackPtr track = index.data( ItemRole ).value< Playlist::Item* >()->track();
                    m_items = new Playlist::GraphicsItem::ActiveItems();
                    m_items->track = track;
                    init( track );
                }
                m_items->groupedTracks = index.data( GroupedTracksRole ).toInt();
                debug() << "I have this many hidden tracks: " << m_items->groupedTracks;
                break;
            case Body:
                //debug() << "Body";
                m_height =  s_nfm->height()/*+ 2 * MARGIN*/;
                break;
            case End:
                //debug() << "End";
                m_height =  s_nfm->height() /*+ 6*/ /*+ 2 * MARGIN*/;
                break;
            case Collapsed:
                //debug() << "Collapsed";
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

void Playlist::GraphicsItem::setTextColor( bool active )
{
    const QModelIndex index = The::playlistModel()->index( m_currentRow, 0 );
    int state = index.data( StateRole ).toInt();

    if( active )
    {
        m_items->bottomLeftText->setDefaultTextColor( App::instance()->palette().brightText().color() );
        m_items->bottomRightText->setDefaultTextColor( App::instance()->palette().brightText().color() );
    }
    else
    {
        QColor textColor;
        switch( state )
        {
            // TODO: what should these be really ?
            case Item::NewlyAdded:
                textColor = App::instance()->palette().link().color();
                m_items->bottomLeftText->setDefaultTextColor( textColor );
                m_items->bottomRightText->setDefaultTextColor( textColor );
                break;

            case Item::DynamicPlayed:
                textColor = App::instance()->palette().brush( 
                        QPalette::Disabled, QPalette::ButtonText ).color();
                m_items->bottomLeftText->setDefaultTextColor( textColor );
                m_items->bottomRightText->setDefaultTextColor( textColor );
                break;

            case Item::Normal:
            default:
                textColor = App::instance()->palette().text().color();
                m_items->bottomLeftText->setDefaultTextColor( textColor );
                m_items->bottomRightText->setDefaultTextColor( textColor );
        }
    }
}


void Playlist::GraphicsItem::paintSingleTrack( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool alternate  )
{
    QRectF trackRect = option->rect;
    trackRect.setHeight( trackRect.height() );

    if ( !alternate )
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "track", (int)trackRect.width(), (int)trackRect.height(), "track" ) );
    else
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "alt_track", (int)trackRect.width(), (int)trackRect.height(), "alt_track" ) );
    
    //paint cover
    QPixmap albumPixmap;
    if( m_items->track->album() )
        albumPixmap =  m_items->track->album()->image( int( SINGLE_TRACK_ALBUM_WIDTH ) );

    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect( imageLocationSingleTrack().x() + offset.x(),
                      imageLocationSingleTrack().y() + offset.y(), imageLocationSingleTrack().width() - offset.x() * 2,
                      imageLocationSingleTrack().height() - offset.y() * 2 );

    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );


    //check if there is a emblem to display
    //does this track have the SourceInfoCapability?
    Meta::SourceInfoCapability *sic = m_items->track->as<Meta::SourceInfoCapability>();
    if( sic )
    {
        //is the source defined
        QString source = sic->sourceName();
        //debug() << "Got SourceInfoCapability, source: " << source;
        if ( !source.isEmpty() ) {
            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );
        }

        delete sic;

    }


    //and make sure the top text elements are shown
    if( !m_items->topRightText->isVisible() )
        m_items->topRightText->show();
    if( !m_items->topLeftText->isVisible() )
        m_items->topLeftText->show();

    //set overlay if item is active:
    //handleActiveOverlay( trackRect, active );

    const qreal lineTwoY = s_nfm->height() + MARGIN;

    if ( active )
    {
        painter->drawPixmap(
                             static_cast<int>( SINGLE_TRACK_ALBUM_WIDTH + MARGIN + 6 ),
                             (int)lineTwoY,
                             The::svgHandler()->renderSvg(
                                        "active_overlay",
                                        static_cast<int>( trackRect.width() - ( SINGLE_TRACK_ALBUM_WIDTH + MARGIN + 8 ) ),
                                        (int)s_nfm->height(),
                                        "active_overlay"
                                      )
                           );
    }

    setTextColor( active );

    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap(
                             static_cast<int>( SINGLE_TRACK_ALBUM_WIDTH + MARGIN + 6 ),
                             (int)lineTwoY,
                             The::svgHandler()->renderSvg(
                                        "selection",
                                        (int)(trackRect.width() - ( SINGLE_TRACK_ALBUM_WIDTH + MARGIN + 8 )),
                                        (int)s_nfm->height(),
                                        "selection"
                                      )
                           );
    }

}

void Playlist::GraphicsItem::paintHead( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool alternate  )
{
    QRectF trackRect = QRectF( option->rect.x(), ALBUM_WIDTH + MARGIN + 4, option->rect.width(), s_nfm->height() /*+ MARGIN*/ );
    QRectF headRect = option->rect;

    if ( alternate )
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "alt_head", (int)headRect.width(), (int)headRect.height(), "alt_head" ) );
    else
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "head", (int)headRect.width(), (int)headRect.height(), "head" ) );

    //draw divider between head and 1st track
    int dividerOffset = (int)(headRect.width() / 20);
    
    painter->drawPixmap( dividerOffset, (int)trackRect.top() - 1, The::svgHandler()->renderSvg( "divider_bottom", (int)headRect.width() - 2 * dividerOffset, 1, "divider_bottom" ) );
    painter->drawPixmap( dividerOffset, (int)trackRect.top(), The::svgHandler()->renderSvg( "divider_top", (int)headRect.width() - 2 * dividerOffset, 1, "divider_top" ) );
    

    

    //paint collapse button
    /*QString collapseString;
    if ( m_groupMode == Head )
    {
        if ( m_collapsible )
            collapseString = "collapse_button";
        else
            collapseString = "collapse_button_grayed_out";
    }
    else
        collapseString = "expand_button";


        painter->drawPixmap(
                             (int)( option->rect.width() - ( 16 + MARGIN ) ),
                             (int)MARGIN,
                              The::svgHandler()->renderSvg( collapseString, 16, 16, collapseString )
                           );

    */

    //paint cover
    QPixmap albumPixmap;
    if( m_items->track->album() )
        albumPixmap =  m_items->track->album()->image( int( ALBUM_WIDTH ) );

    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect( imageLocation().x() + offset.x(),
                      imageLocation().y() + offset.y(),
                      imageLocation().width() - offset.x() * 2,
                      imageLocation().height() - offset.y() * 2
                    );
    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );

    //draw active track marker if needed
    if ( active ) {
        painter->drawPixmap(
                             (int)trackRect.x(),
                             (int)trackRect.y(),
                             The::svgHandler()->renderSvg(
                                        "active_overlay",
                                        (int)trackRect.width(),
                                        (int)trackRect.height() -1 ,
                                        "active_overlay"
                                      )
                           );
    }

    setTextColor( active );

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
        //debug() << "Got SourceInfoCapability!!";
        //is the source defined
        QString source = sic->sourceName();
        if ( !source.isEmpty() )
        {
            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );
        }

        delete sic;

    }


    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap(
                             static_cast<int>(trackRect.x()),
                             static_cast<int>(trackRect.top() ),
                             The::svgHandler()->renderSvg(
                                             
                                        "selection",
                                        static_cast<int>( trackRect.width()),
                                        static_cast<int>(trackRect.height() -1 ),
                                        "selection"
                                      )
                           );
    }
}
#if 0

void Playlist::GraphicsItem::paintCollapsedHead( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool /*alternate*/  )
{
    Q_UNUSED( active ); // Do we want to paint a selected collapsed head differently?
    QRectF trackRect = QRectF( option->rect.x(), ALBUM_WIDTH + 2 * MARGIN * 2, option->rect.width(), s_nfm->height() /*+ MARGIN*/ );
    QRectF headRect = QRectF( option->rect.x(), option->rect.y(), option->rect.width(), option->rect.height() - 2 );

    //paint background
    painter->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "collapsed_head", (int)headRect.width(), (int)headRect.height(), "collapsed_head" ) );

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

    painter->drawPixmap( (int)( option->rect.width() - ( 16 + MARGIN ) ), (int)MARGIN, The::svgHandler()->renderSvg( collapseString, 16, 16, collapseString ) );

    //paint cover:
    QPixmap albumPixmap;
    if( m_items->track->album() )
        albumPixmap =  m_items->track->album()->image( int( ALBUM_WIDTH ) );

    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect(
                      imageLocation().x() + offset.x(),
                      imageLocation().y() + offset.y(),
                      imageLocation().width() - offset.x() * 2,
                      imageLocation().height() - offset.y() * 2
                    );
    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );

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
        if ( !source.isEmpty() )
        {
            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );
        }
        delete sic;
    }



}

#endif

void Playlist::GraphicsItem::paintBody( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool alternate )
{
    QRectF trackRect = option->rect;

    if ( !alternate )
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "body", (int)trackRect.width(), (int)trackRect.height(), "body" ) );
    else
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "alt_body", (int)trackRect.width(), (int)trackRect.height(), "alt_body" ) );

    //draw alternate background if needed
    /*if ( alternate )
        painter->drawPixmap( 5, 0, The::svgHandler()->renderSvg( "body_background", (int)trackRect.width() - 10, (int)trackRect.height(), "body_background" ) );
*/
    //draw active track marker if needed
    if ( active ) {
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "active_overlay", (int)trackRect.width(), (int)trackRect.height() -1, "active_overlay" ) );
    }

    setTextColor( active );

    //make sure that the top text items are not shown
    m_items->topRightText->hide();
    m_items->topLeftText->hide();
    //And the bottom ones are!
    m_items->bottomRightText->show();
    m_items->bottomLeftText->show();

    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap(
                             static_cast<int>( trackRect.x() ),
                             (int)trackRect.top(),
                             The::svgHandler()->renderSvg(
                                        
                                        "selection",
                                        static_cast<int>( trackRect.width() ),
                                        (int)trackRect.height() - 1,
                                        "selection"
                                      )
                           );
    }
}

void Playlist::GraphicsItem::paintTail( QPainter * painter, const QStyleOptionGraphicsItem * option, bool active, bool alternate  )
{
   QRectF trackRect = option->rect;
   trackRect.setHeight( trackRect.height() - 2 ); // add a little space between items

   //painter->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "tail", (int)trackRect.width(), (int)trackRect.height(), "tail" ) );

   if ( alternate )
       painter->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "tail", (int)trackRect.width(), (int)trackRect.height(), "tail" ) );
   else
       painter->drawPixmap( 0, 0, The::svgHandler()->renderSvg( "alt_tail", (int)trackRect.width(), (int)trackRect.height(), "alt_tail" ) );

    //draw active track marker if needed
    if ( active ) {
        painter->drawPixmap(
                             0,
                             0,
                             The::svgHandler()->renderSvg(
                                             
                                        "active_overlay",
                                        static_cast<int>( trackRect.width() ),
                                        static_cast<int>( trackRect.height() - 3),
                                        "active_overlay"
                                      )
                           );
    }

    setTextColor( active );


    //make sure that the top text items are not shown
    m_items->topRightText->hide();
    m_items->topLeftText->hide();
    //And that the bottom ones are!
    m_items->bottomRightText->show();
    m_items->bottomLeftText->show();

    //set selection marker if needed
    if( option->state & QStyle::State_Selected )
    {
        painter->drawPixmap(
                             static_cast<int>( trackRect.x() ),
                            (int)trackRect.top(),
                            The::svgHandler()->renderSvg(
                                       
                                       "selection",
                                       static_cast<int>( trackRect.width() ),
                                       static_cast<int>( trackRect.height() - 4 ),
                                       "selection"
                                     )
                           );
    }
}

void Playlist::GraphicsItem::paintCollapsed()
{
    // just make sure text items are hidden and then get the heck out of here...
    m_items->topRightText->hide();
    m_items->topLeftText->hide();
    m_items->bottomRightText->hide();
    m_items->bottomLeftText->hide();
}

void Playlist::GraphicsItem::handleActiveOverlay( QRectF rect, bool active )
{
    DEBUG_BLOCK
    if( active )
    {
        if( !m_items->foreground )
        {
            //debug() << "Creating active track overlay";
            m_items->foreground = new QGraphicsPixmapItem( this );
            m_items->foreground->setPos( 0.0, rect.top() );
            //m_items->foreground->setZValue( 10.0 );

            m_items->foreground->setPixmap( The::svgHandler()->renderSvg( "active_overlay", (int)rect.width(), (int)rect.height(), "active_overlay" ) );
            m_items->foreground->show();
            //debug() << "Done";
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

void Playlist::GraphicsItem::paletteChange()
{
    The::svgHandler()->reTint( );

    //make sure this image is re rendered
    if ( m_items && m_items->foreground )
    {
        delete m_items->foreground;
        m_items->foreground = 0;
    }

    //ensure that the text items use a sane color
    if ( m_items && m_items->bottomLeftText )
    {
        m_items->bottomLeftText->setDefaultTextColor( App::instance()->palette().text().color() );
        m_items->bottomRightText->setDefaultTextColor( App::instance()->palette().text().color() );
        m_items->topLeftText->setDefaultTextColor( App::instance()->palette().text().color() );
        m_items->topRightText->setDefaultTextColor( App::instance()->palette().text().color() );
    }

    refresh();
}

int Playlist::GraphicsItem::row()
{
    return m_currentRow;
}

