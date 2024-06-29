/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "MatchedTracksPage.h"

#include "App.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Debug.h"
#include "statsyncing/TrackTuple.h"
#include "statsyncing/models/MatchedTracksModel.h"
#include "statsyncing/ui/TrackDelegate.h"

#include <QEvent>
#include <QMenu>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

// needed for QCombobox payloads:
Q_DECLARE_METATYPE( StatSyncing::ProviderPtr )

namespace StatSyncing
{
    class SortFilterProxyModel : public QSortFilterProxyModel
    {
        public:
            SortFilterProxyModel( QObject *parent = nullptr )
                : QSortFilterProxyModel( parent )
                , m_tupleFilter( -1 )
            {
                // filer all columns, accept when at least one column matches:
                setFilterKeyColumn( -1 );
            }

            /**
             * Filter tuples based on their MatchedTracksModel::TupleFlag flag. Set to -1
             * to accept tuples with any flags.
             */
            void setTupleFilter( int filter )
            {
                m_tupleFilter = filter;
                invalidateFilter();
                sort( sortColumn(), sortOrder() ); // this doesn't happen automatically
            }

        protected:
            bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override
            {
                if( source_parent.isValid() )
                    return true; // we match all child items, we filter only root ones
                if( m_tupleFilter != -1 )
                {
                    QModelIndex index = sourceModel()->index( source_row, 0, source_parent );
                    int flags = sourceModel()->data( index, MatchedTracksModel::TupleFlagsRole ).toInt();
                    if( !(flags & m_tupleFilter) )
                        return false;
                }
                return QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );
            }

            bool lessThan( const QModelIndex &left, const QModelIndex &right ) const override
            {
                if( left.parent().isValid() ) // we are comparing childs, special mode:
                {
                    // take providers, e.g. reset column to 0
                    QModelIndex l = sourceModel()->index( left.row(), 0, left.parent() );
                    QModelIndex r = sourceModel()->index( right.row(), 0, right.parent() );
                    QString leftProvider = sourceModel()->data( l, Qt::DisplayRole ).toString();
                    QString rightProvider = sourceModel()->data( r, Qt::DisplayRole ).toString();

                    // make this sorting ignore the sort order, always sort ascendingly:
                    if( sortOrder() == Qt::AscendingOrder )
                        return leftProvider.localeAwareCompare( rightProvider ) < 0;
                    else
                        return leftProvider.localeAwareCompare( rightProvider ) > 0;
                }
                return QSortFilterProxyModel::lessThan( left, right );
            }

        private:
            int m_tupleFilter;
    };
}

using namespace StatSyncing;

