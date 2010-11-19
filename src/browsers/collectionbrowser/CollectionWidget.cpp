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

#define DEBUG_PREFIX "CollectionWidget"

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
#include <KStandardGuiItem>

#include <QActionGroup>
#include <QMetaEnum>
#include <QMetaObject>
#include <QRect>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>

CollectionWidget *CollectionWidget::s_instance = 0;

class CollectionWidget::Private
{
public:
    Private()
        : treeView( 0 )
        , singleTreeView( 0 )
        , viewMode( CollectionWidget::NormalCollections ) {}
    ~Private() {}

    CollectionBrowserTreeView *view( CollectionWidget::ViewMode mode );

    CollectionBrowserTreeView *treeView;
    CollectionBrowserTreeView *singleTreeView;
    QStackedWidget *stack;
    SearchWidget *searchWidget;
    CollectionWidget::ViewMode viewMode;

    QAction *selectedActionLevel[3];
    QMenu *menuLevel[3];
    QList<int> levels;
};

CollectionBrowserTreeView *
CollectionWidget::Private::view( CollectionWidget::ViewMode mode )
{
    CollectionBrowserTreeView *v( 0 );
    switch( mode )
    {
    case CollectionWidget::NormalCollections:
        if( !treeView )
        {
            v = new CollectionBrowserTreeView( stack );
            v->setAlternatingRowColors( true );
            v->setFrameShape( QFrame::NoFrame );
            v->setRootIsDecorated( false );
            connect( v, SIGNAL(leavingTree()), searchWidget->comboBox(), SLOT(setFocus()) );
            CollectionTreeItemDelegate *delegate = new CollectionTreeItemDelegate( v );
            v->setItemDelegate( delegate );
            CollectionTreeItemModelBase *multiModel = new CollectionTreeItemModel( levels );
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
            Collections::ProxyCollection *proxyColl = new Collections::ProxyCollection();
            connect( CollectionManager::instance(),
                     SIGNAL(collectionAdded(Collections::Collection*,CollectionManager::CollectionStatus)),
                     proxyColl,
                     SLOT(addCollection(Collections::Collection*,CollectionManager::CollectionStatus)));
            connect( CollectionManager::instance(), SIGNAL(collectionRemoved(QString)),
                     proxyColl, SLOT(removeCollection(QString)));
            foreach( Collections::Collection* coll, CollectionManager::instance()->viewableCollections() )
            {
                proxyColl->addCollection( coll, CollectionManager::CollectionViewable );
            }
            CollectionTreeItemModelBase *singleModel = new SingleCollectionTreeItemModel( proxyColl, levels );
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
    setMargin( 0 );
    setSpacing( 0 );
    //TODO: we have a really nice opportunity to make these info blurbs both helpful and pretty
    setLongDescription( i18n( "This is where you will find your local music, as well as music from mobile audio players and CDs." ) );
    setImagePath( KStandardDirs::locate( "data", "amarok/images/hover_info_collections.png" ) );

    KHBox *hbox = new KHBox( this );
    d->searchWidget = new SearchWidget( hbox );
    d->searchWidget->setClickMessage( i18n( "Search collection" ) );
    d->stack = new QStackedWidget( this );

    QTimer::singleShot( 0, this, SLOT(init()) );
}

CollectionWidget::~CollectionWidget()
{
    delete d;
}

void
CollectionWidget::init()
{
    DEBUG_BLOCK
    PERF_LOG( "Begin init" );
    d->levels = Amarok::config( "Collection Browser" ).readEntry( "TreeCategory", QList<int>() );
    if ( d->levels.isEmpty() )
        d->levels << CategoryId::Artist << CategoryId::Album;

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "ViewMode" ) );
    const QString &value = Amarok::config( "Collection Browser" ).readEntry( "View Mode" );
    int enumValue = me.keyToValue( value.toLocal8Bit().constData() );
    enumValue == -1 ? d->viewMode = NormalCollections : d->viewMode = (ViewMode) enumValue;

    d->stack->setFrameShape( QFrame::NoFrame );
    d->stack->addWidget( d->view( d->viewMode ) );

    // Filter presets. UserRole is used to store the actual syntax.
    KComboBox *combo = d->searchWidget->comboBox();
    const KIcon icon = KStandardGuiItem::find().icon();
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added This Hour"), QLatin1String("added:<1h" ) );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added Today"), QLatin1String("added:<1d" ) );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added This Week"), QLatin1String("added:<1w") );
    combo->addItem( icon, i18nc("@item:inlistbox Collection widget filter preset", "Added This Month"), QLatin1String("added:<1m") );
    combo->insertSeparator( combo->count() );

    QMenu *filterMenu = new QMenu( 0 );

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

    d->menuLevel[0] = filterMenu->addMenu( i18n( "First Level" ) );

    QAction *firstArtistAction      = d->menuLevel[0]->addAction( i18n( "Artist" ) );
    firstArtistAction->setData( CategoryId::Artist );

    QAction *firstAlbumAction       = d->menuLevel[0]->addAction( i18n( "Album"  ) );
    firstAlbumAction->setData( CategoryId::Album );

    QAction *firstAlbumArtistAction = d->menuLevel[0]->addAction( i18n( "Album Artist"  ) );
    firstAlbumArtistAction->setData( CategoryId::AlbumArtist );

    QAction *firstGenreAction       = d->menuLevel[0]->addAction( i18n( "Genre"  ) );
    firstGenreAction->setData( CategoryId::Genre );

    QAction *firstComposerAction    = d->menuLevel[0]->addAction( i18n( "Composer" ) );
    firstComposerAction->setData( CategoryId::Composer );

    QAction *firstLabelAction       = d->menuLevel[0]->addAction( i18n( "Label" ) );
    firstLabelAction->setData( CategoryId::Label );

    firstArtistAction->setCheckable     ( true );
    firstAlbumAction->setCheckable      ( true );
    firstAlbumArtistAction->setCheckable( true );
    firstGenreAction->setCheckable      ( true );
    firstComposerAction->setCheckable   ( true );
    firstLabelAction->setCheckable      ( true );

    QActionGroup *firstGroup = new QActionGroup( this );
    firstGroup->addAction( firstArtistAction );
    firstGroup->addAction( firstAlbumAction );
    firstGroup->addAction( firstAlbumArtistAction );
    firstGroup->addAction( firstGenreAction );
    firstGroup->addAction( firstComposerAction );
    firstGroup->addAction( firstLabelAction );

    connect( d->menuLevel[0], SIGNAL( triggered( QAction *) ), SLOT( customFilter( QAction * ) ) );

    d->menuLevel[1] = filterMenu->addMenu( i18n( "Second Level" ) );
    QAction *secondNullAction       = d->menuLevel[1]->addAction( i18n( "None" ) );
    secondNullAction->setData( CategoryId::None );

    QAction *secondArtistAction     = d->menuLevel[1]->addAction( i18n( "Artist" ) );
    secondArtistAction->setData( CategoryId::Artist );

    QAction *secondAlbumAction      = d->menuLevel[1]->addAction( i18n( "Album"  ) );
    secondAlbumAction->setData( CategoryId::Album );

    QAction *secondAlbumArtistAction= d->menuLevel[1]->addAction( i18n( "Album Artist"  ) );
    secondAlbumArtistAction->setData( CategoryId::AlbumArtist );

    QAction *secondGenreAction      = d->menuLevel[1]->addAction( i18n( "Genre"  ) );
    secondGenreAction->setData( CategoryId::Genre );

    QAction *secondComposerAction   = d->menuLevel[1]->addAction( i18n( "Composer" ) );
    secondComposerAction->setData( CategoryId::Composer );

    QAction *secondLabelAction      = d->menuLevel[1]->addAction( i18n( "Label" ) );
    secondLabelAction->setData( CategoryId::Label );

    secondNullAction->setCheckable       ( true );
    secondArtistAction->setCheckable     ( true );
    secondAlbumAction->setCheckable      ( true );
    secondAlbumArtistAction->setCheckable( true );
    secondGenreAction->setCheckable      ( true );
    secondComposerAction->setCheckable   ( true );
    secondLabelAction->setCheckable      ( true );

    QActionGroup *secondGroup = new QActionGroup( this );
    secondGroup->addAction( secondNullAction );
    secondGroup->addAction( secondArtistAction );
    secondGroup->addAction( secondAlbumAction );
    secondGroup->addAction( secondAlbumArtistAction );
    secondGroup->addAction( secondGenreAction );
    secondGroup->addAction( secondComposerAction );
    secondGroup->addAction( secondLabelAction );
    secondNullAction->setChecked( true );

    connect( d->menuLevel[1], SIGNAL( triggered( QAction *) ), SLOT( customFilter( QAction * ) ) );

    d->menuLevel[2] = filterMenu->addMenu( i18n( "Third Level" ) );
    QAction *thirdNullAction        = d->menuLevel[2]->addAction( i18n( "None" ) );
    thirdNullAction->setData( CategoryId::None );

    QAction *thirdArtistAction      = d->menuLevel[2]->addAction( i18n( "Artist" ) );
    thirdArtistAction->setData( CategoryId::Artist );

    QAction *thirdAlbumAction       = d->menuLevel[2]->addAction( i18n( "Album"  ) );
    thirdAlbumAction->setData( CategoryId::Album );

    QAction *thirdAlbumArtistAction = d->menuLevel[2]->addAction( i18n( "Album Artist"  ) );
    thirdAlbumArtistAction->setData( CategoryId::AlbumArtist );

    QAction *thirdGenreAction       = d->menuLevel[2]->addAction( i18n( "Genre"  ) );
    thirdGenreAction->setData( CategoryId::Genre );

    QAction *thirdComposerAction    = d->menuLevel[2]->addAction( i18n( "Composer" ) );
    thirdComposerAction->setData( CategoryId::Composer );

    QAction *thirdLabelAction       = d->menuLevel[2]->addAction( i18n( "Label" ) );
    thirdLabelAction->setData( CategoryId::Label );

    thirdNullAction->setCheckable       ( true );
    thirdArtistAction->setCheckable     ( true );
    thirdAlbumAction->setCheckable      ( true );
    thirdAlbumArtistAction->setCheckable( true );
    thirdGenreAction->setCheckable      ( true );
    thirdComposerAction->setCheckable   ( true );
    thirdLabelAction->setCheckable      ( true );

    QActionGroup *thirdGroup = new QActionGroup( this );
    thirdGroup->addAction( thirdNullAction );
    thirdGroup->addAction( thirdArtistAction );
    thirdGroup->addAction( thirdAlbumAction );
    thirdGroup->addAction( thirdAlbumArtistAction );
    thirdGroup->addAction( thirdGenreAction );
    thirdGroup->addAction( thirdComposerAction );
    thirdGroup->addAction( thirdLabelAction );
    thirdNullAction->setChecked( true );

    connect( d->menuLevel[2], SIGNAL( triggered( QAction *) ), SLOT( customFilter( QAction * ) ) );

    filterMenu->addSeparator();
    QAction *showYears = filterMenu->addAction( i18n( "Show Years" ) );
    showYears->setCheckable( true );
    showYears->setChecked( AmarokConfig::showYears() );
    connect( showYears, SIGNAL( toggled( bool ) ), SLOT( slotShowYears( bool ) ) );

    QAction *showTrackNumbers = filterMenu->addAction( i18nc("@action:inmenu", "Show Track Numbers") );
    showTrackNumbers->setCheckable( true );
    showTrackNumbers->setChecked( AmarokConfig::showTrackNumbers() );
    connect( showTrackNumbers, SIGNAL( toggled( bool ) ), SLOT( slotShowTrackNumbers( bool ) ) );

    QAction *showCovers = filterMenu->addAction( i18n( "Show Cover Art" ) );
    showCovers->setCheckable( true );
    showCovers->setChecked( AmarokConfig::showAlbumArt() );
    connect( showCovers, SIGNAL(toggled(bool)), SLOT( slotShowCovers( bool ) ) );

    //do not use d->levels directly here, otherwise
    //takeFirst() removes the first item from the list,
    //but we need the correct d->levels in toggleView()
    QList<int> levelCopy = d->levels;
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
            case CategoryId::AlbumArtist:
                firstAlbumArtistAction->setChecked( true );
                break;
            case CategoryId::Genre:
                firstGenreAction->setChecked( true );
                break;
            case CategoryId::Label:
                firstLabelAction->setChecked( true );
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
            case CategoryId::AlbumArtist:
                secondAlbumArtistAction->setChecked( true );
                break;
            case CategoryId::Genre:
                secondGenreAction->setChecked( true );
                break;
            case CategoryId::Composer:
                secondComposerAction->setChecked( true );
                break;
            case CategoryId::Label:
                secondLabelAction->setChecked( true );
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
            case CategoryId::AlbumArtist:
                thirdAlbumArtistAction->setChecked( true );
                break;
            case CategoryId::Genre:
                thirdGenreAction->setChecked( true );
                break;
            case CategoryId::Composer:
                thirdComposerAction->setChecked( true );
                break;
            case CategoryId::Label:
                thirdLabelAction->setChecked( true );
                break;

            default:
                thirdNullAction->setChecked( true );
        }
    }
    d->selectedActionLevel[0] = firstGroup->checkedAction();
    d->selectedActionLevel[1] = secondGroup->checkedAction();
    d->selectedActionLevel[2] = thirdGroup->checkedAction();
    debug() << "Sort levels:" << d->levels;

    d->searchWidget->toolBar()->addSeparator();

    KAction *toggleAction = new KAction( KIcon( "view-list-tree" ), i18n( "Merged View" ), this );
    toggleAction->setCheckable( true );
    toggleAction->setChecked( d->viewMode == CollectionWidget::UnifiedCollection );
    toggleView( d->viewMode == CollectionWidget::UnifiedCollection );
    connect( toggleAction, SIGNAL( triggered( bool ) ), SLOT( toggleView( bool ) ) );
    d->searchWidget->toolBar()->addAction( toggleAction );

    KAction *searchMenuAction = new KAction( KIcon( "preferences-other" ), i18n( "Sort Options" ), this );
    searchMenuAction->setMenu( filterMenu );
    d->searchWidget->toolBar()->addAction( searchMenuAction );

    QToolButton *tbutton = qobject_cast<QToolButton*>( d->searchWidget->toolBar()->widgetForAction( searchMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );
}

