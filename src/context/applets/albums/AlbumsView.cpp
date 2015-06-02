/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "AlbumsView"

#include "AlbumsView.h"

#include "AlbumItem.h"
#include "AlbumsDefs.h"
#include "SvgHandler.h"
#include "TrackItem.h"
#include "core/capabilities/ActionsCapability.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "dialogs/TagDialog.h"
#include "playlist/PlaylistController.h"
#include "widgets/PrettyTreeView.h"

#include <KAction>
#include <KGlobalSettings>
#include <QIcon>
#include <KMenu>
#include <Plasma/Svg>
#include <Plasma/SvgWidget>
#include <Plasma/ScrollBar>

#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneContextMenuEvent>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QTreeView>
#include <QWheelEvent>

// Subclassed to override the access level of some methods.
// The AlbumsTreeView and the AlbumsView are so highly coupled that this is acceptable, imo.
class AlbumsTreeView : public Amarok::PrettyTreeView
{
    public:
        AlbumsTreeView( QWidget *parent = 0 )
            : Amarok::PrettyTreeView( parent )
        {
            setAttribute( Qt::WA_NoSystemBackground );
            viewport()->setAutoFillBackground( false );

            setHeaderHidden( true );
            setIconSize( QSize(60,60) );
            setDragDropMode( QAbstractItemView::DragOnly );
            setSelectionMode( QAbstractItemView::ExtendedSelection );
            setSelectionBehavior( QAbstractItemView::SelectItems );
            if( KGlobalSettings::graphicEffectsLevel() != KGlobalSettings::NoEffects )
                setAnimated( true );
            setRootIsDecorated( false );
            setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // see wheelEvent()
            setItemDelegate( new AlbumsItemDelegate( this ) );
            setFrameStyle( QFrame::NoFrame );
        }

        // Override access level to make it public. Only visible to the AlbumsView.
        // Used for context menu methods.
        QModelIndexList selectedIndexes() const { return PrettyTreeView::selectedIndexes(); }

    protected:
        void wheelEvent( QWheelEvent *e )
        {
            // scroll per pixel doesn't work when using delegates with big height (QTBUG-7232).
            // This is a work around for scrolling with smaller steps.
            AlbumsProxyModel *proxyModel = static_cast<AlbumsProxyModel*>( model() );
            AlbumsModel *albumsModel = static_cast<AlbumsModel*>( proxyModel->sourceModel() );
            verticalScrollBar()->setSingleStep( albumsModel->rowHeight() );
            Amarok::PrettyTreeView::wheelEvent( e );
        }
};

AlbumsView::AlbumsView( QGraphicsWidget *parent )
    : QGraphicsWidget( parent )
{
    Plasma::Svg *borderSvg = new Plasma::Svg( this );
    borderSvg->setImagePath( "widgets/scrollwidget" );

    m_topBorder = new Plasma::SvgWidget( this );
    m_topBorder->setSvg( borderSvg );
    m_topBorder->setElementID( "border-top" );
    m_topBorder->setZValue( 900 );
    m_topBorder->resize( -1, 10.0 );
    m_topBorder->show();

    m_bottomBorder = new Plasma::SvgWidget( this );
    m_bottomBorder->setSvg( borderSvg );
    m_bottomBorder->setElementID( "border-bottom" );
    m_bottomBorder->setZValue( 900 );
    m_bottomBorder->resize( -1, 10.0 );
    m_bottomBorder->show();

    m_treeProxy = new QGraphicsProxyWidget( this );
    m_treeView = new AlbumsTreeView( 0 );
    connect( m_treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)) );
    connect( m_treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotDoubleClicked()) );
    m_treeProxy->setWidget( m_treeView );

    m_model = new AlbumsModel( this );
    m_model->setColumnCount( 1 );
    m_proxyModel = new AlbumsProxyModel( this );
    m_proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_proxyModel->setSortLocaleAware( true );
    m_proxyModel->setDynamicSortFilter( true );
    m_proxyModel->setSourceModel( m_model );
    m_proxyModel->setFilterRole( NameRole );
    m_treeView->setModel( m_proxyModel );

    QScrollBar *treeScrollBar = m_treeView->verticalScrollBar();
    m_scrollBar = new Plasma::ScrollBar( this );
    m_scrollBar->setFocusPolicy( Qt::NoFocus );

    // synchronize scrollbars
    connect( treeScrollBar, SIGNAL(rangeChanged(int,int)), SLOT(slotScrollBarRangeChanged(int,int)) );
    connect( treeScrollBar, SIGNAL(valueChanged(int)), m_scrollBar, SLOT(setValue(int)) );
    connect( m_scrollBar, SIGNAL(valueChanged(int)), treeScrollBar, SLOT(setValue(int)) );
    m_scrollBar->setRange( treeScrollBar->minimum(), treeScrollBar->maximum() );
    m_scrollBar->setPageStep( treeScrollBar->pageStep() );
    m_scrollBar->setSingleStep( treeScrollBar->singleStep() );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Horizontal );
    layout->addItem( m_treeProxy );
    layout->addItem( m_scrollBar );
    layout->setSpacing( 2 );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    updateScrollBarVisibility();
}

