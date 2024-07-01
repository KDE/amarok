/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008-2009 Dan Meltzer <parallelgrapefruit@gmail.com>                   *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "CollectionWidget"

#include "CollectionWidget.h"

#include "amarokconfig.h"
#include "browsers/CollectionTreeItemModel.h"
#include "browsers/CollectionTreeItemModelBase.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "browsers/collectionbrowser/CollectionBrowserTreeView.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/aggregate/AggregateCollection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "widgets/SearchWidget.h"
#include "widgets/PrettyTreeDelegate.h"

#include <KLocalizedString>
#include <KStandardGuiItem>

#include <QActionGroup>
#include <QIcon>
#include <QMenu>
#include <QMetaEnum>
#include <QMetaObject>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QToolBar>
#include <QToolButton>

CollectionWidget *CollectionWidget::s_instance = nullptr;

#define CATEGORY_LEVEL_COUNT 3

Q_DECLARE_METATYPE( QList<CategoryId::CatMenuId> ) // needed to QAction payload

class CollectionWidget::Private
{
public:
    Private()
        : treeView( nullptr )
        , singleTreeView( nullptr )
        , viewMode( CollectionWidget::NormalCollections ) {}
    ~Private() {}

    CollectionBrowserTreeView *view( CollectionWidget::ViewMode mode );

    CollectionBrowserTreeView *treeView;
    CollectionBrowserTreeView *singleTreeView;
    QStackedWidget *stack;
    SearchWidget *searchWidget;
    CollectionWidget::ViewMode viewMode;

    QMenu *menuLevel[CATEGORY_LEVEL_COUNT];
    QActionGroup *levelGroups[CATEGORY_LEVEL_COUNT];
};

CollectionBrowserTreeView *
CollectionWidget::Private::view( CollectionWidget::ViewMode mode )
{
    CollectionBrowserTreeView *v(nullptr);

    switch( mode )
    {
    case CollectionWidget::NormalCollections:
        if( !treeView )
        {
            v = new CollectionBrowserTreeView( stack );
            v->setAlternatingRowColors( true );
            v->setFrameShape( QFrame::NoFrame );
            v->setRootIsDecorated( false );
            connect( v, &CollectionBrowserTreeView::leavingTree,
                     searchWidget->comboBox(), QOverload<>::of(&QWidget::setFocus) );
            PrettyTreeDelegate *delegate = new PrettyTreeDelegate( v );
            v->setItemDelegate( delegate );
            CollectionTreeItemModelBase *multiModel = new CollectionTreeItemModel( QList<CategoryId::CatMenuId>() );
            multiModel->setParent( stack );
            v->setModel( multiModel );
            treeView = v;
        }
        else
        {
            v = treeView;
        }
        break;

    case CollectionWidget::UnifiedCollection:
        if( !singleTreeView )
        {
            v = new CollectionBrowserTreeView( stack );
            v->setAlternatingRowColors( true );
            v->setFrameShape( QFrame::NoFrame );
            Collections::AggregateCollection *aggregateColl = new Collections::AggregateCollection();
            connect( CollectionManager::instance(), &CollectionManager::collectionAdded,
                     aggregateColl, &Collections::AggregateCollection::addCollection );
            connect( CollectionManager::instance(), &CollectionManager::collectionRemoved,
                     aggregateColl, &Collections::AggregateCollection::removeCollectionById );
            for( Collections::Collection* coll : CollectionManager::instance()->viewableCollections() )
            {
                aggregateColl->addCollection( coll, CollectionManager::CollectionViewable );
            }
            CollectionTreeItemModelBase *singleModel = new SingleCollectionTreeItemModel( aggregateColl, QList<CategoryId::CatMenuId>() );
            singleModel->setParent( stack );
            v->setModel( singleModel );
            singleTreeView = v;
        }
        else
        {
            v = singleTreeView;
        }
        break;
    }
    return v;
}

