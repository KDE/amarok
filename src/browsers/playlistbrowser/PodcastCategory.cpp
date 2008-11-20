/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@kde.org>
   Copyright (c) 2007-2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
   Copyright (c) 2007 Henry de Valence <hdevalence@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "PodcastCategory.h"

#include "Amarok.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "Debug.h"
#include "PodcastModel.h"
#include "PodcastMeta.h"
#include "PopupDropperFactory.h"
#include "services/ServiceInfoProxy.h"
#include "SvgTinter.h"
#include "SvgHandler.h"

#include <QAction>
#include <QFile>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLinearGradient>
#include <QModelIndexList>
#include <QPainter>
#include <QPixmapCache>
#include <QRegExp>
#include <QSvgRenderer>
#include <QToolBar>
#include <QVBoxLayout>
#include <qnamespace.h>
#include <QWebFrame>

#include <KAction>
#include <KMenu>
#include <KStandardDirs>
#include <KIcon>

#include <typeinfo>

using namespace PlaylistBrowserNS;

PodcastCategory::PodcastCategory( PodcastModel *podcastModel )
    : QWidget()
    , m_podcastModel( podcastModel )
{
    resize(339, 574);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth( this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);

    setContentsMargins(0,0,0,0);

    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setContentsMargins(0,0,0,0);

    QToolBar *toolBar = new QToolBar( this );
    toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    QAction* addPodcastAction = new QAction( KIcon( "list-add-amarok" ), i18n("&Add Podcast"), toolBar );
    toolBar->addAction( addPodcastAction );
    connect( addPodcastAction, SIGNAL(triggered( bool )), m_podcastModel, SLOT(addPodcast()) );

    QAction* updateAllAction = new QAction( KIcon("view-refresh-amarok"),
                                            i18n("&Update All"), toolBar );
    toolBar->addAction( updateAllAction );
    connect( updateAllAction, SIGNAL(triggered( bool )),
             m_podcastModel, SLOT(refreshPodcasts()) );

    vLayout->addWidget( toolBar );

    m_podcastTreeView = new PodcastView( podcastModel, this );
    m_podcastTreeView->setFrameShape( QFrame::NoFrame );
    m_podcastTreeView->setContentsMargins(0,0,0,0);
    m_podcastTreeView->setModel( podcastModel );
    m_podcastTreeView->header()->hide();

    m_podcastTreeView->setAlternatingRowColors( true );
    m_podcastTreeView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_podcastTreeView->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_podcastTreeView->setDragEnabled(true);
    m_podcastTreeView->setAcceptDrops(true);
    m_podcastTreeView->setDropIndicatorShown(true);

    //transparency
    QPalette p = m_podcastTreeView->palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );

    c = p.color( QPalette::AlternateBase );
    c.setAlpha( 77 );
    p.setColor( QPalette::AlternateBase, c );

    m_podcastTreeView->setPalette( p );

    QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_podcastTreeView->sizePolicy().hasHeightForWidth());
    m_podcastTreeView->setSizePolicy(sizePolicy1);

    vLayout->addWidget( m_podcastTreeView );

    m_viewKicker = new ViewKicker( m_podcastTreeView );

    connect( m_podcastTreeView, SIGNAL( clicked( const QModelIndex & ) ), this, SLOT( showInfo( const QModelIndex & ) ) );
}

PodcastCategory::~PodcastCategory()
{
}

void
PodcastCategory::showInfo( const QModelIndex & index )
{

    QString description = index.data( ShortDescriptionRole ).toString();
    description.replace( QRegExp("\n "), "\n" );
    description.replace( QRegExp("\n+"), "\n" );

    QVariantMap map;
    map["service_name"] = "Podcasts";
    map["main_info"] = description;
    The::serviceInfoProxy()->setInfo( map );
}

ViewKicker::ViewKicker( QTreeView * treeView )
{
    DEBUG_BLOCK
    m_treeView = treeView;
}

void
ViewKicker::kickView()
{
    DEBUG_BLOCK
    m_treeView->setRootIndex( QModelIndex() );
}

PodcastCategoryDelegate::PodcastCategoryDelegate( QTreeView * view )
    : QItemDelegate()
    , m_view( view )
{
    m_webPage = new QWebPage( view );
}

PodcastCategoryDelegate::~PodcastCategoryDelegate()
{
}

