/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2007-2008 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2007 Henry de Valence <hdevalence@gmail.com>                           *
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

#include "PodcastCategory.h"

#include "Amarok.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "Debug.h"
#include "MetaUtility.h"
#include "PodcastModel.h"
#include "PodcastMeta.h"
#include "PopupDropperFactory.h"
#include "PlaylistsByProviderProxy.h"
#include "PlaylistTreeItemDelegate.h"
#include "browsers/InfoProxy.h"
#include "SvgTinter.h"
#include "SvgHandler.h"

#include <QAction>
#include <QFontMetrics>
#include <QHeaderView>
#include <QIcon>
#include <QLinearGradient>
#include <QModelIndexList>
#include <QPainter>
#include <QRegExp>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebFrame>
#include <QTextDocument>
#include <qnamespace.h>

#include <KAction>
#include <KMenu>
#include <KIcon>
#include <KStandardDirs>
#include <KUrlRequesterDialog>
#include <KGlobal>
#include <KLocale>

#include <typeinfo>

namespace The
{
    PlaylistBrowserNS::PodcastCategory* podcastCategory()
    {
        return PlaylistBrowserNS::PodcastCategory::instance();
    }
}

using namespace PlaylistBrowserNS;

QString PodcastCategory::s_byProviderKey( "Group By Provider" );
QString PodcastCategory::s_configGroup( "Podcast View" );

PodcastCategory* PodcastCategory::s_instance = 0;

PodcastCategory*
PodcastCategory::instance()
{
    return s_instance ? s_instance : new PodcastCategory( The::podcastModel() );
}

void
PodcastCategory::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = 0;
    }
}