CollectionWidget::CollectionWidget( const QString &name , QWidget *parent )
    : BrowserCategory( name, parent )
    , d( new Private )
{
    s_instance = this;
    setObjectName( name );
    //TODO: we have a really nice opportunity to make these info blurbs both helpful and pretty
    setLongDescription( i18n( "This is where you will find your local music, as well as music from mobile audio players and CDs." ) );
    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/hover_info_collections.png") ) );

    // set background
    if( AmarokConfig::showBrowserBackgroundImage() )
        setBackgroundImage( imagePath() );

    // --- the box for the UI elements.
    BoxWidget *hbox = new BoxWidget( false, this );

    d->stack = new QStackedWidget( this );

    // -- read the current view mode from the configuration
    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "ViewMode" ) );
    const QString &value = Amarok::config( QStringLiteral("Collection Browser") ).readEntry( "View Mode" );
    int enumValue = me.keyToValue( value.toLocal8Bit().constData() );
    enumValue == -1 ? d->viewMode = NormalCollections : d->viewMode = (ViewMode) enumValue;

    // -- the search widget
    d->searchWidget = new SearchWidget( hbox );
    d->searchWidget->setClickMessage( i18n( "Search collection" ) );

    // Filter presets. UserRole is used to store the actual syntax.
    QComboBox *combo = d->searchWidget->comboBox();
    const QIcon icon = KStandardGuiItem::find().icon();
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added This Hour"),
                    QString(Meta::shortI18nForField( Meta::valCreateDate ) + QStringLiteral(":<1h")) );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added Today"),
                    QString(Meta::shortI18nForField( Meta::valCreateDate ) + QStringLiteral(":<1d")) );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added This Week"),
                    QString(Meta::shortI18nForField( Meta::valCreateDate ) + QStringLiteral(":<1w")) );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added This Month"),
                    QString(Meta::shortI18nForField( Meta::valCreateDate ) + QStringLiteral(":<1m")) );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Never Played"),
                    QString(Meta::shortI18nForField( Meta::valPlaycount ) + QStringLiteral(":0")) );
    combo->insertSeparator( combo->count() );

    QMenu *filterMenu = new QMenu( nullptr );

    using namespace CategoryId;
    static const QList<QList<CatMenuId> > levelPresets = QList<QList<CatMenuId> >()
        << ( QList<CatMenuId>() << CategoryId::AlbumArtist << CategoryId::Album )
        << ( QList<CatMenuId>() << CategoryId::Album << CategoryId::Artist ) // album artist has no sense here
        << ( QList<CatMenuId>() << CategoryId::Genre << CategoryId::AlbumArtist )
        << ( QList<CatMenuId>() << CategoryId::Genre << CategoryId::AlbumArtist << CategoryId::Album );
    for( const QList<CatMenuId> &levels : levelPresets )
    {
        QStringList categoryLabels;
        for( CatMenuId category : levels )
            categoryLabels << CollectionTreeItemModelBase::nameForCategory( category );
        QAction *action = filterMenu->addAction( categoryLabels.join( i18nc(
                "separator between collection browser level categories, i.e. the ' / ' "
                "in 'Artist / Album'", " / " ) ) );
        action->setData( QVariant::fromValue( levels ) );
    }
    // following catches all actions in the filter menu
    connect( filterMenu, &QMenu::triggered, this, &CollectionWidget::sortByActionPayload );
    filterMenu->addSeparator();

    // -- read the view level settings from the configuration
    QList<CategoryId::CatMenuId> levels = readLevelsFromConfig();
    if ( levels.isEmpty() )
        levels << levelPresets.at( 0 ); // use first preset as default

    // -- generate the level menus
    d->menuLevel[0] = filterMenu->addMenu( i18n( "First Level" ) );
    d->menuLevel[1] = filterMenu->addMenu( i18n( "Second Level" ) );
    d->menuLevel[2] = filterMenu->addMenu( i18n( "Third Level" ) );

    // - fill the level menus
    static const QList<CatMenuId> levelChoices = QList<CatMenuId>()
            << CategoryId::AlbumArtist
            << CategoryId::Artist
            << CategoryId::Album
            << CategoryId::Genre
            << CategoryId::Composer
            << CategoryId::Label;
    for( int i = 0; i < CATEGORY_LEVEL_COUNT; i++ )
    {
        QList<CatMenuId> usedLevelChoices = levelChoices;
        QActionGroup *actionGroup = new QActionGroup( this );
        if( i > 0 ) // skip first submenu
            usedLevelChoices.prepend( CategoryId::None );

        QMenu *menuLevel = d->menuLevel[i];
        for( CatMenuId level : usedLevelChoices )
        {
            QAction *action = menuLevel->addAction( CollectionTreeItemModelBase::nameForCategory( level ) );
            action->setData( QVariant::fromValue<CatMenuId>( level ) );
            action->setCheckable( true );
            action->setChecked( ( levels.count() > i ) ? ( levels[i] == level )
                    : ( level == CategoryId::None ) );
            actionGroup->addAction( action );
        }

        d->levelGroups[i] = actionGroup;
        connect( menuLevel, &QMenu::triggered, this, &CollectionWidget::sortLevelSelected );
    }

    // -- create the checkboxesh
    filterMenu->addSeparator();
    QAction *showYears = filterMenu->addAction( i18n( "Show Years" ) );
    showYears->setCheckable( true );
    showYears->setChecked( AmarokConfig::showYears() );
    connect( showYears, &QAction::toggled, this, &CollectionWidget::slotShowYears );

    QAction *showTrackNumbers = filterMenu->addAction( i18nc("@action:inmenu", "Show Track Numbers") );
    showTrackNumbers->setCheckable( true );
    showTrackNumbers->setChecked( AmarokConfig::showTrackNumbers() );
    connect( showTrackNumbers, &QAction::toggled, this, &CollectionWidget::slotShowTrackNumbers );

    QAction *showArtistForVarious = filterMenu->addAction( i18nc("@action:inmenu", "Show Artist for Various Artists") );
    showArtistForVarious->setCheckable( true );
    showArtistForVarious->setChecked( AmarokConfig::showArtistForVarious() );
    connect( showArtistForVarious, &QAction::toggled, this, &CollectionWidget::slotShowArtistForVarious );

    QAction *showCovers = filterMenu->addAction( i18n( "Show Cover Art" ) );
    showCovers->setCheckable( true );
    showCovers->setChecked( AmarokConfig::showAlbumArt() );
    connect( showCovers, &QAction::toggled, this, &CollectionWidget::slotShowCovers );

    d->searchWidget->toolBar()->addSeparator();

    QAction *toggleAction = new QAction( QIcon::fromTheme( QStringLiteral("view-list-tree") ), i18n( "Merged View" ), this );
    toggleAction->setCheckable( true );
    toggleAction->setChecked( d->viewMode == CollectionWidget::UnifiedCollection );
    toggleView( d->viewMode == CollectionWidget::UnifiedCollection );
    connect( toggleAction, &QAction::triggered, this, &CollectionWidget::toggleView );
    d->searchWidget->toolBar()->addAction( toggleAction );

    QAction *searchMenuAction = new QAction( QIcon::fromTheme( QStringLiteral("preferences-other") ), i18n( "Sort Options" ), this );
    searchMenuAction->setMenu( filterMenu );
    d->searchWidget->toolBar()->addAction( searchMenuAction );

    QToolButton *tbutton = qobject_cast<QToolButton*>( d->searchWidget->toolBar()->widgetForAction( searchMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

    setLevels( levels );
}

CollectionWidget::~CollectionWidget()
{
    delete d;
}


void
CollectionWidget::focusInputLine()
{
    d->searchWidget->comboBox()->setFocus();
}

void
CollectionWidget::sortLevelSelected( QAction *action )
{
    Q_UNUSED( action );

    QList<CategoryId::CatMenuId> levels;
    for( int i = 0; i < CATEGORY_LEVEL_COUNT; i++ )
    {
        const QAction *action = d->levelGroups[i]->checkedAction();
        if( action )
        {
            CategoryId::CatMenuId category = action->data().value<CategoryId::CatMenuId>();
            if( category != CategoryId::None )
                levels << category;
        }
    }
    setLevels( levels );
}

void
CollectionWidget::sortByActionPayload( QAction *action )
{
    QList<CategoryId::CatMenuId> levels = action->data().value<QList<CategoryId::CatMenuId> >();
    if( !levels.isEmpty() )
        setLevels( levels );
}

void
CollectionWidget::slotShowYears( bool checked )
{
    AmarokConfig::setShowYears( checked );
    setLevels( levels() );
}

void
CollectionWidget::slotShowTrackNumbers( bool checked )
{
    AmarokConfig::setShowTrackNumbers( checked );
    setLevels( levels() );
}

void
CollectionWidget::slotShowArtistForVarious( bool checked )
{
    AmarokConfig::setShowArtistForVarious( checked );
    setLevels( levels() );
}

void
CollectionWidget::slotShowCovers(bool checked)
{
    AmarokConfig::setShowAlbumArt( checked );
    setLevels( levels() );
}

QString
CollectionWidget::filter() const
{
    return d->searchWidget->currentText();
}

void CollectionWidget::setFilter( const QString &filter )
{
    d->searchWidget->setSearchString( filter );
}

QList<CategoryId::CatMenuId>
CollectionWidget::levels() const
{
    // return const_cast<CollectionWidget*>( this )->view( d->viewMode )->levels();
    return d->view( d->viewMode )->levels();
}

void CollectionWidget::setLevels( const QList<CategoryId::CatMenuId> &levels )
{
    // -- select the correct menu entries
    QSet<CategoryId::CatMenuId> encounteredLevels;
    for( int i = 0; i < CATEGORY_LEVEL_COUNT; i++ )
    {
        CategoryId::CatMenuId category;
        if( levels.count() > i )
            category = levels[i];
        else
            category = CategoryId::None;

        for( QAction *action : d->levelGroups[i]->actions() )
        {
            CategoryId::CatMenuId actionCategory = action->data().value<CategoryId::CatMenuId>();
            if( actionCategory == category )
                action->setChecked( true ); // unchecks other actions in the same group
            action->setEnabled( !encounteredLevels.contains( actionCategory ) );
        }

        if( category != CategoryId::None )
            encounteredLevels << category;
    }

    // -- set the levels in the view
    d->view( d->viewMode )->setLevels( levels );
    debug() << "Sort levels:" << levels;
}

void CollectionWidget::toggleView( bool merged )
{
    CollectionWidget::ViewMode newMode = merged ? UnifiedCollection : NormalCollections;
    CollectionBrowserTreeView *oldView = d->view( d->viewMode );

    if( oldView )
    {
        d->searchWidget->disconnect( oldView );
        oldView->disconnect( d->searchWidget );
    }

    CollectionBrowserTreeView *newView = d->view( newMode );
    connect( d->searchWidget, &SearchWidget::filterChanged,
             newView, &CollectionBrowserTreeView::slotSetFilter );
    connect( d->searchWidget, &SearchWidget::returnPressed,
             newView, &CollectionBrowserTreeView::slotAddFilteredTracksToPlaylist );
    // reset search string after successful adding of filtered items to playlist
    connect( newView, &CollectionBrowserTreeView::addingFilteredTracksDone,
             d->searchWidget, &SearchWidget::emptySearchString );

    if( d->stack->indexOf( newView ) == -1 )
        d->stack->addWidget( newView );
    d->stack->setCurrentWidget( newView );
    const QString &filter = d->searchWidget->currentText();
    if( !filter.isEmpty() )
    {
        typedef CollectionTreeItemModelBase CTIMB;
        CTIMB *model = qobject_cast<CTIMB*>( newView->filterModel()->sourceModel() );
        model->setCurrentFilter( filter );
    }

    d->viewMode = newMode;
    if( oldView )
        setLevels( oldView->levels() );

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "ViewMode" ) );
    Amarok::config( QStringLiteral("Collection Browser") ).writeEntry( "View Mode", me.valueToKey( d->viewMode ) );
}