void
PodcastCategoryDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option,
                                const QModelIndex & index ) const
{
    DEBUG_BLOCK

    int width = m_view->viewport()->size().width() - 4;
    int iconWidth = 16;
    int iconHeight = 16;
    int iconPadX = 8;
    int iconPadY = 4;
    int height = option.rect.height();

    painter->save();
    painter->setRenderHint ( QPainter::Antialiasing );

    QPixmap background = The::svgHandler()->renderSvg( "service_list_item", width - 40, height - 4, "service_list_item" );
    painter->drawPixmap( option.rect.topLeft().x() + 2, option.rect.topLeft().y() + 2, background );

    painter->setPen(Qt::black);

    painter->setFont(QFont("Arial", 9));

    QIcon icon = index.data( Qt::DecorationRole ).value<QIcon>();
    QPixmap iconPixmap = icon.pixmap( iconWidth, iconHeight );
    painter->drawPixmap( option.rect.topLeft() + QPoint( iconPadX, iconPadY ), iconPixmap );


    QRectF titleRect;
    titleRect.setLeft( option.rect.topLeft().x() + iconWidth + iconPadX );
    titleRect.setTop( option.rect.top() );
    titleRect.setWidth( width - ( iconWidth  + iconPadX * 2 + m_view->indentation() ) );
    titleRect.setHeight( iconHeight + iconPadY );

    QString title = index.data( Qt::DisplayRole ).toString();


    //TODO: these metrics should be made static members so they are not created all the damn time!!
    QFontMetricsF tfm( painter->font() );

    title = tfm.elidedText ( title, Qt::ElideRight, titleRect.width() - 8, Qt::AlignHCenter );
    //TODO: has a weird overlap
    painter->drawText ( titleRect, Qt::AlignLeft | Qt::AlignBottom, title );

    painter->setFont(QFont("Arial", 8));

    QRect textRect;
    textRect.setLeft( option.rect.topLeft().x() + iconPadX );
    textRect.setTop( option.rect.top() + iconHeight + iconPadY );
    textRect.setWidth( width - ( iconPadX * 2 + m_view->indentation() + 16) );
    textRect.setHeight( height - ( iconHeight + iconPadY ) );

    QFontMetricsF fm( painter->font() );
    QRectF textBound;

    QString description = index.data( ShortDescriptionRole ).toString();
    description.replace( QRegExp("\n "), "\n" );
    description.replace( QRegExp("\n+"), "\n" );


    if (option.state & QStyle::State_Selected)
        textBound = fm.boundingRect( textRect, Qt::TextWordWrap | Qt::AlignHCenter, description );
    else
        textBound = fm.boundingRect( titleRect, Qt::TextWordWrap | Qt::AlignHCenter, title );

    bool toWide = textBound.width() > textRect.width();
    bool toHigh = textBound.height() > textRect.height();
    if ( toHigh || toWide )
    {
        QLinearGradient gradient;
        gradient.setStart( textRect.bottomLeft().x(), textRect.bottomLeft().y() - 16 );

        //if( toWide && toHigh ) gradient.setFinalStop( textRect.bottomRight() );
        //else if ( toWide ) gradient.setFinalStop( textRect.topRight() );
        gradient.setFinalStop( textRect.bottomLeft() );

        gradient.setColorAt(0, painter->pen().color());
        gradient.setColorAt(0.5, Qt::transparent);
        gradient.setColorAt(1, Qt::transparent);
        QPen pen;
        pen.setBrush(QBrush(gradient));
        painter->setPen(pen);
    }

    if (option.state & QStyle::State_Selected)
    {
        //painter->drawText( textRect, Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft, description );
        m_webPage->setViewportSize( QSize( textRect.width(), textRect.height() ) );
        m_webPage->mainFrame()->setHtml( description );
        m_webPage->mainFrame()->render ( painter, QRegion( textRect ) );
    }

    painter->restore();

}

QSize
PodcastCategoryDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED( option );

    int width = m_view->viewport()->size().width() - 4;

    //todo: the height should be defined the way it is in the delegate: iconpadY*2 + iconheight
    //Meta::PodcastMetaCommon* pmc = static_cast<Meta::PodcastMetaCommon *>( index.internalPointer() );
    int height = 24;

    if( /*option.state & QStyle::State_HasFocus*/ m_view->currentIndex() == index )
    {
        QString description = index.data( ShortDescriptionRole ).toString();

        /*QFontMetrics fm( QFont( "Arial", 8 ) );
        height = fm.boundingRect ( 0, 0, width - ( 32 + m_view->indentation() ), 1000,
                                   Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap ,
                                   description ).height() + 40;
	    debug() << "Option is selected, height = " << height;*/

    }

    return QSize ( width, height );
}

PodcastView::PodcastView( PodcastModel *model, QWidget * parent )
    : Amarok::PrettyTreeView( parent )
    , m_model( model )
    , m_pd( 0 )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
    , m_downloadAction( 0 )
    , m_deleteAction( 0 )
    , m_removeAction( 0 )
    , m_renameAction( 0 )
    , m_configureAction( 0 )
    , m_labelAction( 0 )
{
}

PodcastView::~PodcastView()
{
}

