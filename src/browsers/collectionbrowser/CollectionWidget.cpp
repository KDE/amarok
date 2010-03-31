/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008-2009 Dan Meltzer <parallelgrapefruit@gmail.com>                   *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CollectionWidget.h"

#include "CollectionTreeItemModel.h"
#include "CollectionTreeItemModelBase.h"
#include "CollectionTreeItemDelegate.h"
#include "CollectionBrowserTreeView.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/proxycollection/ProxyCollection.h"
#include "core/support/Debug.h"
#include "SearchWidget.h"
#include "SingleCollectionTreeItemModel.h"
#include <amarokconfig.h>

#include <KAction>
#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KMenuBar>
#include <KStandardDirs>

#include <QActionGroup>
#include <QMetaEnum>
#include <QMetaObject>
#include <QRect>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>

CollectionWidget *CollectionWidget::s_instance = 0;

CollectionWidget::CollectionWidget( const QString &name , QWidget *parent )
    : BrowserCategory( name, parent )
    , m_viewMode( CollectionWidget::NormalCollections )
{
    s_instance = this;
    setObjectName( name );
    setMargin( 0 );
    setSpacing( 0 );

    QMenu *filterMenu = new QMenu( this );
    KHBox *hbox = new KHBox( this );
    m_searchWidget = new SearchWidget( hbox );
    m_searchWidget->setClickMessage( i18n( "Search collection" ) );

    m_stack = new QStackedWidget( this );
    m_stack->setFrameShape( QFrame::NoFrame );

    m_treeView = new CollectionBrowserTreeView( m_stack );
    m_stack->addWidget( m_treeView );
    m_treeView->setAlternatingRowColors( true );
    m_treeView->setFrameShape( QFrame::NoFrame );
    m_treeView->setRootIsDecorated( false );

    CollectionTreeItemDelegate *delegate = new CollectionTreeItemDelegate( m_treeView );
    m_treeView->setItemDelegate( delegate );

    m_singleTreeView = new CollectionBrowserTreeView( m_stack );
    m_stack->addWidget( m_singleTreeView );
    m_singleTreeView->setAlternatingRowColors( true );
    m_singleTreeView->setFrameShape( QFrame::NoFrame );

    m_levels = Amarok::config( "Collection Browser" ).readEntry( "TreeCategory", QList<int>() );
    if ( m_levels.isEmpty() )
        m_levels << CategoryId::Artist << CategoryId::Album;

    m_multiModel = new CollectionTreeItemModel( m_levels );

    Collections::ProxyCollection *proxyColl = new Collections::ProxyCollection();
    connect( CollectionManager::instance(), SIGNAL(collectionAdded(Collections::Collection*,CollectionManager::CollectionStatus)), proxyColl, SLOT(addCollection(Collections::Collection*,CollectionManager::CollectionStatus)));
    connect( CollectionManager::instance(), SIGNAL(collectionRemoved(QString)), proxyColl, SLOT(removeCollection(QString)));
    foreach( Collections::Collection* coll, CollectionManager::instance()->viewableCollections() )
    {
        proxyColl->addCollection( coll, CollectionManager::CollectionViewable );
    }

    m_singleModel = new SingleCollectionTreeItemModel( proxyColl, m_levels );
    m_treeView->setModel( m_multiModel );
    m_singleTreeView->setModel( m_singleModel );

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "ViewMode" ) );
    const QString &value = KGlobal::config()->group( "Collection Browser" ).readEntry( "View Mode" );
    int enumValue = me.keyToValue( value.toLocal8Bit().constData() );
    enumValue == -1 ? m_viewMode = NormalCollections : m_viewMode = (ViewMode) enumValue;

    QAction *action = new QAction( i18n( "Artist / Album" ), this );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByArtistAlbum() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Genre / Artist" ), this );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( sortByGenreArtist() ) );
    filterMenu->addAction( action );

    action = new QAction( i18n( "Genre / Artist / Album" ), this );
    connect( action, SIGNAL(triggered( bool ) ), SLOT( sortByGenreArtistAlbum() ) );
    filterMenu->addAction( action );

    filterMenu->addSeparator();

    m_firstLevel = filterMenu->addMenu( i18n( "First Level" ) );

    QAction *firstArtistAction   = m_firstLevel->addAction( i18n( "Artist" ) );
    firstArtistAction->setData( CategoryId::Artist );

    QAction *firstAlbumAction    = m_firstLevel->addAction( i18n( "Album"  ) );
    firstAlbumAction->setData( CategoryId::Album );

    QAction *firstGenreAction    = m_firstLevel->addAction( i18n( "Genre"  ) );
    firstGenreAction->setData( CategoryId::Genre );

    QAction *firstComposerAction = m_firstLevel->addAction( i18n( "Composer" ) );
    firstComposerAction->setData( CategoryId::Composer );

    firstArtistAction->setCheckable  ( true );
    firstAlbumAction->setCheckable   ( true );
    firstGenreAction->setCheckable   ( true );
    firstComposerAction->setCheckable( true );

    QActionGroup *firstGroup = new QActionGroup( this );
    firstGroup->addAction( firstArtistAction );
    firstGroup->addAction( firstAlbumAction );
    firstGroup->addAction( firstGenreAction );
    firstGroup->addAction( firstComposerAction );

    connect( m_firstLevel, SIGNAL( triggered( QAction *) ), SLOT( customFilter( QAction * ) ) );

    m_secondLevel = filterMenu->addMenu( i18n( "Second Level" ) );
    QAction *secondNullAction     = m_secondLevel->addAction( i18n( "None" ) );
    secondNullAction->setData( CategoryId::None );

    QAction *secondArtistAction   = m_secondLevel->addAction( i18n( "Artist" ) );
    secondArtistAction->setData( CategoryId::Artist );

    QAction *secondAlbumAction    = m_secondLevel->addAction( i18n( "Album"  ) );
    secondAlbumAction->setData( CategoryId::Album );

    QAction *secondGenreAction    = m_secondLevel->addAction( i18n( "Genre"  ) );
    secondGenreAction->setData( CategoryId::Genre );

    QAction *secondComposerAction = m_secondLevel->addAction( i18n( "Composer" ) );
    secondComposerAction->setData( CategoryId::Composer );

    secondNullAction->setCheckable    ( true );
    secondArtistAction->setCheckable  ( true );
    secondAlbumAction->setCheckable   ( true );
    secondGenreAction->setCheckable   ( true );
    secondComposerAction->setCheckable( true );

    QActionGroup *secondGroup = new QActionGroup( this );
    secondGroup->addAction( secondNullAction );
    secondGroup->addAction( secondArtistAction );
    secondGroup->addAction( secondAlbumAction );
    secondGroup->addAction( secondGenreAction );
    secondGroup->addAction( secondComposerAction );
    secondNullAction->setChecked( true );

    connect( m_secondLevel, SIGNAL( triggered( QAction *) ), SLOT( customFilter( QAction * ) ) );

    m_thirdLevel = filterMenu->addMenu( i18n( "Third Level" ) );
    QAction *thirdNullAction     = m_thirdLevel->addAction( i18n( "None" ) );
    thirdNullAction->setData( CategoryId::None );

    QAction *thirdArtistAction   = m_thirdLevel->addAction( i18n( "Artist" ) );
    thirdArtistAction->setData( CategoryId::Artist );

    QAction *thirdAlbumAction    = m_thirdLevel->addAction( i18n( "Album"  ) );
    thirdAlbumAction->setData( CategoryId::Album );

    QAction *thirdGenreAction    = m_thirdLevel->addAction( i18n( "Genre"  ) );
    thirdGenreAction->setData( CategoryId::Genre );

    QAction *thirdComposerAction = m_thirdLevel->addAction( i18n( "Composer" ) );
    thirdComposerAction->setData( CategoryId::Composer );

    thirdNullAction->setCheckable    ( true );
    thirdArtistAction->setCheckable  ( true );
    thirdAlbumAction->setCheckable   ( true );
    thirdGenreAction->setCheckable   ( true );
    thirdComposerAction->setCheckable( true );

    QActionGroup *thirdGroup = new QActionGroup( this );
    thirdGroup->addAction( thirdNullAction );
    thirdGroup->addAction( thirdArtistAction );
    thirdGroup->addAction( thirdAlbumAction );
    thirdGroup->addAction( thirdGenreAction );
    thirdGroup->addAction( thirdComposerAction );
    thirdNullAction->setChecked( true );

    connect( m_thirdLevel, SIGNAL( triggered( QAction *) ), SLOT( customFilter( QAction * ) ) );

    filterMenu->addSeparator();
    QAction *showYears = filterMenu->addAction( i18n( "Show Years" ) );
    showYears->setCheckable( true );
    showYears->setChecked( AmarokConfig::showYears() );
    connect( showYears, SIGNAL( toggled( bool ) ), SLOT( slotShowYears( bool ) ) );

    QAction *showCovers = filterMenu->addAction( i18n( "Show Cover Art" ) );
    showCovers->setCheckable( true );
    showCovers->setChecked( AmarokConfig::showAlbumArt() );
    connect( showCovers, SIGNAL(toggled(bool)), SLOT( slotShowCovers( bool ) ) );

    //do not use m_levels directly here, otherwise
    //takeFirst() removes the first item from the list,
    //but we need the correct m_levels in toggleView()
    QList<int> levelCopy = m_levels;
    // Preset the checked status properly
    if( levelCopy.size() > 0 )
    {
        //First Category
        const int i = levelCopy.takeFirst();
        switch( i )
        {
            case CategoryId::Artist:
                firstArtistAction->setChecked( true );
                break;
            case CategoryId::Album:
                firstAlbumAction->setChecked( true );
                break;
            case CategoryId::Genre:
                firstGenreAction->setChecked( true );
                break;
            default: //as good a fall through as any, here
                firstComposerAction->setChecked( true );
                break;
        }
    }
    if( levelCopy.size() > 0 ) //We have a second level
    {
        const int i = levelCopy.takeFirst();
        switch( i )
        {
            case CategoryId::Artist:
                secondArtistAction->setChecked( true );
                break;
            case CategoryId::Album:
                secondAlbumAction->setChecked( true );
                break;
            case CategoryId::Genre:
                secondGenreAction->setChecked( true );
                break;
            case CategoryId::Composer:
                secondComposerAction->setChecked( true );
                break;
            default:
                secondNullAction->setChecked( true );
        }
    }
    if( levelCopy.size() > 0 ) //We have a third level
    {
        const int i = levelCopy.takeFirst();
        switch( i )
        {
            case CategoryId::Artist:
                thirdArtistAction->setChecked( true );
                break;
            case CategoryId::Album:
                thirdAlbumAction->setChecked( true );
                break;
            case CategoryId::Genre:
                thirdGenreAction->setChecked( true );
                break;
            case CategoryId::Composer:
                thirdComposerAction->setChecked( true );
                break;
            default:
                thirdNullAction->setChecked( true );
        }
    }
    m_firstLevelSelectedAction = firstGroup->checkedAction();
    m_secondLevelSelectedAction = secondGroup->checkedAction();
    m_thirdLevelSelectedAction = thirdGroup->checkedAction();

    m_searchWidget->toolBar()->addSeparator();

    KAction *searchMenuAction = new KAction( KIcon( "preferences-other" ), i18n( "Sort Options" ), this );
    searchMenuAction->setMenu( filterMenu );

    m_searchWidget->toolBar()->addAction( searchMenuAction );

    KAction *toggleAction = new KAction( KIcon( "view-list-tree" ), i18n( "Merged View" ), this );
    toggleAction->setCheckable( true );
    toggleView( m_viewMode == CollectionWidget::UnifiedCollection );
    connect( toggleAction, SIGNAL( triggered( bool ) ), SLOT( toggleView( bool ) ) );
    m_searchWidget->toolBar()->addAction( toggleAction );

    QToolButton *tbutton = qobject_cast<QToolButton*>( m_searchWidget->toolBar()->widgetForAction( searchMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );
    
    //TODO: we have a really nice opportunity to make these info blurbs both helpful and pretty
    setLongDescription( i18n( "This is where you will find your local music, as well as music from mobile audio players and CDs." ) );

    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_collections.png" ) );

}

