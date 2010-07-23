/****************************************************************************************
 * Copyright (c) 2007-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#include "core/support/Amarok.h"
#include "App.h"
#include "context/ContextView.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "context/popupdropper/libpud/PopupDropper.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaUtility.h"
#include "PodcastModel.h"
#include "core/podcasts/PodcastMeta.h"
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
#include <KToolBar>
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

QString PodcastCategory::s_configGroup( "Podcast View" );

PodcastCategory* PodcastCategory::s_instance = 0;

PodcastCategory*
PodcastCategory::instance()
{
    return s_instance ? s_instance : new PodcastCategory( 0 );
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

PodcastCategory::PodcastCategory( QWidget *parent )
    : PlaylistBrowserCategory( Playlists::PodcastChannelPlaylist,
                               "podcasts",
                               s_configGroup,
                               The::podcastModel(),
                               parent )
{
    setPrettyName( i18n( "Podcasts" ) );
    setShortDescription( i18n( "List of podcast subscriptions and episodes" ) );
    setIcon( KIcon( "podcast-amarok" ) );

    setLongDescription( i18n( "Manage your podcast subscriptions and browse individual episodes. "
                              "Downloading episodes to the disk is also done here, or you can tell "
                              "Amarok to do this automatically." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_podcasts.png" ) );

    QAction *addPodcastAction = new QAction( KIcon( "list-add-amarok" ), i18n("&Add Podcast"),
                                             m_toolBar );
    m_toolBar->addAction( addPodcastAction );
    connect( addPodcastAction, SIGNAL(triggered( bool )), The::podcastModel(), SLOT(addPodcast()) );

    QAction *updateAllAction = new QAction( KIcon("view-refresh-amarok"),
                                            i18n("&Update All"), m_toolBar );
    m_toolBar->addAction( updateAllAction );
    connect( updateAllAction, SIGNAL(triggered( bool )),
             The::podcastModel(), SLOT(refreshPodcasts()) );


    QAction *importOpmlAction = new QAction( KIcon("document-import")
                                             , i18n( "Import OPML File" )
                                             , m_toolBar
                                         );
    importOpmlAction->setToolTip( i18n( "Import OPML File" ) );
    m_toolBar->addAction( importOpmlAction );
    connect( importOpmlAction, SIGNAL( triggered() ), SLOT( slotImportOpml() ) );

    //transparency
//    QPalette p = m_podcastTreeView->palette();
//    QColor c = p.color( QPalette::Base );
//    c.setAlpha( 0 );
//    p.setColor( QPalette::Base, c );
//
//    c = p.color( QPalette::AlternateBase );
//    c.setAlpha( 77 );
//    p.setColor( QPalette::AlternateBase, c );
//
//    m_podcastTreeView->setPalette( p );
//
//    QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
//    sizePolicy1.setHorizontalStretch(0);
//    sizePolicy1.setVerticalStretch(0);
//    sizePolicy1.setHeightForWidth(m_podcastTreeView->sizePolicy().hasHeightForWidth());
//    m_podcastTreeView->setSizePolicy(sizePolicy1);
//

}

PodcastCategory::~PodcastCategory()
{
}

void
PodcastCategory::showInfo( const QModelIndex &index )
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
                "this.style.cursor='-webkit-zoom-out';"
                "this.parentNode.style.float='inherit';"
                "this.parentNode.style.textAlign='center';"
            "} else {"
                "this.style.width='150px';"
                "this.style.marginLeft='1em';"
                "this.style.cursor='-webkit-zoom-in';"
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
        "body {color: %3;}"
        "::selection {background-color: %4;}"
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
        .arg( description )
        .arg( App::instance()->palette().brush( QPalette::Text ).color().name() )
        .arg( PaletteHandler::highlightColor().name() );
    
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
        The::podcastModel()->importOpml( url );
    }
    else
    {
        // user entered nothing or pressed Cancel
        debug() << "invalid input or cancel";
    }
}

PodcastCategoryDelegate::PodcastCategoryDelegate( QTreeView *view )
    : QItemDelegate()
    , m_view( view )
{
    m_webPage = new QWebPage( view );
}

PodcastCategoryDelegate::~PodcastCategoryDelegate()
{
}

void
PodcastCategoryDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index ) const
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
    //Podcasts::PodcastMetaCommon* pmc = static_cast<Podcasts::PodcastMetaCommon *>( index.internalPointer() );
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

#include "PodcastCategory.moc"