void
PodcastView::mousePressEvent( QMouseEvent * event )
{
    if( event->button() == Qt::LeftButton )
        m_dragStartPosition = event->pos();

    QTreeView::mousePressEvent( event );
}

void
PodcastView::mouseReleaseEvent( QMouseEvent * event )
{
    Q_UNUSED( event )

    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd->hide();
    }
    m_pd = 0;
}

void
PodcastView::mouseDoubleClickEvent( QMouseEvent * event )
{
    QModelIndex index = indexAt( event->pos() );

    if( index.isValid() && index.internalPointer()  /*&& index.parent().isValid()*/ )
    {
        Meta::PodcastMetaCommon *pmc = static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());

        if ( pmc->podcastType() ==  Meta::EpisodeType )
        {
            Meta::PodcastEpisode *episode = static_cast<Meta::PodcastEpisode *>( pmc );
            The::playlistController()->insertOptioned( Meta::TrackPtr( static_cast<Meta::Track *>(episode )), Playlist::Append );
        }
        else if ( pmc->podcastType() ==  Meta::ChannelType )
        {
            QModelIndexList list;
            list << index;
            m_model->refreshItems( list );
        }
    }
}

void
PodcastView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK

    //Waah? when a parent item is dragged, startDrag is called a bunch of times
    static bool ongoingDrags = false;
    if( ongoingDrags )
        return;
    ongoingDrags = true;

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {

        QModelIndexList indices = selectedIndexes();

        QList<PopupDropperAction*> actions = actionsForIndices( indices );

        foreach( PopupDropperAction * action, actions ) {
            m_pd->addItem( The::popupDropperFactory()->createItem( action ), false );
        }

        m_pd->show();
    }

    QTreeView::startDrag( supportedActions );
    debug() << "After the drag!";

    if( m_pd )
    {
        debug() << "clearing PUD";
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( clear() ) );
        m_pd->hide();
    }
    ongoingDrags = false;
}

void
PodcastView::contextMenuEvent( QContextMenuEvent * event )
{
    DEBUG_BLOCK

    KMenu menu;

    QList<PopupDropperAction *> actions =
            actionsForIndices( selectionModel()->selectedIndexes() );

    foreach( PopupDropperAction * action, actions )
    {
        if( action )
            menu.addAction( action );
    }

    KAction* result = dynamic_cast< KAction* >( menu.exec( mapToGlobal( event->pos() ) ) );
    Q_UNUSED( result )
}

QList< PopupDropperAction * >
PodcastView::actionsForIndices( QModelIndexList indices )
{
    bool episodeSelected = false;
    bool channelSelected = false;

    QList<PopupDropperAction *> actions = createCommonActions( indices );

    m_currentItems.clear();
    foreach( const QModelIndex &index, indices )
    {
        if( index.isValid() && index.internalPointer() )
        {
            Meta::PodcastMetaCommon *pmc =
                static_cast<Meta::PodcastMetaCommon *>(index.internalPointer());

            m_currentItems << pmc;
            switch( pmc->podcastType() )
            {
                case Meta::EpisodeType: episodeSelected = true; break;
                case Meta::ChannelType: channelSelected = true; break;
            }
        }
    }

    if( episodeSelected && !channelSelected )
        actions << createEpisodeActions( indices );
    else if( channelSelected && !episodeSelected )
        actions << createChannelActions( indices );

    return actions;
}

QList< PopupDropperAction * >
PodcastView::createCommonActions( QModelIndexList indices )
{
    Q_UNUSED( indices )
    QList< PopupDropperAction * > actions;

    if( m_appendAction == 0 )
    {
        m_appendAction = new PopupDropperAction(
            The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "append",
            KIcon( "media-track-add-amarok" ),
            i18n( "&Append to Playlist" ),
            this
        );
        connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppend() ) );
    }

    if( m_loadAction == 0 )
    {
        m_loadAction = new PopupDropperAction(
            The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "load",
            KIcon( "folder-open" ),
            i18nc( "Replace the currently loaded tracks with these",
            "&Load" ),
            this
        );
        connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotLoad() ) );
    }

    /*TODO: rename episodes
    if( m_renameAction == 0 )
    {
        m_renameAction =  new PopupDropperAction(
            The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "edit",
            KIcon( "media-track-edit-amarok" ),
            i18n( "&Rename" ),
            this
        );
        connect( m_renameAction, SIGNAL( triggered() ), this, SLOT( slotRename() ) );
    }
    */

    actions << m_appendAction;
    actions << m_loadAction;
//     actions << m_renameAction;

    return actions;
}