void
CollectionWidget::customFilter( QAction *action )
{
    QMenu *menu = qobject_cast<QMenu*>( sender() );

    if( menu == d->menuLevel[0] )
        d->selectedActionLevel[0] = action;
    else if( menu == d->menuLevel[1] )
        d->selectedActionLevel[1] = action;
    else
        d->selectedActionLevel[2] = action;

    const int firstLevel = d->selectedActionLevel[0]->data().toInt();
    const int secondLevel = d->selectedActionLevel[1]->data().toInt();
    const int thirdLevel = d->selectedActionLevel[2]->data().toInt();
    d->levels.clear();
    d->levels << firstLevel;
    if( secondLevel != CategoryId::None )
        d->levels << secondLevel;
    if( thirdLevel != CategoryId::None )
        d->levels << thirdLevel;
    setLevels( d->levels );
}

void
CollectionWidget::sortByArtistAlbum()
{
    d->levels.clear();
    d->levels << CategoryId::Artist << CategoryId::Album;
    setLevels( d->levels );
}

void
CollectionWidget::sortByGenreArtist()
{
    d->levels.clear();
    d->levels << CategoryId::Genre << CategoryId::Artist;
    setLevels( d->levels );
}

void
CollectionWidget::sortByGenreArtistAlbum()
{
    d->levels.clear();
    d->levels << CategoryId::Genre << CategoryId::Artist << CategoryId::Album;
    setLevels( d->levels );
}