QList<CategoryId::CatMenuId>
CollectionWidget::readLevelsFromConfig() const
{
    QList<int> levelNumbers = Amarok::config( QStringLiteral("Collection Browser") ).readEntry( "TreeCategory", QList<int>() );
    QList<CategoryId::CatMenuId> levels;

    // we changed "Track Artist" to "Album Artist" default before Amarok 2.8. Migrate user
    // config mentioning Track Artist to Album Artist where it makes sense:
    static const int OldArtistValue = 2;
    bool albumOrAlbumArtistEncountered = false;
    for( int levelNumber : levelNumbers )
    {
        CategoryId::CatMenuId category;
        if( levelNumber == OldArtistValue )
        {
            if( albumOrAlbumArtistEncountered )
                category = CategoryId::Artist;
            else
                category = CategoryId::AlbumArtist;
        }
        else
            category = CategoryId::CatMenuId( levelNumber );

        levels << category;
        if( category == CategoryId::Album || category == CategoryId::AlbumArtist )
            albumOrAlbumArtistEncountered = true;
    }

    return levels;
}

CollectionBrowserTreeView*
CollectionWidget::currentView()
{
    return d->view( d->viewMode );
}

CollectionWidget::ViewMode
CollectionWidget::viewMode() const
{
    return d->viewMode;
}

SearchWidget*
CollectionWidget::searchWidget()
{
    return d->searchWidget;
}