QList< PopupDropperAction * >
PodcastView::createChannelActions( QModelIndexList indices )
{
    Q_UNUSED( indices )
    QList< PopupDropperAction * > actions;

    if( m_removeAction == 0 )
    {
        m_removeAction = new PopupDropperAction(
            The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "remove",
            KIcon( "news-unsubscribe" ),
            i18n( "&Remove Subscription" ),
            this
        );
        connect( m_removeAction, SIGNAL( triggered() ), this, SLOT( slotRemove() ) );
    }

    /* TODO: label channels
    if( m_labelAction == 0 )
    {
        m_labelAction = new PopupDropperAction(
            The::svgHandler()->getRenderer("amarok/images/pud_items.svg"),
            "label",
            KIcon( "flag-black" ),
            i18nc( "Label (or tag) the currently selected Channel(s)", "&Label" ),
            this
        );
        connect( m_labelAction, SIGNAL( triggered() ), this, SLOT( slotLabel() ) );
    }
    */

    if( m_configureAction == 0 )
    {
        m_configureAction = new PopupDropperAction(
            The::svgHandler()->getRenderer("amarok/images/pud_items.svg"),
            "configure",
            KIcon( "configure" ),
            i18n( "&Configure" ),
            this
        );
        connect( m_configureAction, SIGNAL( triggered() ), this, SLOT( slotConfigure() ));
    }

    actions << m_removeAction;
//     actions << m_labelAction;
    actions << m_configureAction;

    return actions;
}

QList< PopupDropperAction * >
PodcastView::createEpisodeActions( QModelIndexList indices )
{
    Q_UNUSED( indices )
    QList< PopupDropperAction * > actions;

    if ( m_deleteAction == 0 )
    {
        m_deleteAction = new PopupDropperAction(
            The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "delete",
            KIcon( "edit-delete" ),
            i18n( "&Delete Downloaded Episode" ),
            this
        );
        connect( m_deleteAction, SIGNAL( triggered() ), this, SLOT( slotDelete() ) );
    }
    actions << m_deleteAction;
    if ( m_downloadAction == 0 )
    {
        m_downloadAction = new PopupDropperAction(
            The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ),
            "download",
            KIcon( "go-down" ),
            i18n( "&Download Episode" ),
            this
        );
        connect( m_downloadAction, SIGNAL( triggered() ), this, SLOT( slotDownload() ) );
    }
    actions << m_downloadAction;

    return actions;
}

Meta::PodcastChannelList
PodcastView::selectedChannels()
{
    Meta::PodcastChannelList channels;
    foreach( Meta::PodcastMetaCommon *pmc, m_currentItems )
    {
        switch( pmc->podcastType() )
        {
            case Meta::EpisodeType:
                break;
            case Meta::ChannelType:
                channels << Meta::PodcastChannelPtr( static_cast<Meta::PodcastChannel *>(pmc) );
                break;
        }
    }
    return channels;
}

Meta::PodcastEpisodeList
PodcastView::selectedEpisodes()
{
    Meta::PodcastEpisodeList episodes;
    foreach( Meta::PodcastMetaCommon *pmc, m_currentItems )
    {
        switch( pmc->podcastType() )
        {
            case Meta::EpisodeType:
                episodes << Meta::PodcastEpisodePtr( static_cast<Meta::PodcastEpisode *>(pmc) );
                break;
            case Meta::ChannelType:
                episodes << static_cast<Meta::PodcastChannel *>(pmc)->episodes();
                break;
        }
    }
    return episodes;
}

Meta::TrackList
PodcastView::podcastEpisodesToTracks( Meta::PodcastEpisodeList episodes )
{
    Meta::TrackList tracks;
    foreach( Meta::PodcastEpisodePtr episode, episodes )
        tracks << Meta::TrackPtr::staticCast( episode );
    return tracks;
}

void
PodcastView::slotAppend()
{
    DEBUG_BLOCK

    Meta::PodcastEpisodeList episodes = selectedEpisodes();
    if( !episodes.empty() )
    {
        The::playlistController()->insertOptioned( podcastEpisodesToTracks( episodes ), Playlist::Append );
    }
}

void
PodcastView::slotConfigure()
{
    m_model->configureChannels( selectedIndexes() );
}

void
PodcastView::slotDelete()
{
   DEBUG_BLOCK
}

void
PodcastView::slotDownload()
{
    DEBUG_BLOCK
    m_model->downloadItems( selectedIndexes() );
}

void
PodcastView::slotLabel()
{
    DEBUG_BLOCK
}

void
PodcastView::slotLoad()
{
    DEBUG_BLOCK

    Meta::PodcastEpisodeList episodes = selectedEpisodes();

    if( !episodes.empty() )
    {
        The::playlistController()->insertOptioned( podcastEpisodesToTracks( episodes ), Playlist::Replace );
    }
}

void
PodcastView::slotRename()
{
    DEBUG_BLOCK
}

void
PodcastView::slotRemove()
{
    DEBUG_BLOCK
    m_model->removeSubscription( selectedIndexes() );
}

#include "PodcastCategory.moc"