PodcastCategory::PodcastCategory( PodcastModel *podcastModel )
    : BrowserCategory( "podcasts", 0 )
    , m_podcastModel( podcastModel )
{
    setPrettyName( i18n( "Podcasts" ) );
    setShortDescription( i18n( "List of subscribed podcasts and episodes" ) );
    setIcon( KIcon( "podcast-amarok" ) );

    setLongDescription( i18n( "Manage your podcast subscriptions and browse individual episodes. Downloading episodes to the disk is also done here, or you can tell Amarok to do this automatically." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_podcasts.png" ) );

    resize(339, 574);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth( this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);

    setContentsMargins(0,0,0,0);

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

    //a QWidget with minimumExpanding makes the next button right aligned.
    QWidget *spacerWidget = new QWidget( this );
    spacerWidget->setSizePolicy( QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding );
    toolBar->addWidget( spacerWidget );

    m_byProviderProxy = new PlaylistsByProviderProxy( podcastModel );
    m_podcastTreeView = new PodcastView( podcastModel, this );

    m_podcastTreeView->setFrameShape( QFrame::NoFrame );
    m_podcastTreeView->setContentsMargins(0,0,0,0);

    m_byProviderDelegate = new PlaylistTreeItemDelegate( m_podcastTreeView );
    m_defaultItemView = m_podcastTreeView->itemDelegate();

    KAction *toggleAction = new KAction( KIcon( "view-list-tree" ),
                                         i18n( "Toggle unified view mode" ), toolBar );
    toggleAction->setCheckable( true );
    toolBar->addAction( toggleAction );
    connect( toggleAction, SIGNAL( triggered( bool ) ), SLOT( toggleView( bool ) ) );
    if( Amarok::config( s_configGroup ).readEntry( s_byProviderKey, true ) )
    {
        m_podcastTreeView->setModel( m_byProviderProxy );
        m_podcastTreeView->setItemDelegate( m_byProviderDelegate );
        toggleAction->setChecked( true );
        m_podcastTreeView->setRootIsDecorated( false );
    }
    else
    {
        m_podcastTreeView->setModel( podcastModel );
        toggleAction->setChecked( false );
    }

    m_podcastTreeView->header()->hide();
    m_podcastTreeView->setIconSize( QSize( 32, 32 ) );

    m_podcastTreeView->setAlternatingRowColors( true );
    m_podcastTreeView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_podcastTreeView->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_podcastTreeView->setDragEnabled( true );

    for( int column = 1; column < podcastModel->columnCount(); ++ column )
    {
        m_podcastTreeView->hideColumn( column );
    }

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

    m_viewKicker = new ViewKicker( m_podcastTreeView );

    connect( m_podcastTreeView, SIGNAL( clicked( const QModelIndex & ) ), this,
             SLOT( showInfo( const QModelIndex & ) ) );

    QAction *importOpmlAction = new QAction( KIcon("document-import")
                                             , i18n( "Import OPML File" )
                                             , toolBar
                                         );
    importOpmlAction->setToolTip( i18n( "Import OPML File" ) );
    toolBar->addAction( importOpmlAction );
    connect( importOpmlAction, SIGNAL( triggered() ), SLOT( slotImportOpml() ) );

}

PodcastCategory::~PodcastCategory()
{
    delete m_viewKicker;
}

void
PodcastCategory::showInfo( const QModelIndex & index )
{
    QVariantMap map;
    const int row = index.row();
    QString description;
    QString title( index.data( Qt::DisplayRole ).toString() );
    QString subtitle( index.sibling( row, SubtitleColumn ).data( Qt::DisplayRole ).toString() );
    KUrl imageUrl( qvariant_cast<KUrl>(
        index.sibling( row, ImageColumn ).data( Qt::DisplayRole )
    ) );
    QString author( index.sibling( row, AuthorColumn ).data( Qt::DisplayRole ).toString() );
    QStringList keywords( qvariant_cast<QStringList>(
        index.sibling( row, KeywordsColumn ).data( Qt::DisplayRole )
    ) );
    bool isEpisode = index.sibling( row, IsEpisodeColumn ).data( Qt::DisplayRole ).toBool();
    QString authorAndPubDate;
    
    if( !author.isEmpty() )
    {
        authorAndPubDate = QString( "<b>%1</b> %2 " )
            .arg( i18n( "By" ) )
            .arg( Qt::escape( author ) );
    }

    if( !subtitle.isEmpty() )
    {
        description += QString( "<h1 class=\"subtitle\">%1</h1>" )
            .arg( Qt::escape( subtitle ) );
    }

    if( !imageUrl.isEmpty() )
    {
        description += QString( "<p style=\"float:right;\"><img src=\"%1\" onclick=\""
            "if (this.style.width=='150px') {"
                "this.style.width='auto';"
                "this.style.marginLeft='0em';"
                "this.style.cursor='-webkit-zoom-in';"
                "this.parentNode.style.float='inherit';"
                "this.parentNode.style.textAlign='center';"
            "} else {"
                "this.style.width='150px';"
                "this.style.marginLeft='1em';"
                "this.style.cursor='-webkit-zoom-out';"
                "this.parentNode.style.float='right';"
                "this.parentNode.style.textAlign='inherit';"
            "}\""
            " style=\"width: 150px; margin-left: 1em;"
            " margin-right: 0em; cursor: -webkit-zoom-in;\""
            "/></p>" )
            .arg( Qt::escape( imageUrl.url() ) );
    }

    if( isEpisode )
    {
        QDateTime pubDate( index.sibling( row, DateColumn ).data( Qt::DisplayRole ).toDateTime() );
        
        if( pubDate.isValid() )
        {
            authorAndPubDate += QString( "<b>%1</b> %2" )
                .arg( i18nc( "Podcast published on date", "On" ) )
                .arg( KGlobal::locale()->formatDateTime( pubDate, KLocale::FancyShortDate ) );
        }
    }

    if( !authorAndPubDate.isEmpty() )
    {
        description += QString( "<p>%1</p>" )
            .arg( authorAndPubDate );
    }

    if( isEpisode )
    {
        int fileSize = index.sibling( row, FilesizeColumn ).data( Qt::DisplayRole ).toInt();

        if( fileSize != 0 )
        {
            description += QString( "<p><b>%1</b> %2</p>" )
                .arg( i18n( "File Size:" ) )
                .arg( Meta::prettyFilesize( fileSize ) );
        }

    }
    else
    {
        QDate subsDate( index.sibling( row, DateColumn ).data( Qt::DisplayRole ).toDate() );
        
        if( subsDate.isValid() )
        {
            description += QString( "<p><b>%1</b> %2</p>" )
                .arg( i18n( "Subscription Date:" ) )
                .arg( KGlobal::locale()->formatDate( subsDate, KLocale::FancyShortDate ) );
        }
    }

    if( !keywords.isEmpty() )
    {
        description += QString( "<p><b>%1</b> %2</p>" )
            .arg( i18n( "Keywords:" ) )
            .arg( Qt::escape( keywords.join( ", " ) ) );
    }

    description += index.data( ShortDescriptionRole ).toString();
    
    description = QString(
        "<html>"
        "    <head>"
        "        <title>%1</title>"
        "        <style type=\"text/css\">"
        "h1 {text-align:center; font-size: 1.2em;}"
        "h1.subtitle {text-align:center; font-size: 1em; font-weight: normal;}"
        "        </style>"
        "    </head>"
        "    <body>"
        "        <h1>%1</h1>"
        "        %2"
        "    </body>"
        "</html>")
        .arg( Qt::escape( title ) )
        .arg( description );
    
    map["service_name"] = title;
    map["main_info"] = description;
    The::infoProxy()->setInfo( map );
}

void
PodcastCategory::slotImportOpml()
{
    DEBUG_BLOCK
    KUrl url = KUrlRequesterDialog::getUrl( QString(), this
                                            , i18n( "Select OPML file to import" )
                                            );
    if( !url.isEmpty() )
    {
        // user entered something and pressed OK
        m_podcastModel->importOpml( url );
    }
    else
    {
        // user entered nothing or pressed Cancel
        debug() << "invalid input or cancel";
    }
}

void
PodcastCategory::toggleView( bool enabled ) //SLOT
{
    if( enabled )
    {
        m_podcastTreeView->setModel( m_byProviderProxy );
        m_podcastTreeView->setItemDelegate( m_byProviderDelegate );
        m_podcastTreeView->setRootIsDecorated( false );
    }
    else
    {
        m_podcastTreeView->setModel( m_podcastModel );
        m_podcastTreeView->setItemDelegate( m_defaultItemView );
        m_podcastTreeView->setRootIsDecorated( true );
    }

    Amarok::config( s_configGroup ).writeEntry( s_byProviderKey, enabled );
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
    , m_podcastModel( model )
    , m_pd( 0 )
    , m_ongoingDrag( false )
    , m_dragMutex()
    , m_justDoubleClicked( false )
{
    connect( &m_clickTimer, SIGNAL( timeout() ), this, SLOT( slotClickTimeout() ) );
}

PodcastView::~PodcastView()
{}

void PodcastView::mousePressEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( KGlobalSettings::singleClick() )
        setItemsExpandable( false );
    if( !index.parent().isValid() ) //not a root element, don't bother checking actions
    {
        Amarok::PrettyTreeView::mousePressEvent( event );
        return;
    }

    const int actionCount =
            index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionCountRole ).toInt();
    if( actionCount > 0 )
    {
        const QRect rect = PlaylistTreeItemDelegate::actionsRect( index );
        if( rect.contains( event->pos() ) )
            return;
    }

    Amarok::PrettyTreeView::mousePressEvent( event );
}

void
PodcastView::mouseReleaseEvent( QMouseEvent * event )
{
    const QModelIndex index = indexAt( event->pos() );
    if( !index.parent().isValid() ) // not a root element, don't bother checking actions
    {
        const int actionCount =
            index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionCountRole ).toInt();
        if( actionCount > 0 )
        {
            const QRect rect = PlaylistTreeItemDelegate::actionsRect( index );
            if( rect.contains( event->pos() ) )
            {
                QList<QAction*> actions =
                        index.data( PlaylistBrowserNS::MetaPlaylistModel::ActionRole )
                        .toList().first().value<QList<QAction*> >();
                foreach( QAction *action, actions )
                {
                    if( action )
                        action->trigger();
                }
                return;
            }
        }
    }

    if( m_pd )
    {
        connect( m_pd, SIGNAL( fadeHideFinished() ), m_pd, SLOT( deleteLater() ) );
        m_pd->hide();
    }
    m_pd = 0;

    setItemsExpandable( true );

    if( m_clickTimer.isActive() || m_justDoubleClicked )
    {
        //it's a double-click...so ignore it
        m_clickTimer.stop();
        m_justDoubleClicked = false;
        m_savedClickIndex = QModelIndex();
        event->accept();
        return;
    }

    m_savedClickIndex = indexAt( event->pos() );
    KConfigGroup cg( KGlobal::config(), "KDE" );
    m_clickTimer.start( cg.readEntry( "DoubleClickInterval", 400 ) );
    m_clickLocation = event->pos();
    Amarok::PrettyTreeView::mouseReleaseEvent( event );
}