void
CollectionWidget::customFilter( QAction *action )
{
    QMenu *menu = qobject_cast<QMenu*>( sender() );

    if( menu == m_firstLevel )
        m_firstLevelSelectedAction = action;
    else if( menu == m_secondLevel )
        m_secondLevelSelectedAction = action;
    else
        m_thirdLevelSelectedAction = action;

    const int firstLevel = m_firstLevelSelectedAction->data().toInt();
    const int secondLevel = m_secondLevelSelectedAction->data().toInt();
    const int thirdLevel = m_thirdLevelSelectedAction->data().toInt();
    m_levels.clear();
    m_levels << firstLevel;
    if( secondLevel != CategoryId::None )
        m_levels << secondLevel;
    if( thirdLevel != CategoryId::None )
        m_levels << thirdLevel;
    setLevels( m_levels );
}

void
CollectionWidget::sortByArtistAlbum()
{
    m_levels.clear();
    m_levels << CategoryId::Artist << CategoryId::Album;
    setLevels( m_levels );
}

void
CollectionWidget::sortByGenreArtist()
{
    m_levels.clear();
    m_levels << CategoryId::Genre << CategoryId::Artist;
    setLevels( m_levels );
}

void
CollectionWidget::sortByGenreArtistAlbum()
{
    m_levels.clear();
    m_levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;
    setLevels( m_levels );
}

