/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BrowserCategoryList.h"

#include "App.h"
#include "Debug.h"
#include "BrowserCategoryListDelegate.h"
#include "context/ContextView.h"
#include "InfoProxy.h"
#include "PaletteHandler.h"
#include "widgets/PrettyTreeView.h"
#include "widgets/SearchWidget.h"

#include <KLineEdit>
#include <KStandardDirs>

#include <QFile>


BrowserCategoryList::BrowserCategoryList( QWidget * parent, const QString& name )
    : BrowserCategory( name )
    , m_currentCategory( 0 )
    , m_categoryListModel( new BrowserCategoryListModel() )
{
    setObjectName( name );
    setParent( parent );

    debug() << "BrowserCategoryList named " << name << " starting...";

    m_searchWidget = new SearchWidget( this, this, false );
    m_searchWidget->setClickMessage( i18n( "Filter Music Sources" ) );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );

    m_categoryListView = new Amarok::PrettyTreeView( this );
#ifdef Q_WS_MAC
    m_categoryListView->setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // for some bizarre reason w/ some styles on mac
    m_categoryListView->setHorizontalScrollMode( QAbstractItemView::ScrollPerItem ); // per-pixel scrolling is slower than per-item
#else
    m_categoryListView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
    m_categoryListView->setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
#endif

    m_categoryListView->setFrameShape( QFrame::NoFrame );

    m_proxyModel = new BrowserCategoryListSortFilterProxyModel( this );
    m_proxyModel->setSourceModel( m_categoryListModel );

    m_delegate = new BrowserCategoryListDelegate( m_categoryListView );
    m_categoryListView->setItemDelegate( m_delegate );
    m_categoryListView->setSelectionMode( QAbstractItemView::NoSelection );
    m_categoryListView->setHeaderHidden( true );
    m_categoryListView->setRootIsDecorated( false );
    m_categoryListView->setSortingEnabled( true );
    m_categoryListView->setAlternatingRowColors( true );
    m_categoryListView->setModel( m_proxyModel );
    m_categoryListView->setMouseTracking ( true );

    connect( m_categoryListView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( categoryActivated( const QModelIndex & ) ) );

    connect( m_categoryListView, SIGNAL( entered( const QModelIndex & ) ), this, SLOT( categoryEntered( const QModelIndex & ) ) );

    The::paletteHandler()->updateItemView( m_categoryListView );

    setFrameShape( QFrame::NoFrame );

}


BrowserCategoryList::~BrowserCategoryList()
{
    DEBUG_BLOCK
    qDeleteAll( m_categories.values() );
    delete m_categoryListView;
    delete m_categoryListModel;
    delete m_delegate;
}

void
BrowserCategoryList::addCategory( BrowserCategory * category )
{
    if( !category )
        return;

    category->setParentList( this );

    //insert service into service map
    m_categories[category->name()] = category;
    m_categoryListModel->addCategory( category );
    connect( category, SIGNAL( home() ), this, SLOT( home() ) );

    //if this is also a category list, watch it for changes as we need to report
    //these down the tree

    BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( category );
    if ( childList )
        connect( childList, SIGNAL( viewChanged() ), this, SLOT( childViewChanged() ) );
}

void
BrowserCategoryList::categoryActivated( const QModelIndex & index )
{
    DEBUG_BLOCK
    BrowserCategory * category = 0;

    if ( index.data( CustomCategoryRoles::CategoryRole ).canConvert<BrowserCategory *>() )
        category = index.data( CustomCategoryRoles::CategoryRole ).value<BrowserCategory *>();
    else
        return;

    if ( category )
    {
        debug() << "Show service: " <<  category->name();
        showCategory( category->name() );
        emit( viewChanged() );
    }
}

void
BrowserCategoryList::showCategory( const QString &name )
{
    DEBUG_BLOCK
    BrowserCategory * category = 0;
    if ( m_categories.contains( name ) )
        category = m_categories.value( name );

    if ( category != 0 && category != m_currentCategory )
    {
        //if a service is already shown, make damn sure to deactivate that one first...
        if ( m_currentCategory )
            m_currentCategory->setParent( 0 );

        m_categoryListView->setParent( 0 );
        category->setParent ( this );
        category->move( QPoint( 0, 0 ) );
        category->show();
        category->polish();
        m_currentCategory = category;
    }

    m_searchWidget->hide();

    emit( viewChanged() );
}

void
BrowserCategoryList::home()
{
    if ( m_currentCategory != 0 )
    {

        BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( m_currentCategory );
        if ( childList )
            childList->home();

        m_currentCategory->setParent( 0 );
        m_categoryListView->setParent( this );
        m_currentCategory = 0; // remove any context stuff we might have added
        m_searchWidget->show();

        emit( viewChanged() );
    }
}


QMap< QString, BrowserCategory * >
BrowserCategoryList::categories()
{
    return m_categories;
}

void
BrowserCategoryList::removeCategory( const QString &name )
{
    DEBUG_BLOCK
    debug() << "removing category: " << name;
    BrowserCategory * category = m_categories.take( name );
    if ( m_currentCategory == category )
        home();

    if( category )
        m_categoryListModel->removeCategory( category );
    delete category;
    m_categoryListView->reset();
}

void BrowserCategoryList::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_currentFilter = lineEdit->text();
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void BrowserCategoryList::slotFilterNow()
{
    m_proxyModel->setFilterFixedString( m_currentFilter );
}

QString BrowserCategoryList::activeCategoryName()
{
    DEBUG_BLOCK
    if ( m_currentCategory )
        return m_currentCategory->name();
    return QString();
}

BrowserCategory * BrowserCategoryList::activeCategory()
{
    return m_currentCategory;
}

void BrowserCategoryList::back()
{
    BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( m_currentCategory );
    if ( childList )
    {
        if ( childList->activeCategory() != 0 )
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
    showCategory( childName );

    //check if this category is also BrowserCategoryList.target
    BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( m_currentCategory );

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

    BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( m_currentCategory );

    if ( childList )
        pathString += "/" + childList->path();
    else if ( m_currentCategory )
        pathString += "/" + m_currentCategory->name();

    debug() << "path: " << pathString;
    return pathString;
}

void BrowserCategoryList::activate( BrowserCategory * category )
{
    DEBUG_BLOCK
    showCategory( category->name() );
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
        currentHtml.replace( "%%IMAGE_PATH%%", category->imagePath() );

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

QString BrowserCategoryList::filter() const
{
    return m_currentFilter;
}

BrowserCategory *BrowserCategoryList::activeCategoryRecursive()
{
    BrowserCategory *category = activeCategory();

    if( !category )
        return this;

    BrowserCategoryList *childList = dynamic_cast<BrowserCategoryList*>( m_currentCategory );

    if( childList )
        return childList->activeCategoryRecursive();

    return category;
}

void BrowserCategoryList::setFilter( const QString &filter )
{
    m_currentFilter = filter;
    m_searchWidget->setSearchString( filter );
}


#include "BrowserCategoryList.moc"
