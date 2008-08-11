#include "AlbumEntry.h"
#include "Debug.h"

#include <plasma/paintutils.h>
#include <plasma/animator.h>

#include <KColorScheme>
#include <KIcon>

#include <QAction>
#include <QPainter>

AlbumEntry::AlbumEntry( QGraphicsItem *parent )
    : QGraphicsItem( parent )
    , m_hovering( 0 )
    , m_size( 100, 50 )
    , m_coverWidth( 50 )
    , m_animHighlightFrame( 0 )
{
    setAcceptsHoverEvents( true );
    
    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-albums" );
    m_theme->setContainsMultipleImages( true );
    
    m_albumName = new QGraphicsSimpleTextItem( this );
    m_trackCountString = new QGraphicsSimpleTextItem( this );
    m_cover = new QGraphicsPixmapItem( this );

    const qreal margin = 14.0;

    const qreal labelX = m_coverWidth + margin + 14.0;
    const qreal labelWidth = 15;
    const qreal textX = labelX + labelWidth + margin;

    const qreal textHeight = 22;
    const qreal textWidth = size().toSize().width() - ( textX + margin );
    const qreal yPos = 14.0;

    m_cover->setPos( QPointF( margin + 2, yPos ) );
    m_albumName->setPos( QPointF( textX, yPos ) );
    m_trackCountString->setPos( QPointF( textX, yPos + textHeight ) );    

    m_addIcon = new ToolBoxIcon( this );
    
    QAction *addToPlaylist = new QAction( i18n( "Add to playlist" ), this );
    addToPlaylist->setIcon( KIcon( "list-add" ) );
    addToPlaylist->setVisible( true );
    addToPlaylist->setEnabled( true );
    connect( addToPlaylist, SIGNAL( triggered() ), this, SLOT( addToPlaylist() ) );
    
    m_addIcon->setAction( addToPlaylist );

    m_addIcon->setText( "" );
    m_addIcon->setToolTip( addToPlaylist->text() );
    m_addIcon->setDrawBackground( false );
    m_addIcon->setOrientation( Qt::Horizontal );
    QSizeF iconSize = m_addIcon->sizeFromIconSize( 32 );
    m_addIcon->setMinimumSize( iconSize );
    m_addIcon->setMaximumSize( iconSize );
    m_addIcon->resize( m_addIcon->size() );

    m_addIcon->hide();
    m_addIcon->setZValue( zValue() + 1 );
    
}

QPainterPath
AlbumEntry::shape() const
{
    int width = boundingRect().width() - 5;
    int height = boundingRect().height() - 2;
    return Plasma::PaintUtils::roundedRectangle( QRectF( QPointF( 2.0, 2.0 ),
                                                             QSize( width, height ) ).adjusted( 2, 2, -2, -2 ), 4.0 );
}

QRectF
AlbumEntry::boundingRect() const
{
    return QRectF( 0, 0, size().width(), size().height() );
}

void
AlbumEntry::paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    Q_UNUSED( widget )

    QColor color1 = KColorScheme(QPalette::Active, KColorScheme::Window,
                               Plasma::Theme::defaultTheme()->colorScheme()).background().color();
    color1.setAlpha( 30 );

    painter->save();
    
    painter->setOpacity( m_animHighlightFrame );
    
    painter->setBrush( color1 );
    painter->setPen( Qt::NoPen );
    QPainterPath p = shape();
    painter->drawPath( p );
    painter->restore();

    m_cover->setPixmap( coverImage() );
    
    painter->save();
    
    const qreal margin = 14.0;
    const qreal labelX = m_coverWidth + margin;
    const qreal textHeight = 22;

    const qreal iconX = labelX + margin;
    const qreal yPos = margin;
    
    m_theme->paint( painter, QRect( margin - 5, yPos - 1, m_coverWidth + 12, m_coverWidth + 2), "cd-box" );
    m_theme->paint( painter, QRectF( iconX, yPos, 16, 16 ), "album" );
    m_theme->paint( painter, QRectF( iconX, yPos + textHeight, 16, 16 ), "track" );

    painter->restore();
}

void
AlbumEntry::setAlbumName( QString albumName )
{
    m_albumName->setText( albumName );    
}

void
AlbumEntry::setCoverImage( const QPixmap &cover )
{
    m_coverImage = cover;
}

void
AlbumEntry::setTrackCount( const QString &trackCount )
{
    m_trackCountString->setText( trackCount );
}

QString
AlbumEntry::albumName() const
{
    return m_albumName->text();
}

const QPixmap &
AlbumEntry::coverImage() const
{
    return m_coverImage;
}

QString
AlbumEntry::trackCount() const
{
    return m_trackCountString->text();
}

void
AlbumEntry::resize( QSizeF newSize )
{
    if( m_size != newSize )
    {
        m_size = newSize;
        update();
    }
}

QSizeF
AlbumEntry::size() const
{
    return m_size;
}

void
AlbumEntry::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    DEBUG_BLOCK
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );

    m_hovering = true;
    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseInCurve,
                                                                   this, "animateHighlight" );
    m_addIcon->setPos( boundingRect().width() - 50, boundingRect().height()/2 - m_addIcon->size().height()/2 );
    m_addIcon->show();
    QGraphicsItem::hoverEnterEvent( event );

}

void
AlbumEntry::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    DEBUG_BLOCK
    if( m_animHighlightId )
        Plasma::Animator::self()->stopCustomAnimation( m_animHighlightId );
    m_hovering = false;

    m_animHighlightId = Plasma::Animator::self()->customAnimation( 10, 240, Plasma::Animator::EaseOutCurve,
                                                                   this, "animateHighlight" );
    m_addIcon->hide();                                                              
    QGraphicsItem::hoverLeaveEvent( event );
}

void
AlbumEntry::animateHighlight( qreal progress )
{
    DEBUG_BLOCK
    if( m_hovering )
        m_animHighlightFrame = progress;
    else
        m_animHighlightFrame = 1.0 - progress;

    if( progress >= 1.0 )
        m_animHighlightId = 0;
    update();
}

void
AlbumEntry::addToPlaylist()
{
    emit clicked( m_albumName->text() );
}