void
PodcastView::mouseMoveEvent( QMouseEvent *event )
{
    if( event->buttons() || event->modifiers() )
    {
        Amarok::PrettyTreeView::mouseMoveEvent( event );
        update();
        return;
    }
    QPoint point = event->pos() - m_clickLocation;
    KConfigGroup cg( KGlobal::config(), "KDE" );
    if( point.manhattanLength() > cg.readEntry( "StartDragDistance", 4 ) )
    {
        m_clickTimer.stop();
        slotClickTimeout();
        event->accept();
    }
    else
        Amarok::PrettyTreeView::mouseMoveEvent( event );
}

void
PodcastView::mouseDoubleClickEvent( QMouseEvent * event )
{
    QModelIndex index = indexAt( event->pos() );

    if( index.isValid() )
    {
        QModelIndexList indices;
        indices << index;
        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>( model() );
        if( mpm )
            mpm->loadItems( indices, Playlist::AppendAndPlay );
        event->accept();
    }

    m_clickTimer.stop();
    //m_justDoubleClicked is necessary because the mouseReleaseEvent still
    //comes through, but after the mouseDoubleClickEvent, so we need to tell
    //mouseReleaseEvent to ignore that one event
    m_justDoubleClicked = true;
    setExpanded( index, !isExpanded( index ) );

    event->accept();
}