AlbumsView::~AlbumsView()
{
}

void
AlbumsView::appendAlbum( QStandardItem *album )
{
    m_model->appendRow( album );
}

void
AlbumsView::sort()
{
    m_proxyModel->sort( 0 );
}

void
AlbumsView::scrollTo( QStandardItem *album )
{
    const QModelIndex &proxyIndex = m_proxyModel->mapFromSource( album->index() );
    m_treeView->scrollTo( proxyIndex, QAbstractItemView::EnsureVisible );
}

QString
AlbumsView::filterPattern() const
{
    return m_proxyModel->filterRegExp().pattern();
}

void
AlbumsView::setFilterPattern( const QString &pattern )
{
    m_proxyModel->setFilterRegExp( QRegExp(pattern, Qt::CaseInsensitive) );
}

void
AlbumsView::clear()
{
    qDeleteAll( m_model->findItems( QLatin1String( "*" ), Qt::MatchWildcard ) );
    m_model->clear();
}

AlbumsProxyModel::Mode
AlbumsView::mode() const
{
    return m_proxyModel->mode();
}

void
AlbumsView::setMode( AlbumsProxyModel::Mode mode )
{
    m_proxyModel->setMode( mode );
}

Qt::Alignment
AlbumsView::lengthAlignment() const
{
    return static_cast<AlbumsItemDelegate*>( m_treeView->itemDelegate() )->lengthAlignment();
}

void
AlbumsView::setLengthAlignment( Qt::Alignment alignment )
{
    static_cast<AlbumsItemDelegate*>( m_treeView->itemDelegate() )->setLengthAlignment( alignment );
}

void
AlbumsView::itemClicked( const QModelIndex &index )
{
    if( !m_treeView->model()->hasChildren( index ) )
        return;

    bool expanded = m_treeView->isExpanded( index );
    if( expanded )
        m_treeView->setExpanded( index, !expanded );
    else
        setRecursiveExpanded( index, !expanded );
}

void
AlbumsView::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
    const QModelIndex index = m_treeView->indexAt( event->pos().toPoint() );
    if( !index.isValid() )
    {
        QGraphicsWidget::contextMenuEvent( event );
        return;
    }

    KMenu menu;
    KAction *appendAction = new KAction( QIcon::fromTheme( "media-track-add-amarok" ), i18n( "&Add to Playlist" ), &menu );
    KAction *loadAction   = new KAction( QIcon::fromTheme( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Replace Playlist" ), &menu );
    KAction *queueAction  = new KAction( QIcon::fromTheme( "media-track-queue-amarok" ), i18n( "&Queue" ), &menu );
    KAction *editAction   = new KAction( QIcon::fromTheme( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), &menu );

    menu.addAction( appendAction );
    menu.addAction( loadAction );
    menu.addAction( queueAction );
    menu.addAction( editAction );

    connect( appendAction, SIGNAL(triggered()), this, SLOT(slotAppendSelected()) );
    connect( loadAction  , SIGNAL(triggered()), this, SLOT(slotReplaceWithSelected()) );
    connect( queueAction , SIGNAL(triggered()), this, SLOT(slotQueueSelected()) );
    connect( editAction  , SIGNAL(triggered()), this, SLOT(slotEditSelected()) );

    KMenu menuCover( i18n( "Album" ), &menu );
    const QStandardItem *item = m_model->itemFromIndex( m_proxyModel->mapToSource(index) );
    if( item->type() == AlbumType )
    {
        Meta::AlbumPtr album = static_cast<const AlbumItem*>( item )->album();
        QScopedPointer<Capabilities::ActionsCapability> ac( album->create<Capabilities::ActionsCapability>() );
        if( ac )
        {
            QList<QAction *> actions = ac->actions();
            if( !actions.isEmpty() )
            {
                // ensure that the actions are cleaned up afterwards
                foreach( QAction *action, actions )
                {
                    if( !action->parent() )
                        action->setParent( &menuCover );
                }

                menuCover.addActions( actions );
                menuCover.setIcon( QIcon::fromTheme( "filename-album-amarok" ) );
                menu.addMenu( &menuCover );
            }
        }
    }
    menu.exec( event->screenPos() );
}

void
AlbumsView::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    QGraphicsWidget::resizeEvent( event );
    if( m_topBorder )
    {
        m_topBorder->resize( event->newSize().width(), m_topBorder->size().height() );
        m_bottomBorder->resize( event->newSize().width(), m_bottomBorder->size().height() );
        m_topBorder->setPos( m_treeProxy->pos() );
        QPointF bottomPoint = m_treeProxy->boundingRect().bottomLeft();
        bottomPoint.ry() -= m_bottomBorder->size().height();
        m_bottomBorder->setPos( bottomPoint );
    }
}