void CollectionWidget::sortByAlbum()
{
    m_levels.clear();
    m_levels << CategoryId::Album;
    setLevels( m_levels );
}

void CollectionWidget::sortByArtist()
{
    m_levels.clear();
    m_levels << CategoryId::Artist;
    setLevels( m_levels );
}

void
CollectionWidget::slotShowYears( bool checked )
{
    AmarokConfig::setShowYears( checked );
    setLevels( levels() );
}

void
CollectionWidget::slotShowCovers(bool checked)
{
    AmarokConfig::setShowAlbumArt( checked );
    setLevels( levels() );
}


void CollectionWidget::setFilter( const QString &filter )
{
    m_searchWidget->setSearchString( filter );
}

QString
CollectionWidget::filter() const
{
    return m_searchWidget->lineEdit()->text();
}

QList<int>
CollectionWidget::levels() const
{
    return m_viewMode == CollectionWidget::NormalCollections ? m_treeView->levels() : m_singleTreeView->levels();
}

void CollectionWidget::setLevels( const QList<int> &levels )
{
    m_levels = levels;
    m_viewMode == CollectionWidget::NormalCollections ? m_treeView->setLevels( m_levels ) : m_singleTreeView->setLevels( m_levels );
}

void CollectionWidget::toggleView( bool merged )
{
    if( merged )
    {
        debug() << "Switching to merged model";
        m_searchWidget->disconnect( m_treeView );
        m_searchWidget->setup( m_singleTreeView );
        m_stack->setCurrentWidget( m_singleTreeView );
        m_singleModel->setCurrentFilter( m_searchWidget->lineEdit()->text() );
        m_singleTreeView->slotFilterNow();
        if( m_levels != m_singleTreeView->levels() )
            m_singleTreeView->setLevels( m_levels );
        m_viewMode = CollectionWidget::UnifiedCollection;
    }
    else
    {
        debug() << "switching to multi model";
        m_searchWidget->disconnect( m_singleTreeView );
        m_searchWidget->setup( m_treeView );
        m_stack->setCurrentWidget( m_treeView );
        m_multiModel->setCurrentFilter( m_searchWidget->lineEdit()->text() );
        m_treeView->slotFilterNow();
        if( m_levels != m_treeView->levels() )
            m_treeView->setLevels( m_levels );
        m_viewMode = CollectionWidget::NormalCollections;
    }

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "ViewMode" ) );
    KGlobal::config()->group( "Collection Browser" ).writeEntry( "View Mode",
                                                                 me.valueToKey( m_viewMode ) );
}

#include "CollectionWidget.moc"