MatchedTracksPage::MatchedTracksPage( QWidget *parent, Qt::WindowFlags f )
    : QWidget( parent, f )
    , m_matchedTracksModel( nullptr )
{
    setupUi( this );
    // this group box is only shown upon setTracksToScrobble() call
    scrobblingGroupBox->hide();

    m_matchedProxyModel = new SortFilterProxyModel( this );
    m_uniqueProxyModel = new QSortFilterProxyModel( this );
    m_excludedProxyModel = new QSortFilterProxyModel( this );

#define SETUP_MODEL( proxyModel, name, Name ) \
    proxyModel->setSortLocaleAware( true ); \
    proxyModel->setSortCaseSensitivity( Qt::CaseInsensitive ); \
    proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive ); \
    connect( proxyModel, &QSortFilterProxyModel::modelReset, this, &MatchedTracksPage::refresh##Name##StatusText ); \
    connect( proxyModel, &QSortFilterProxyModel::rowsInserted, this, &MatchedTracksPage::refresh##Name##StatusText ); \
    connect( proxyModel, &QSortFilterProxyModel::rowsRemoved, this, &MatchedTracksPage::refresh##Name##StatusText ); \
    name##TreeView->setModel( m_##name##ProxyModel ); \
    name##TreeView->setItemDelegate( new TrackDelegate( name##TreeView ) ); \
    connect( name##FilterLine, &QLineEdit::textChanged, proxyModel, &QSortFilterProxyModel::setFilterFixedString ); \
    name##TreeView->header()->setStretchLastSection( false ); \
    name##TreeView->header()->setDefaultSectionSize( 80 );

    SETUP_MODEL( m_matchedProxyModel, matched, Matched )
    SETUP_MODEL( m_uniqueProxyModel, unique, Unique )
    SETUP_MODEL( m_excludedProxyModel, excluded, Excluded )
#undef SETUP_MODEL

    connect( uniqueFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &MatchedTracksPage::changeUniqueTracksProvider );
    connect( excludedFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &MatchedTracksPage::changeExcludedTracksProvider );

    QPushButton *configure = buttonBox->addButton( i18n( "Configure Synchronization..." ), QDialogButtonBox::ActionRole );
    connect( configure, &QPushButton::clicked, this, &MatchedTracksPage::openConfiguration );

    QPushButton *back = buttonBox->addButton( i18n( "Back" ), QDialogButtonBox::ActionRole );

    QPushButton *synchronize = buttonBox->addButton( i18n( "Synchronize" ), QDialogButtonBox::AcceptRole );
    synchronize->setIcon( QIcon( QStringLiteral("document-save") ) );

    connect( back, &QAbstractButton::clicked, this, &MatchedTracksPage::back );
    connect( buttonBox, &QDialogButtonBox::accepted, this, &MatchedTracksPage::accepted );
    connect( buttonBox, &QDialogButtonBox::rejected, this, &MatchedTracksPage::rejected );

    tabWidget->setTabEnabled( 1, false );
    tabWidget->setTabToolTip( 1, i18n( "There are no tracks unique to one of the sources "
                                       "participating in the synchronization" ) );
    tabWidget->setTabEnabled( 2, false );
    tabWidget->setTabToolTip( 2, i18n( "There are no tracks excluded from "
                                       "synchronization" ) );

    QMenu *menu = new QMenu( matchedExpandButton );
    menu->addAction( i18n( "Expand Tracks With Conflicts" ), this, &MatchedTracksPage::expand )->setData(
        MatchedTracksModel::HasConflict );
    menu->addAction( i18n( "Expand Updated" ), this, &MatchedTracksPage::expand )->setData(
        MatchedTracksModel::HasUpdate );
    menu->addAction( i18n( "Expand All" ), this, &MatchedTracksPage::expand )->setData( 0 );
    matchedExpandButton->setMenu( menu );

    menu = new QMenu( matchedCollapseButton );
    menu->addAction( i18n( "Collapse Tracks Without Conflicts" ), this, &MatchedTracksPage::collapse )->setData(
        MatchedTracksModel::HasConflict );
    menu->addAction( i18n( "Collapse Not Updated" ), this, &MatchedTracksPage::collapse )->setData(
        MatchedTracksModel::HasUpdate );
    menu->addAction( i18n( "Collapse All" ), this, &MatchedTracksPage::collapse )->setData( 0 );
    matchedCollapseButton->setMenu( menu );
}

MatchedTracksPage::~MatchedTracksPage()
{
}

void
MatchedTracksPage::setProviders( const ProviderPtrList &providers )
{
    // populate menu of the "Take Ratings From" button
    QMenu *takeRatingsMenu = new QMenu( matchedRatingsButton );
    for( const ProviderPtr &provider : providers )
    {
        QAction *action = takeRatingsMenu->addAction( provider->icon(), provider->prettyName(),
                                                      this, &MatchedTracksPage::takeRatingsFrom );
        action->setData( QVariant::fromValue<ProviderPtr>( provider ) );
    }
    takeRatingsMenu->addAction( i18n( "Reset All Ratings to Undecided" ), this, &MatchedTracksPage::takeRatingsFrom );
    matchedRatingsButton->setMenu( takeRatingsMenu );
    matchedRatingsButton->setIcon( QIcon::fromTheme( Meta::iconForField( Meta::valRating ) ) );

    // populate menu of the "Labels" button
    QMenu *labelsMenu = new QMenu( matchedLabelsButton );
    for( const ProviderPtr &provider : providers )
    {
        QString text = i18nc( "%1 is collection name", "Include Labels from %1", provider->prettyName() );
        QAction *action = labelsMenu->addAction( provider->icon(), text, this, &MatchedTracksPage::includeLabelsFrom );
        action->setData( QVariant::fromValue<ProviderPtr>( provider ) );

        text = i18nc( "%1 is collection name", "Exclude Labels from %1", provider->prettyName() );
        action = labelsMenu->addAction( provider->icon(), text, this, &MatchedTracksPage::excludeLabelsFrom );
        action->setData( QVariant::fromValue<ProviderPtr>( provider ) );
    }
    labelsMenu->addAction( i18n( "Reset All Labels to Undecided (Don't Synchronize Them)" ),
                           this, &MatchedTracksPage::excludeLabelsFrom );
    matchedLabelsButton->setMenu( labelsMenu );
    matchedLabelsButton->setIcon( QIcon::fromTheme( Meta::iconForField( Meta::valLabel ) ) );
}

void
MatchedTracksPage::setMatchedTracksModel( MatchedTracksModel *model )
{
    m_matchedTracksModel = model;
    Q_ASSERT( m_matchedTracksModel );
    m_matchedProxyModel->setSourceModel( m_matchedTracksModel );

    setHeaderSizePoliciesFromModel( matchedTreeView->header(), m_matchedTracksModel );
    m_matchedProxyModel->sort( 0, Qt::AscendingOrder );
    // initially, expand tuples with conflicts:
    expand( MatchedTracksModel::HasConflict );

    connect( m_matchedProxyModel, &StatSyncing::SortFilterProxyModel::rowsAboutToBeRemoved,
             this, &MatchedTracksPage::rememberExpandedState );
    connect( m_matchedProxyModel, &StatSyncing::SortFilterProxyModel::rowsInserted,
             this, &MatchedTracksPage::restoreExpandedState );

    // re-fill combo box and disable choices without tracks
    bool hasConflict = m_matchedTracksModel->hasConflict();
    matchedFilterCombo->clear();
    matchedFilterCombo->addItem( i18n( "All Tracks" ), -1 );
    matchedFilterCombo->addItem( i18n( "Updated Tracks" ), int( MatchedTracksModel::HasUpdate ) );
    matchedFilterCombo->addItem( i18n( "Tracks With Conflicts" ), int( MatchedTracksModel::HasConflict ) );
    QStandardItemModel *comboModel = dynamic_cast<QStandardItemModel *>( matchedFilterCombo->model() );
    int bestIndex = 0;
    if( comboModel )
    {
        bestIndex = 2;
        if( !hasConflict )
        {
            comboModel->item( 2 )->setFlags( Qt::NoItemFlags );
            matchedFilterCombo->setItemData( 2, i18n( "There are no tracks with conflicts" ),
                                        Qt::ToolTipRole );
            bestIndex = 1;
            if( !m_matchedTracksModel->hasUpdate() )
            {
                comboModel->item( 1 )->setFlags( Qt::NoItemFlags );
                matchedFilterCombo->setItemData( 1, i18n( "There are no tracks going to be "
                                            "updated" ), Qt::ToolTipRole );
                bestIndex = 0; // no other possibility
            }
        }
    }

    matchedFilterCombo->setCurrentIndex( bestIndex );
    changeMatchedTracksFilter( bestIndex );
    connect( matchedFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &MatchedTracksPage::changeMatchedTracksFilter );

    matchedRatingsButton->setEnabled( hasConflict );
    matchedLabelsButton->setEnabled( hasConflict );
}

void
MatchedTracksPage::addUniqueTracksModel( ProviderPtr provider, QAbstractItemModel *model )
{
    bool first = m_uniqueTracksModels.isEmpty();
    m_uniqueTracksModels.insert( provider, model );
    uniqueFilterCombo->addItem( provider->icon(), provider->prettyName(),
                                QVariant::fromValue<ProviderPtr>( provider ) );

    if( first )
    {
        tabWidget->setTabEnabled( 1, true );
        tabWidget->setTabToolTip( 1, i18n( "Tracks that are unique to their sources" ) );
        setHeaderSizePoliciesFromModel( uniqueTreeView->header(), model );
        uniqueFilterCombo->setCurrentIndex( 0 );
        m_uniqueProxyModel->sort( 0, Qt::AscendingOrder );
    }
}

void
MatchedTracksPage::addExcludedTracksModel( ProviderPtr provider, QAbstractItemModel *model )
{
    bool first = m_excludedTracksModels.isEmpty();
    m_excludedTracksModels.insert( provider, model );
    excludedFilterCombo->addItem( provider->icon(), provider->prettyName(),
                                  QVariant::fromValue<ProviderPtr>( provider ) );

    if( first )
    {
        tabWidget->setTabEnabled( 2, true );
        tabWidget->setTabToolTip( 2, i18n( "Tracks that have been excluded from "
                                           "synchronization due to ambiguity" ) );
        setHeaderSizePoliciesFromModel( excludedTreeView->header(), model );
        excludedFilterCombo->setCurrentIndex( 0 );
        m_excludedProxyModel->sort( 0, Qt::AscendingOrder );
    }
}

void
MatchedTracksPage::setTracksToScrobble( const TrackList &tracksToScrobble,
                                        const QList<ScrobblingServicePtr> &services )
{
    int tracks = tracksToScrobble.count();
    int plays = 0;
    for( const TrackPtr &track : tracksToScrobble )
    {
        plays += track->recentPlayCount();
    }
    QStringList serviceNames;
    for( const ScrobblingServicePtr &service : services )
    {
        serviceNames << QStringLiteral("<b>") + service->prettyName() + QStringLiteral("</b>");
    }

    if( plays )
    {
        QString playsText = i18np( "<b>One</b> play", "<b>%1</b> plays", plays );
        QString text = i18ncp( "%2 is the 'X plays message above'",
                "%2 of <b>one</b> track will be scrobbled to %3.",
                "%2 of <b>%1</b> tracks will be scrobbled to %3.", tracks, playsText,
                serviceNames.join( i18nc( "comma between list words", ", " ) ) );
        scrobblingLabel->setText( text );
        scrobblingGroupBox->show();
    }
    else
        scrobblingGroupBox->hide();
}

void
MatchedTracksPage::changeMatchedTracksFilter( int index )
{
    int filter = matchedFilterCombo->itemData( index ).toInt();
    m_matchedProxyModel->setTupleFilter( filter );
}

void
MatchedTracksPage::changeUniqueTracksProvider( int index )
{
    ProviderPtr provider = uniqueFilterCombo->itemData( index ).value<ProviderPtr>();
    m_uniqueProxyModel->setSourceModel( m_uniqueTracksModels.value( provider ) );
    // trigger re-sort, Qt doesn't do that automatically apparently
    m_uniqueProxyModel->sort( m_uniqueProxyModel->sortColumn(), m_uniqueProxyModel->sortOrder() );
}

void
MatchedTracksPage::changeExcludedTracksProvider( int index )
{
    ProviderPtr provider = excludedFilterCombo->itemData( index ).value<ProviderPtr>();
    m_excludedProxyModel->setSourceModel( m_excludedTracksModels.value( provider ) );
    // trigger re-sort, Qt doesn't do that automatically apparently
    m_excludedProxyModel->sort( m_excludedProxyModel->sortColumn(), m_excludedProxyModel->sortOrder() );
}

void
MatchedTracksPage::refreshMatchedStatusText()
{
    refreshStatusTextHelper( m_matchedProxyModel, matchedStatusBar );
}

void
MatchedTracksPage::refreshUniqueStatusText()
{
    refreshStatusTextHelper( m_uniqueProxyModel, uniqueStatusBar );
}

void
MatchedTracksPage::refreshExcludedStatusText()
{
    refreshStatusTextHelper( m_excludedProxyModel, excludedStatusBar );
}

void
MatchedTracksPage::refreshStatusTextHelper( QSortFilterProxyModel *topModel , QLabel *label )
{
    int bottomModelRows = topModel->sourceModel() ?
        topModel->sourceModel()->rowCount() : 0;
    int topModelRows = topModel->rowCount();

    QString bottomText = i18np( "%1 track", "%1 tracks", bottomModelRows );
    if( topModelRows == bottomModelRows )
        label->setText( bottomText );
    else
    {
        QString text = i18nc( "%2 is the above '%1 track(s)' message", "Showing %1 out "
            "of %2", topModelRows, bottomText );
        label->setText( text );
    }
}

void
MatchedTracksPage::rememberExpandedState( const QModelIndex &parent, int start, int end )
{
    if( parent.isValid() )
        return;
    for( int topModelRow = start; topModelRow <= end; topModelRow++ )
    {
        QModelIndex topModelIndex = m_matchedProxyModel->index( topModelRow, 0 );
        int bottomModelRow = m_matchedProxyModel->mapToSource( topModelIndex ).row();
        if( matchedTreeView->isExpanded( topModelIndex ) )
            m_expandedTuples.insert( bottomModelRow );
        else
            m_expandedTuples.remove( bottomModelRow );
    }
}

void
MatchedTracksPage::restoreExpandedState( const QModelIndex &parent, int start, int end )
{
    if( parent.isValid() )
        return;
    for( int topModelRow = start; topModelRow <= end; topModelRow++ )
    {
        QModelIndex topIndex = m_matchedProxyModel->index( topModelRow, 0 );
        int bottomModelRow = m_matchedProxyModel->mapToSource( topIndex ).row();
        if( m_expandedTuples.contains( bottomModelRow ) )
            matchedTreeView->expand( topIndex );
    }
}

void
MatchedTracksPage::takeRatingsFrom()
{
    QAction *action = qobject_cast<QAction *>( sender() );
    if( !action )
    {
        warning() << __PRETTY_FUNCTION__ << "must only be called from QAction";
        return;
    }

    // provider may be null, it means "reset all ratings to undecided"
    ProviderPtr provider = action->data().value<ProviderPtr>();
    m_matchedTracksModel->takeRatingsFrom( provider );
}

void
MatchedTracksPage::includeLabelsFrom()
{
    QAction *action = qobject_cast<QAction *>( sender() );
    if( !action )
    {
        warning() << __PRETTY_FUNCTION__ << "must only be called from QAction";
        return;
    }

    ProviderPtr provider = action->data().value<ProviderPtr>();
    if( provider ) // no sense with null provider
        m_matchedTracksModel->includeLabelsFrom( provider );
}

void
MatchedTracksPage::excludeLabelsFrom()
{
    QAction *action = qobject_cast<QAction *>( sender() );
    if( !action )
    {
        warning() << __PRETTY_FUNCTION__ << "must only be called from QAction";
        return;
    }

    // provider may be null, it means "reset all labels to undecided"
    ProviderPtr provider = action->data().value<ProviderPtr>();
    m_matchedTracksModel->excludeLabelsFrom( provider );
}

void
MatchedTracksPage::expand( int onlyWithTupleFlags )
{
    if( onlyWithTupleFlags < 0 )
    {
        QAction *action = qobject_cast<QAction *>( sender() );
        if( action )
            onlyWithTupleFlags = action->data().toInt();
        else
            onlyWithTupleFlags = 0;
    }

    for( int i = 0; i < m_matchedProxyModel->rowCount(); i++ )
    {
        QModelIndex idx = m_matchedProxyModel->index( i, 0 );
        if( matchedTreeView->isExpanded( idx ) )
            continue;

        int flags = idx.data( MatchedTracksModel::TupleFlagsRole ).toInt();
        if( ( flags & onlyWithTupleFlags ) == onlyWithTupleFlags )
            matchedTreeView->expand( idx );
    }
}

void
MatchedTracksPage::collapse()
{
    int excludingFlags;
    QAction *action = qobject_cast<QAction *>( sender() );
    if( action )
        excludingFlags = action->data().toInt();
    else
        excludingFlags = 0;

    for( int i = 0; i < m_matchedProxyModel->rowCount(); i++ )
    {
        QModelIndex idx = m_matchedProxyModel->index( i, 0 );
        if( !matchedTreeView->isExpanded( idx ) )
            continue;

        int flags = idx.data( MatchedTracksModel::TupleFlagsRole ).toInt();
        if( ( flags & excludingFlags ) == 0 )
            matchedTreeView->collapse( idx );
    }
}

void
MatchedTracksPage::openConfiguration()
{
    App *app = pApp;
    if( app )
        app->slotConfigAmarok( QStringLiteral("MetadataConfig") );
}

void
MatchedTracksPage::setHeaderSizePoliciesFromModel( QHeaderView *header, QAbstractItemModel *model )
{
    for( int column = 0; column < model->columnCount(); column++ )
    {
        QVariant headerData = model->headerData( column, Qt::Horizontal,
                                                 CommonModel::ResizeModeRole );
        QHeaderView::ResizeMode mode = QHeaderView::ResizeMode( headerData.toInt() );
        header->setSectionResizeMode( column, mode );
    }
}
