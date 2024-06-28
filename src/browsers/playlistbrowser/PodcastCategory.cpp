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

#define DEBUG_PREFIX "PodcastCategory"

#include "PodcastCategory.h"

#include "amarokconfig.h"
#include "amarokurls/AmarokUrl.h"
#include "App.h"
#include "browsers/InfoProxy.h"
#include "core/support/Debug.h"
#include "core/meta/support/MetaUtility.h"
#include "PaletteHandler.h"
#include "PodcastModel.h"
#include "PlaylistBrowserView.h"
#include "widgets/PrettyTreeRoles.h"

#include <QModelIndexList>
#include <QTextBrowser>

#include <QAction>
#include <QIcon>
#include <QStandardPaths>
#include <KUrlRequesterDialog>

#include <KLocalizedString>
#include <KToolBar>

namespace The
{
    PlaylistBrowserNS::PodcastCategory* podcastCategory()
    {
        return PlaylistBrowserNS::PodcastCategory::instance();
    }
}

using namespace PlaylistBrowserNS;

QString PodcastCategory::s_configGroup( QStringLiteral("Podcast View") );

PodcastCategory* PodcastCategory::s_instance = nullptr;

PodcastCategory*
PodcastCategory::instance()
{
    return s_instance ? s_instance : new PodcastCategory( nullptr );
}

void
PodcastCategory::destroy()
{
    if( s_instance )
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

PodcastCategory::PodcastCategory( QWidget *parent )
    : PlaylistBrowserCategory( Playlists::PodcastChannelPlaylist,
                               QStringLiteral("podcasts"),
                               s_configGroup,
                               The::podcastModel(),
                               parent )
{
    setPrettyName( i18n( "Podcasts" ) );
    setShortDescription( i18n( "List of podcast subscriptions and episodes" ) );
    setIcon( QIcon::fromTheme( QStringLiteral("podcast-amarok") ) );

    setLongDescription( i18n( "Manage your podcast subscriptions and browse individual episodes. "
                              "Downloading episodes to the disk is also done here, or you can tell "
                              "Amarok to do this automatically." ) );

    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/hover_info_podcasts.png") ) );

    // set background
    if( AmarokConfig::showBrowserBackgroundImage() )
        setBackgroundImage( imagePath() );

    QAction *addPodcastAction = new QAction( QIcon::fromTheme( QStringLiteral("list-add-amarok") ), i18n("&Add Podcast"),
                                             m_toolBar );
    addPodcastAction->setPriority( QAction::NormalPriority );
    m_toolBar->insertAction( m_separator, addPodcastAction );
    connect( addPodcastAction, &QAction::triggered, The::podcastModel(), &PodcastModel::addPodcast );

    QAction *updateAllAction = new QAction( QIcon::fromTheme(QStringLiteral("view-refresh-amarok")), QString(), m_toolBar );
    updateAllAction->setToolTip( i18n("&Update All") );
    updateAllAction->setPriority( QAction::LowPriority );
    m_toolBar->insertAction( m_separator, updateAllAction );
    connect( updateAllAction, &QAction::triggered,
             The::podcastModel(), &PodcastModel::refreshPodcasts );


    QAction *importOpmlAction = new QAction( QIcon::fromTheme(QStringLiteral("document-import"))
                                             , i18n( "Import OPML File" )
                                             , m_toolBar
                                         );
    importOpmlAction->setToolTip( i18n( "Import OPML File" ) );
    importOpmlAction->setPriority( QAction::LowPriority );
    m_toolBar->addAction( importOpmlAction );
    connect( importOpmlAction, &QAction::triggered, this, &PodcastCategory::slotImportOpml );

    PlaylistBrowserView *view = static_cast<PlaylistBrowserView*>( playlistView() );
    connect( view, &PlaylistBrowserView::currentItemChanged, this, &PodcastCategory::showInfo );

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
}

PodcastCategory::~PodcastCategory()
{
}

void
PodcastCategory::showInfo( const QModelIndex &index )
{
    if( !index.isValid() )
        return;

    const int row = index.row();
    QString description;
    QString title( index.data( Qt::DisplayRole ).toString() );
    QString subtitle( index.sibling( row, SubtitleColumn ).data( Qt::DisplayRole ).toString() );
    QUrl imageUrl( qvariant_cast<QUrl>(
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
        authorAndPubDate = QStringLiteral( "<b>%1</b> %2 " )
            .arg( i18n( "By" ),
                  author.toHtmlEscaped() );
    }

    if( !subtitle.isEmpty() )
    {
        description += QStringLiteral( "<h1 class=\"subtitle\">%1</h1>" )
            .arg( subtitle.toHtmlEscaped() );
    }

    if( !imageUrl.isEmpty() )
    {
        description += QStringLiteral( "<p style=\"float:right;\"><img src=\"%1\" onclick=\""
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
            .arg( imageUrl.url().toHtmlEscaped() );
    }

    if( isEpisode )
    {
        QDateTime pubDate( index.sibling( row, DateColumn ).data( Qt::DisplayRole ).toDateTime() );
        
        if( pubDate.isValid() )
        {
            authorAndPubDate += QStringLiteral( "<b>%1</b> %2" )
                .arg( i18nc( "Podcast published on date", "On" ),
                      QLocale().toString( pubDate, QLocale::ShortFormat ) );
        }
    }

    if( !authorAndPubDate.isEmpty() )
    {
        description += QStringLiteral( "<p>%1</p>" )
            .arg( authorAndPubDate );
    }

    if( isEpisode )
    {
        int fileSize = index.sibling( row, FilesizeColumn ).data( Qt::DisplayRole ).toInt();

        if( fileSize != 0 )
        {
            description += QStringLiteral( "<p><b>%1</b> %2</p>" )
                .arg( i18n( "File Size:" ),
                      Meta::prettyFilesize( fileSize ) );
        }

    }
    else
    {
        QDate subsDate( index.sibling( row, DateColumn ).data( Qt::DisplayRole ).toDate() );
        
        if( subsDate.isValid() )
        {
            description += QStringLiteral( "<p><b>%1</b> %2</p>" )
                .arg( i18n( "Subscription Date:" ),
                      QLocale().toString( subsDate, QLocale::ShortFormat ) );
        }
    }

    if( !keywords.isEmpty() )
    {
        description += QStringLiteral( "<p><b>%1</b> %2</p>" )
            .arg( i18n( "Keywords:" ),
                  keywords.join( QStringLiteral(", ") ).toHtmlEscaped() );
    }

    description += index.data( PrettyTreeRoles::ByLineRole ).toString();

    description = QString(QStringLiteral(
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
        "</html>"))
        .arg( title.toHtmlEscaped(),
              description,
              pApp->palette().brush( QPalette::Text ).color().name(),
              The::paletteHandler()->highlightColor().name() );
    
    QVariantMap map;
    map[QStringLiteral("service_name")] = title;
    map[QStringLiteral("main_info")] = description;
    The::infoProxy()->setInfo( map );
}

void
PodcastCategory::slotImportOpml()
{
    AmarokUrl( QStringLiteral("amarok://service-podcastdirectory/addOpml") ).run();
}