void
PodcastView::startDrag( Qt::DropActions supportedActions )
{
    DEBUG_BLOCK

    // When a parent item is dragged, startDrag() is called a bunch of times. Here we prevent that:
    m_dragMutex.lock();
    if( m_ongoingDrag )
    {
        m_dragMutex.unlock();
        return;
    }
    m_ongoingDrag = true;
    m_dragMutex.unlock();

    if( !m_pd )
        m_pd = The::popupDropperFactory()->createPopupDropper( Context::ContextView::self() );

    if( m_pd && m_pd->isHidden() )
    {
        QList<QAction*> actions;
        MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>( model() );
        if( mpm )
            actions = mpm->actionsFor( selectedIndexes() );

        foreach( QAction * action, actions )
        {
            m_pd->addItem( The::popupDropperFactory()->createItem( action ) );
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
    m_dragMutex.lock();
    m_ongoingDrag = false;
    m_dragMutex.unlock();
}

void
PodcastView::contextMenuEvent( QContextMenuEvent * event )
{
    DEBUG_BLOCK

    KMenu menu;
    QModelIndexList indices = selectedIndexes();
    QList<QAction *> actions;
    MetaPlaylistModel *mpm = dynamic_cast<MetaPlaylistModel *>( model() );
    if( mpm )
        actions = mpm->actionsFor( indices );

    if( actions.isEmpty() )
        return;

    foreach( QAction * action, actions )
    {
        if( action )
            menu.addAction( action );
    }

    KAction* result = dynamic_cast< KAction* >( menu.exec( mapToGlobal( event->pos() ) ) );
    Q_UNUSED( result )

   debug() << indices.count() << " selectedIndexes";
}

void
PodcastView::slotClickTimeout()
{
    m_clickTimer.stop();
    if( m_savedClickIndex.isValid() && KGlobalSettings::singleClick() )
    {
        setExpanded( m_savedClickIndex, !isExpanded( m_savedClickIndex ) );
    }
    m_savedClickIndex = QModelIndex();
}

#include "PodcastCategory.moc"

