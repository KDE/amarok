/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "BrowserCategoryList"

#include "BrowserCategoryList.h"

#include "App.h"
#include "context/ContextView.h"
#include "core/support/Debug.h"
#include "InfoProxy.h"
#include "PaletteHandler.h"
#include "widgets/PrettyTreeView.h"
#include "widgets/PrettyTreeDelegate.h"
#include "widgets/SearchWidget.h"

#include <QStackedWidget>
#include <QTreeView>

#include <KComboBox>
#include <KStandardDirs>

#include <QFile>

BrowserCategoryList::BrowserCategoryList( const QString &name, QWidget* parent, bool sort )
    : BrowserCategory( name, parent )
    , m_categoryListModel( new BrowserCategoryListModel( this ) )
    , m_sorting( sort )
{
    // -- the widget stack
    m_widgetStack = new QStackedWidget( this );

    QWidget* mainWidget = new QWidget( m_widgetStack );
    QVBoxLayout* vLayout = new QVBoxLayout( mainWidget );
    mainWidget->setLayout( vLayout );

    // -- the search widget
    m_searchWidget = new SearchWidget( this, false );
    m_searchWidget->setClickMessage( i18n( "Filter Music Sources" ) );
    vLayout->addWidget( m_searchWidget );

    connect( m_searchWidget, SIGNAL(filterChanged(QString)), SLOT(setFilter(QString)) );

    // -- the main list view
    m_categoryListView = new Amarok::PrettyTreeView();
    m_categoryListView->setFrameShape( QFrame::NoFrame );

    m_proxyModel = new BrowserCategoryListSortFilterProxyModel( this );
    m_proxyModel->setSourceModel( m_categoryListModel );

    m_categoryListView->setItemDelegate( new PrettyTreeDelegate( m_categoryListView ) );
    m_categoryListView->setHeaderHidden( true );
    m_categoryListView->setRootIsDecorated( false );
    m_categoryListView->setModel( m_proxyModel );
    m_categoryListView->setMouseTracking ( true );

    if( sort )
    {
        m_proxyModel->setSortRole( Qt::DisplayRole );
        m_categoryListView->setSortingEnabled( true );
        m_categoryListView->sortByColumn( 0 );
    }

    connect( m_categoryListView, SIGNAL(activated(QModelIndex)),
            SLOT(categoryActivated(QModelIndex)) );

    connect( m_categoryListView, SIGNAL(entered(QModelIndex)),
            SLOT(categoryEntered(QModelIndex)) );

    vLayout->addWidget( m_categoryListView );
    m_widgetStack->addWidget( mainWidget );
}

BrowserCategoryList::~BrowserCategoryList()
{ }


void
BrowserCategoryList::categoryActivated( const QModelIndex &index )
{
    DEBUG_BLOCK
    BrowserCategory * category = 0;

    if( index.data( CustomCategoryRoles::CategoryRole ).canConvert<BrowserCategory *>() )
        category = index.data( CustomCategoryRoles::CategoryRole ).value<BrowserCategory *>();
    else
        return;

    if( category )
    {
        debug() << "Show service: " <<  category->name();
        setActiveCategory( category );
    }
}

void
BrowserCategoryList::home()
{
    DEBUG_BLOCK
    if( activeCategory() )
    {
        BrowserCategoryList *childList = qobject_cast<BrowserCategoryList*>( activeCategory() );
        if( childList )
            childList->home();

        activeCategory()->clearAdditionalItems();
        m_widgetStack->setCurrentIndex( 0 );

        emit( viewChanged() );
    }
}


QMap<QString, BrowserCategory*>
BrowserCategoryList::categories()
{
    return m_categories;
}

void
BrowserCategoryList::addCategory( BrowserCategory *category )
{
    Q_ASSERT( category );

    category->setParentList( this );

    //insert service into service map
    category->setParent( this );
    m_categories[category->name()] = category;
    m_categoryListModel->addCategory( category );
    m_widgetStack->addWidget( category );

    //if this is also a category list, watch it for changes as we need to report
    //these down the tree

    BrowserCategoryList *childList = qobject_cast<BrowserCategoryList*>( category );
    if ( childList )
        connect( childList, SIGNAL(viewChanged()), this, SLOT(childViewChanged()) );

    category->polish(); // service categories do an additional construction in polish

    if( m_sorting )
    {
        m_proxyModel->sort( 0 );
    }
    emit( viewChanged() );
}


void
BrowserCategoryList::removeCategory( BrowserCategory *category )
{
    Q_ASSERT( category );

    if( m_widgetStack->indexOf( category ) == -1 )
        return; // no such category

    if( m_widgetStack->currentWidget() == category )
        home();

    m_categories.remove( category->name() );
    m_categoryListModel->removeCategory( category );
    m_widgetStack->removeWidget( category );
    delete category;

    m_categoryListView->reset();

    emit( viewChanged() );
}

BrowserCategory*
BrowserCategoryList::activeCategory() const
{
    return qobject_cast<BrowserCategory*>(m_widgetStack->currentWidget());
}

void BrowserCategoryList::setActiveCategory( BrowserCategory* category )
{
    DEBUG_BLOCK;

    if( m_widgetStack->indexOf( category ) == -1 )
        return; // no such category

    if( !category || activeCategory() == category )
        return; // nothing to do

    if( activeCategory() )
        activeCategory()->clearAdditionalItems();
    category->setupAddItems();

    m_widgetStack->setCurrentWidget( category );

    emit( viewChanged() );
}

void BrowserCategoryList::back()
{
    DEBUG_BLOCK

    BrowserCategoryList *childList = qobject_cast<BrowserCategoryList*>( activeCategory() );
    if( childList )
    {
        if( childList->activeCategory() != 0 )
        {
            childList->back();
            return;
        }
    }

    home();
}