void AlbumsView::slotDoubleClicked()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::OnDoubleClickOnSelectedItems );
}

void
AlbumsView::slotAppendSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::OnAppendToPlaylistAction );
}

void
AlbumsView::slotReplaceWithSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::OnReplacePlaylistAction );
}

void
AlbumsView::slotQueueSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::OnQueueToPlaylistAction );
}

void
AlbumsView::slotEditSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    if( !selected.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( selected );
        dialog->show();
    }
}

void
AlbumsView::slotScrollBarRangeChanged( int min, int max )
{
    m_scrollBar->setRange( min, max );
    m_scrollBar->setPageStep( m_treeView->verticalScrollBar()->pageStep() );
    m_scrollBar->setSingleStep( m_treeView->verticalScrollBar()->singleStep() );
    updateScrollBarVisibility();
}

void
AlbumsView::updateScrollBarVisibility()
{
    QGraphicsLinearLayout *lo = static_cast<QGraphicsLinearLayout*>( layout() );
    if( m_scrollBar->maximum() == 0 )
    {
        if( lo->count() > 1 && lo->itemAt(1) == m_scrollBar )
        {
            lo->removeAt( 1 );
            m_scrollBar->hide();
        }
    }
    else if( lo->count() == 1 )
    {
        lo->addItem( m_scrollBar );
        m_scrollBar->show();
    }
}

Meta::TrackList
AlbumsView::getSelectedTracks() const
{
    Meta::TrackList selected;

    QModelIndexList indexes = static_cast<AlbumsTreeView*>( m_treeView )->selectedIndexes();
    foreach( const QModelIndex &index, indexes )
    {
        if( index.isValid() )
        {
            const QModelIndex &srcIndex = m_proxyModel->mapToSource( index );
            const QStandardItem *item = m_model->itemFromIndex( srcIndex );
            if( item->type() == AlbumType )
            {
                selected << static_cast<const AlbumItem*>( item )->album()->tracks();
            }
            else if( item->type() == TrackType )
            {
                selected << static_cast<const TrackItem*>( item )->track();
            }
            else if( m_model->hasChildren( srcIndex ) ) // disc type
            {
                for( int i = m_model->rowCount( srcIndex ) - 1; i >= 0; --i )
                {
                    const QStandardItem *trackItem = m_model->itemFromIndex( srcIndex.child(i, 0) );
                    selected << static_cast<const TrackItem*>( trackItem )->track();
                }
            }
        }
    }

    return selected;
}

void
AlbumsView::setRecursiveExpanded( QStandardItem *item, bool expanded )
{
    setRecursiveExpanded( m_proxyModel->mapFromSource( item->index() ), expanded );
}

void
AlbumsView::setRecursiveExpanded( const QModelIndex &index, bool expanded )
{
    if( m_proxyModel->hasChildren( index ) )
    {
        for( int i = 0, count = m_proxyModel->rowCount( index ); i < count; ++i )
            m_treeView->setExpanded( index.child( i, 0 ), expanded );
    }
    m_treeView->setExpanded( index, expanded );
}