void CollectionWidget::sortByAlbum()
{
    d->levels.clear();
    d->levels << CategoryId::Album;
    setLevels( d->levels );
}

void CollectionWidget::sortByArtist()
{
    d->levels.clear();
    d->levels << CategoryId::Artist;
    setLevels( d->levels );
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
CollectionWidget::slotShowCovers(bool checked)
{
    AmarokConfig::setShowAlbumArt( checked );
    setLevels( levels() );
}


void CollectionWidget::setFilter( const QString &filter )
{
    d->searchWidget->setSearchString( filter );
}

QString
CollectionWidget::filter() const
{
    return d->searchWidget->currentText();
}

QList<int>
CollectionWidget::levels() const
{
    // return const_cast<CollectionWidget*>( this )->view( d->viewMode )->levels();
    return d->view( d->viewMode )->levels();
}

void CollectionWidget::setLevels( const QList<int> &levels )
{
    d->levels = levels;
    d->view( d->viewMode )->setLevels( levels );
    debug() << "Sort levels:" << d->levels;
}

void CollectionWidget::toggleView( bool merged )
{
    if( merged )
    {
        debug() << "Switching to merged model";
        if( d->treeView )
            d->searchWidget->disconnect( d->treeView );
    }
    else
    {
        debug() << "switching to multi model";
        if( d->singleTreeView )
            d->searchWidget->disconnect( d->singleTreeView );
    }

    CollectionBrowserTreeView *treeView = merged ? d->view( UnifiedCollection ) : d->view( NormalCollections );
    d->searchWidget->setup( treeView );
    if( d->stack->indexOf( treeView ) == -1 )
        d->stack->addWidget( treeView );
    d->stack->setCurrentWidget( treeView );
    const QString &filter = d->searchWidget->currentText();
    if( !filter.isEmpty() )
    {
        typedef CollectionTreeItemModelBase CTIMB;
        CTIMB *model = qobject_cast<CTIMB*>( treeView->filterModel()->sourceModel() );
        model->setCurrentFilter( filter );
        treeView->slotFilterNow();
    }
    if( d->levels != treeView->levels() )
        treeView->setLevels( d->levels );
    d->viewMode = merged ? UnifiedCollection : NormalCollections;

    const QMetaObject *mo = metaObject();
    const QMetaEnum me = mo->enumerator( mo->indexOfEnumerator( "ViewMode" ) );
    Amarok::config( "Collection Browser" ).writeEntry( "View Mode", me.valueToKey( d->viewMode ) );
}

#include "CollectionWidget.moc"