void BrowserCategoryList::childViewChanged()
{
    DEBUG_BLOCK
    emit( viewChanged() );
}

QString BrowserCategoryList::navigate( const QString & target )
{
    DEBUG_BLOCK
    debug() << "target: " << target;
    QStringList categories = target.split( '/' );
    if ( categories.size() == 0 )
        return QString();

    //remove our own name if present, before passing on...
    if ( categories.at( 0 ) == name() )
    {
        debug() << "removing own name (" << categories.at( 0 ) << ") from path";
        categories.removeFirst();

        if ( categories.size() == 0 )
        {
            //nothing else left, make sure this category is visible
            home();
            return QString();
        }
    }

    QString childName = categories.at( 0 );
    debug() << "looking for child category " << childName;
    if ( !m_categories.contains( childName ) )
        return target;


    debug() << "got it!";
    setActiveCategory( m_categories[childName] );

    //check if this category is also BrowserCategoryList.target
    BrowserCategoryList *childList = qobject_cast<BrowserCategoryList*>( activeCategory() );

    if ( childList == 0 )
    {
        debug() << "child is not a list...";
        if ( categories.size() > 1 )
        {
            categories.removeFirst();
            QString leftover = categories.join( "/" );
            return leftover;
        }
        return QString();

    }

    //check if there are more arguments in the navigate string.
    if ( categories.size() == 1 )
    {
        debug() << "Child is a list but path ends here...";
        //only one name, but since the category we switched to is also
        //a category list, make sure that it is reset to home
        childList->home();
        return QString();
    }

    categories.removeFirst();
    debug() << "passing remaining path to child: " << categories.join( "/" );
    return childList->navigate( categories.join( "/" ) );

}

QString BrowserCategoryList::path()
{
    DEBUG_BLOCK
    QString pathString = name();

    BrowserCategoryList *childList = qobject_cast<BrowserCategoryList*>( activeCategory() );

    if( childList )
        pathString += '/' + childList->path();
    else if( activeCategory() )
        pathString += '/' + activeCategory()->name();

    debug() << "path: " << pathString;
    return pathString;
}

void BrowserCategoryList::categoryEntered( const QModelIndex & index )
{
    //get the long description for this item and pass it it to info proxy.

    BrowserCategory *category = 0;

    if ( index.data( CustomCategoryRoles::CategoryRole ).canConvert<BrowserCategory *>() )
        category = index.data( CustomCategoryRoles::CategoryRole ).value<BrowserCategory *>();
    else
        return;

    if( category )
    {

        //instead of just throwing out raw text, let's format the long description and the
        //icon into a nice html page.

        if ( m_infoHtmlTemplate.isEmpty() )
        {

            KUrl dataUrl( KStandardDirs::locate( "data", "amarok/data/" ) );
            QString dataPath = dataUrl.path();

            //load html
            QString htmlPath = dataPath + "hover_info_template.html";
            QFile file( htmlPath );
            if ( !file.open( QIODevice::ReadOnly | QIODevice::Text) )
            {
                debug() << "error opening file. Error: " << file.error();
                return;
            }
            m_infoHtmlTemplate = file.readAll();
            file.close();

            m_infoHtmlTemplate.replace( "{background_color}",PaletteHandler::highlightColor().lighter( 150 ).name() );
            m_infoHtmlTemplate.replace( "{border_color}", PaletteHandler::highlightColor().lighter( 150 ).name() );
            m_infoHtmlTemplate.replace( "{text_color}", App::instance()->palette().brush( QPalette::Text ).color().name() );
            QColor highlight( App::instance()->palette().highlight().color() );
            highlight.setHsvF( highlight.hueF(), 0.3, .95, highlight.alphaF() );
            m_infoHtmlTemplate.replace( "{header_background_color}", highlight.name() );

        }

        QString currentHtml = m_infoHtmlTemplate;

        currentHtml.replace( "%%NAME%%", category->prettyName() );
        currentHtml.replace( "%%DESCRIPTION%%", category->longDescription() );
        currentHtml.replace( "%%IMAGE_PATH%%", "file://" + category->imagePath() );

        QVariantMap variantMap;
        variantMap["main_info"] = QVariant( currentHtml );
        The::infoProxy()->setInfo( variantMap );
    }
}

QString BrowserCategoryList::css()
{
    QString style =
            "<style type='text/css'>"
            "body"
            "{"
            "    text-align:center;"
            "    background-color: {background_color};"
            "}"
            "#main"
            "    {"
            "        text-align: center;"
            "    }"
            ""
            "#text-border"
            "    {"
            "        display: block;"
            "        margin-left: 0;"
            "        margin-right: 0;"
            "        padding: 4px;"
            "        border: 4px solid {border_color};"
            "        -webkit-border-radius: 4px;"
            "        -khtml-border-radius: 4px;"
            "        -moz-border-radius: 4px;"
            "        border-radius: 4px;"
            "        font-size: 94%;"
            "        text-align: center;"
            "        word-wrap: normal;"
            "        background-color: {content_background_color};"
            "        color: {text_color};"
            "    }"
            "</style>";

    return style;
}

BrowserCategory *BrowserCategoryList::activeCategoryRecursive()
{
    BrowserCategory *category = activeCategory();

    if( !category )
        return this;

    BrowserCategoryList *childList = qobject_cast<BrowserCategoryList*>( category );
    if( childList )
        return childList->activeCategoryRecursive();

    return category;
}

void BrowserCategoryList::setFilter( const QString &filter )
{
    m_proxyModel->setFilterFixedString( filter );
}