AlbumsItemDelegate::AlbumsItemDelegate( QObject *parent )
    : QStyledItemDelegate( parent )
    , m_lengthAlignment( Qt::AlignLeft )
{}

/*
 * Customize the painting of items in the tree view.
 *
 * AlbumItems: The album items' displayed text has the format
 * "artist - album (year)\ntracks, time", i.e. the album info is shown in two
 * lines. When resizing the context view, the string is elided if the available
 * width is less than the text width. That ellipsis is done on the whole string
 * however, so the second line disappears. A solution is to do the eliding
 * ourselves.
 *
 * TrackItems: The track number and length gets special treatment. They are
 * painted at the beginning and end of the string, respectively. The track
 * numbers are right aligned and a suitable width is used to make the numbers
 * align for all the tracks in the album. The track name and artist (the latter
 * is included if the album is a compilation), are placed in between the track
 * number and length; it is elided if necessary. Thus the track number and
 * length are always shown, even during eliding.
 */
void
AlbumsItemDelegate::paint( QPainter *p,
                           const QStyleOptionViewItem &option,
                           const QModelIndex &index ) const
{
    QStyleOptionViewItem sepOption = option;
    QStyledItemDelegate::paint( p, sepOption, index );
    const QAbstractProxyModel *xyModel = qobject_cast<const QAbstractProxyModel *>( index.model() );
    const QStandardItemModel *stdModel = qobject_cast<const QStandardItemModel *>( xyModel->sourceModel() );
    const QStandardItem *item = stdModel->itemFromIndex( xyModel->mapToSource(index) );
    if( item->type() == AlbumType )
    {
        // draw the text ourselves. The superclass will skip painting the
        // text since the text in Qt::DisplayRole is not actually set.
        QStyleOptionViewItemV4 vopt( option );
        initStyleOption( &vopt, index );
        const AlbumItem *albumItem = static_cast<const AlbumItem *>( item );
        int iconSize = albumItem->iconSize();
        QSize coverSize = albumItem->album()->image( iconSize ).size();
        coverSize.rwidth() += 6; // take into account of svg borders
        coverSize.rheight() += 6;
        qreal aspectRatio = static_cast<qreal>( coverSize.width() ) / coverSize.height();
        const int margin = vopt.widget->style()->pixelMetric( QStyle::PM_FocusFrameHMargin ) + 1;
        const int offset = qMin( int(iconSize * aspectRatio), iconSize ) + margin;
        if( option.direction == Qt::RightToLeft )
            vopt.rect.adjust( 0, 0, -offset, 0 );
        else
            vopt.rect.adjust( offset, 0, 0, 0 );
        drawAlbumText( p, vopt );
    }
    else if( item->type() == TrackType )
    {
        QStyleOptionViewItemV4 vopt( option );
        initStyleOption( &vopt, index );
        if( option.direction == Qt::RightToLeft )
            vopt.rect.adjust( 2, 0, 0, 0 );
        else
            vopt.rect.adjust( 0, 0, -2, 0 );
        drawTrackText( p, vopt );
    }
}

void
AlbumsItemDelegate::drawAlbumText( QPainter *p, const QStyleOptionViewItemV4 &vopt ) const
{
    const QModelIndex &index = vopt.index;
    const QRect &textRect = vopt.rect.adjusted( 4, 0, -4, 0 );

    p->save();
    p->setClipRect( textRect );
    applyCommonStyle( p, vopt );

    QString name = index.data( NameRole ).toString();
    int year     = index.data( AlbumYearRole ).toInt();

    QStringList texts;
    texts << ((year > 0) ? QString( "%1 (%2)" ).arg( name, QString::number(year) ) : name);
    texts << index.data( AlbumLengthRole ).toString();

    // elide each line according to available width
    QFontMetrics fm = vopt.fontMetrics;
    QMutableStringListIterator it( texts );
    while( it.hasNext() )
    {
        const QString &text = it.next();
        if( fm.width( text ) > textRect.width() )
            it.setValue( fm.elidedText( text, Qt::ElideRight, textRect.width() ) );
    }

    p->drawText( textRect, Qt::AlignLeft | Qt::AlignVCenter, texts.join("\n") );
    p->restore();
}

