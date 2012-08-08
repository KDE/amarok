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

#include <KStandardGuiItem>
#include <KPushButton>

#include <QEvent>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

// needed for QCombobox payloads:
Q_DECLARE_METATYPE( StatSyncing::ProviderPtr )

namespace StatSyncing
{
    class SortFilterProxyModel : public QSortFilterProxyModel
    {
        public:
            SortFilterProxyModel( QObject *parent = 0 )
                : QSortFilterProxyModel( parent )
                , m_tupleFilter( -1 )
            {}

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
            bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
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

            bool lessThan( const QModelIndex &left, const QModelIndex &right ) const
            {
                if( left.parent().isValid() ) // we are comparing childs, special mode:
                {
                    // take providers, e.g. reset column to 0
                    QModelIndex l = sourceModel()->index( left.row(), 0, left.parent() );
                    QModelIndex r = sourceModel()->index( right.row(), 0, right.parent() );
                    QString leftProvider = sourceModel()->data( l, Qt::DisplayRole ).toString();
                    QString rightProvider = sourceModel()->data( r, Qt::DisplayRole ).toString();

                    // make this sorting ignore the sort order, always sort acsendingly:
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
    , m_polished( false )
    , m_matchedTracksComboLastIndex( 2 ) // tracks with conflict
    , m_proxyModel( 0 )
    , m_matchedTracksModel( 0 )
{
    setupUi( this );
    m_proxyModel = new SortFilterProxyModel( this );
    m_proxyModel->setSortLocaleAware( true );
    m_proxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    connect( m_proxyModel, SIGNAL(modelReset()), SLOT(refreshStatusText()) );
    connect( m_proxyModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(refreshStatusText()) );
    connect( m_proxyModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(refreshStatusText()) );
    treeView->setModel( m_proxyModel );
    treeView->setItemDelegate( new TrackDelegate( treeView ) );

    connect( matchedRadio, SIGNAL(toggled(bool)), SLOT(showMatchedTracks(bool)) );
    connect( uniqueRadio, SIGNAL(toggled(bool)), SLOT(showUniqueTracks(bool)) );
    connect( excludedRadio, SIGNAL(toggled(bool)), SLOT(showExcludedTracks(bool)) );
    connect( filterLine, SIGNAL(textChanged(QString)),
             m_proxyModel, SLOT(setFilterFixedString(QString)) );

    KGuiItem configure = KStandardGuiItem::configure();
    configure.setText( i18n( "Configure Automatic Synchronization..." ) );
    buttonBox->addButton( configure, QDialogButtonBox::ActionRole, this, SLOT(openConfiguration()) );
    KPushButton *back = buttonBox->addButton( KStandardGuiItem::back(),
                                              QDialogButtonBox::ActionRole );
    buttonBox->addButton( KGuiItem( i18n( "Synchronize" ), "document-save" ),
                          QDialogButtonBox::AcceptRole );
    connect( back, SIGNAL(clicked(bool)), SIGNAL(back()) );
    connect( buttonBox, SIGNAL(accepted()), SIGNAL(accepted()) );
    connect( buttonBox, SIGNAL(rejected()), SIGNAL(rejected()) );

    QHeaderView *header = treeView->header();
    header->setStretchLastSection( false );
    header->setDefaultSectionSize( 80 );
}

MatchedTracksPage::~MatchedTracksPage()
{
}

void
MatchedTracksPage::setProviders( const ProviderPtrList &providers )
{
    m_providers = providers;
}

void
MatchedTracksPage::setMatchedTracksModel( MatchedTracksModel *model )
{
    m_matchedTracksModel = model;
}

void
MatchedTracksPage::addUniqueTracksModel( ProviderPtr provider, QAbstractItemModel *model )
{
    m_uniqueTracksModels.insert( provider, model );
}

void
MatchedTracksPage::addExcludedTracksModel( ProviderPtr provider, QAbstractItemModel *model )
{
    m_excludedTracksModels.insert( provider, model );
}

void MatchedTracksPage::showEvent( QShowEvent *event )
{
    if( !m_polished )
        polish();
    QWidget::showEvent( event );
}

void
MatchedTracksPage::showMatchedTracks( bool checked )
{
    if( checked )
    {
        m_proxyModel->setSourceModel( m_matchedTracksModel );
        restoreExpandedTuples();
        connect( m_proxyModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                 SLOT(rememberExpandedState(QModelIndex,int,int)) );
        connect( m_proxyModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                 SLOT(restoreExpandedState(QModelIndex,int,int)) );

        // re-fill combo box and disable choices without tracks
        bool hasConflict = m_matchedTracksModel->hasConflict();
        filterCombo->clear();
        filterCombo->addItem( i18n( "All Tracks" ), -1 );
        filterCombo->addItem( i18n( "Updated Tracks" ), int( MatchedTracksModel::HasUpdate ) );
        filterCombo->addItem( i18n( "Tracks With Conflicts" ), int( MatchedTracksModel::HasConflict ) );
        QStandardItemModel *comboModel = dynamic_cast<QStandardItemModel *>( filterCombo->model() );
        if( comboModel )
        {
            if( !hasConflict )
            {
                comboModel->item( 2 )->setFlags( Qt::NoItemFlags );
                filterCombo->setItemData( 2, i18n( "There are no tracks with conflicts" ),
                                          Qt::ToolTipRole );
                m_matchedTracksComboLastIndex = qBound( 0, m_matchedTracksComboLastIndex, 1 );
                if( !m_matchedTracksModel->hasUpdate() )
                {
                    comboModel->item( 1 )->setFlags( Qt::NoItemFlags );
                    filterCombo->setItemData( 1, i18n( "There are no tracks going to be "
                                              "updated" ), Qt::ToolTipRole );
                    m_matchedTracksComboLastIndex = 0; // no other possibility
                }
            }
        }

        filterCombo->setCurrentIndex( m_matchedTracksComboLastIndex );
        changeMatchedTracksFilter( m_matchedTracksComboLastIndex );
        connect( filterCombo, SIGNAL(currentIndexChanged(int)), SLOT(changeMatchedTracksFilter(int)) );

        takeRatingsButton->setEnabled( hasConflict );
    }
    else
    {
        disconnect( filterCombo, 0, this, SLOT(changeMatchedTracksFilter(int)) );
        disconnect( m_proxyModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                    this, SLOT(rememberExpandedState(QModelIndex,int,int)) );
        disconnect( m_proxyModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                    this, SLOT(restoreExpandedState(QModelIndex,int,int)) );
        saveExpandedTuples();
        m_proxyModel->setTupleFilter( -1 ); // reset filter for single tracks models
        takeRatingsButton->setEnabled( false );
    }
}

void
MatchedTracksPage::showUniqueTracks( bool checked )
{
    if( checked )
    {
        showSingleTracks( m_uniqueTracksModels );
        connect( filterCombo, SIGNAL(currentIndexChanged(int)),
                 SLOT(changeUniqueTracksProvider(int)) );
    }
    else
        disconnect( filterCombo, 0, this, SLOT(changeUniqueTracksProvider(int)) );
}

void
MatchedTracksPage::showExcludedTracks( bool checked )
{
    if( checked )
    {
        showSingleTracks( m_excludedTracksModels );
        connect( filterCombo, SIGNAL(currentIndexChanged(int)),
                 SLOT(changeExcludedTracksProvider(int)) );
    }
    else
        disconnect( filterCombo, 0, this, SLOT(changeExcludedTracksProvider(int)) );
}

void
MatchedTracksPage::showSingleTracks( const QMap<ProviderPtr, QAbstractItemModel *> &models )
{
    ProviderPtr lastProvider =
        filterCombo->itemData( filterCombo->currentIndex() ).value<ProviderPtr>();
    filterCombo->clear();
    int currentIndex = 0;
    int i = 0;
    foreach( ProviderPtr provider, models.keys() )
    {
        if( provider == lastProvider )
            currentIndex = i;
        filterCombo->insertItem( i++, provider->icon(), provider->prettyName(),
                           QVariant::fromValue<ProviderPtr>( provider ) );
    }
    filterCombo->setCurrentIndex( currentIndex );
    changeSingleTracksProvider( currentIndex, models );
}

void
MatchedTracksPage::changeMatchedTracksFilter( int index )
{
    m_matchedTracksComboLastIndex = index;
    int filter = filterCombo->itemData( index ).toInt();
    m_proxyModel->setTupleFilter( filter );
}

void
MatchedTracksPage::changeUniqueTracksProvider( int index )
{
    changeSingleTracksProvider( index, m_uniqueTracksModels );
}

void
MatchedTracksPage::changeExcludedTracksProvider( int index )
{
    changeSingleTracksProvider( index, m_excludedTracksModels );
}

void
MatchedTracksPage::changeSingleTracksProvider( int index,
    const QMap<ProviderPtr, QAbstractItemModel *> &models )
{
    ProviderPtr provider = filterCombo->itemData( index ).value<ProviderPtr>();
    m_proxyModel->setSourceModel( models.value( provider ) );
}

void
MatchedTracksPage::refreshStatusText()
{
    int bottomModelRows = m_proxyModel->sourceModel() ?
        m_proxyModel->sourceModel()->rowCount() : 0;
    int topModelRows = m_proxyModel->rowCount();

    QString bottomText = i18np( "%1 track", "%1 tracks", bottomModelRows );
    if( topModelRows == bottomModelRows )
        statusBar->setText( bottomText );
    else
    {
        QString text = i18nc( "%2 is the above '%1 track(s)' message", "Showing %1 out "
            "of %2", topModelRows, bottomText );
        statusBar->setText( text );
    }
}

void
MatchedTracksPage::rememberExpandedState( const QModelIndex &parent, int start, int end )
{
    if( parent.isValid() )
        return;
    for( int topModelRow = start; topModelRow <= end; topModelRow++ )
    {
        QModelIndex topModelIndex = m_proxyModel->index( topModelRow, 0 );
        int bottomModelRow = m_proxyModel->mapToSource( topModelIndex ).row();
        if( treeView->isExpanded( topModelIndex ) )
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
        QModelIndex topIndex = m_proxyModel->index( topModelRow, 0 );
        int bottomModelRow = m_proxyModel->mapToSource( topIndex ).row();
        if( m_expandedTuples.contains( bottomModelRow ) )
            treeView->expand( topIndex );
    }
}

void
MatchedTracksPage::takeRatingsFrom()
{
    QAction *action = dynamic_cast<QAction *>( sender() );
    if( !action )
    {
        warning() << __PRETTY_FUNCTION__ << "must only be called from QAction";
        return;
    }

    ProviderPtr provider = action->data().value<ProviderPtr>();
    m_matchedTracksModel->takeRatingsFrom( provider );
}

void
MatchedTracksPage::openConfiguration()
{
    App *app = App::instance();
    if( app )
        app->slotConfigAmarok( "MetadataConfig" );
}

void
MatchedTracksPage::polish()
{
    Q_ASSERT( m_matchedTracksModel );
    // initially, expand tuples with conflicts:
    for( int i = 0; i < m_matchedTracksModel->rowCount(); i++ )
    {
        if( m_matchedTracksModel->hasConflict( i ) )
            m_expandedTuples.insert( i );
    }
    if( m_uniqueTracksModels.isEmpty() )
    {
        uniqueRadio->setEnabled( false );
        uniqueRadio->setToolTip( i18n( "There are no tracks unique to one of the sources "
            "participating in the synchronization" ) );
    }
    if( m_excludedTracksModels.isEmpty() )
    {
        excludedRadio->setEnabled( false );
        excludedRadio->setToolTip( i18n( "There are no tracks excluded from "
            "synchronization" ) );
    }

    // populate menu of the "Take Ratings From" button
    QMenu *takeRatingsMenu = new QMenu( takeRatingsButton );
    foreach( ProviderPtr provider, m_providers )
    {
        QAction *action = takeRatingsMenu->addAction( provider->icon(), provider->prettyName() );
        action->setData( QVariant::fromValue<ProviderPtr>( provider ) );
        connect( action, SIGNAL(triggered(bool)), SLOT(takeRatingsFrom()) );
    }
    takeRatingsMenu->addAction( i18n( "Reset All Ratings to Undecided" ), this, SLOT(takeRatingsFrom()) );
    takeRatingsButton->setMenu( takeRatingsMenu );
    takeRatingsButton->setIcon( KIcon( Meta::iconForField( Meta::valRating ) ) );

    matchedRadio->setChecked( true ); // calls showMatchedTracks() that sets the model
    QHeaderView *header = treeView->header();
    for( int column = 0; column < m_matchedTracksModel->columnCount(); column++ )
    {
        QVariant headerData = m_matchedTracksModel->headerData( column, Qt::Horizontal,
                                                                CommonModel::ResizeModeRole );
        QHeaderView::ResizeMode mode = QHeaderView::ResizeMode( headerData.toInt() );
        header->setResizeMode( column, mode );
    }
    m_proxyModel->sort( 0, Qt::AscendingOrder );

    m_polished = true;
}

void
MatchedTracksPage::saveExpandedTuples()
{
    rememberExpandedState( QModelIndex(), 0, m_proxyModel->rowCount() - 1 );
}

void
MatchedTracksPage::restoreExpandedTuples()
{
    foreach( int bottomModelRow, m_expandedTuples )
    {
        QModelIndex bottomIndex = m_matchedTracksModel->index( bottomModelRow, 0 );
        QModelIndex topIndex = m_proxyModel->mapFromSource( bottomIndex );
        if( topIndex.isValid() )
            treeView->expand( topIndex );
    }
}