void
AlbumsItemDelegate::drawTrackText( QPainter *p, const QStyleOptionViewItemV4 &vopt ) const
{
    const QModelIndex &index = vopt.index;

    int trackDigitCount = index.data( AlbumMaxTrackNumberRole ).toString().length();
    bool isCompilation = index.data( AlbumCompilationRole ).toBool();
    const QString &name = index.data( NameRole ).toString();
    const QString &artist = index.data( TrackArtistRole ).toString();
    QString length = " (" + Meta::msToPrettyTime( index.data( TrackLengthRole ).toInt() ) + ')';
    QString number = index.data( TrackNumberRole ).toString() + ". ";
    QString middle = isCompilation ? QString( "%1 - %2" ).arg( artist, name ) : name;

    // use boldface font metrics for measuring track numbers
    QFont boldFont = vopt.font;
    boldFont.setBold( true );
    QFontMetrics boldFm( boldFont, p->device() );
    QFontMetrics fm( vopt.fontMetrics );

    int numberFillWidth = boldFm.width( QChar('0') ) * ( trackDigitCount - number.length() + 2 );
    int numberRectWidth = 0;
    if( number != QLatin1String("0. ") )
        numberRectWidth = numberFillWidth + boldFm.width( number ) + 2;
    int lengthRectWidth = boldFm.width( length );
    int availableWidth = vopt.rect.width() - numberRectWidth - lengthRectWidth;
    if( availableWidth < fm.width( middle ) )
        middle = fm.elidedText( middle, Qt::ElideRight, availableWidth );

    p->save();
    p->setClipRect( vopt.rect );
    p->setBackground( vopt.backgroundBrush );
    p->setLayoutDirection( vopt.direction );
    p->setFont( vopt.font );
    applyCommonStyle( p, vopt );
    int textRectWidth = (m_lengthAlignment == Qt::AlignLeft) ? fm.width( middle ) : availableWidth;

    QRect numberRect;
    QRect textRect;
    QRect lengthRect;
    if( vopt.direction == Qt::RightToLeft )
    {
        QPoint corner = vopt.rect.topRight();
        corner.rx() -= numberRectWidth;
        numberRect = QRect( corner, QSize( numberRectWidth, vopt.rect.height() ) );

        corner = numberRect.topLeft();
        corner.rx() -= textRectWidth;
        textRect = QRect( corner, QSize( textRectWidth, vopt.rect.height() ) );

        corner = textRect.topLeft();
        corner.rx() -= lengthRectWidth;
        lengthRect = QRect( corner, QSize( lengthRectWidth, vopt.rect.height() ) );
    }
    else
    {
        numberRect = QRect( vopt.rect.topLeft(), QSize( numberRectWidth, vopt.rect.height() ) );
        textRect = QRect( numberRect.topRight(), QSize( textRectWidth, vopt.rect.height() ) );
        lengthRect = QRect( textRect.topRight(), QSize( lengthRectWidth, vopt.rect.height() ) );
    }

    if( number != QLatin1String("0. ") )
        p->drawText( numberRect, Qt::AlignRight | Qt::AlignVCenter, number );
    p->drawText( textRect, Qt::AlignVCenter, middle );
    p->drawText( lengthRect, m_lengthAlignment | Qt::AlignVCenter, length );
    p->restore();
}

void
AlbumsItemDelegate::applyCommonStyle( QPainter *p, const QStyleOptionViewItemV4 &vopt ) const
{
    // styling code from QCommonStyle. These aren't actually used right
    // now, but will be needed if something like inline tag editing is
    // implemented.
    QPalette::ColorGroup cg = vopt.state & QStyle::State_Enabled
        ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(vopt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

    if (vopt.state & QStyle::State_Selected) {
        p->setPen(vopt.palette.color(cg, QPalette::HighlightedText));
    } else {
        p->setPen(vopt.palette.color(cg, QPalette::Text));
    }
    if (vopt.state & QStyle::State_Editing) {
        p->setPen(vopt.palette.color(cg, QPalette::Text));
        p->drawRect(vopt.rect.adjusted(0, 0, -1, -1));
    }
}

Qt::Alignment
AlbumsItemDelegate::lengthAlignment() const
{
    return m_lengthAlignment;
}

void
AlbumsItemDelegate::setLengthAlignment( Qt::Alignment a )
{
    if( a != Qt::AlignLeft && a != Qt::AlignRight )
        a = Qt::AlignLeft;
    m_lengthAlignment = a;
}

#include <AlbumsView.moc>
