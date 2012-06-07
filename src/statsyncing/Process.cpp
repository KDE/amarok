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

#include "Process.h"

#include "MetaValues.h"
#include "ui_MatchedTracksPage.h"
#include "core/interfaces/Logger.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "statsyncing/Process.h"
#include "statsyncing/jobs/MatchTracksJob.h"
#include "statsyncing/models/MatchedTracksModel.h"
#include "statsyncing/models/SingleTracksModel.h"

#include <ThreadWeaver/Weaver>

namespace StatSyncing
{
    class MatchedTracksPage : public QWidget, public Ui_MatchedTracksPage
    {
        public:
            explicit MatchedTracksPage( QWidget *parent = 0 )
                : QWidget( parent )
            {
                setupUi( this );
            }
    };
}

using namespace StatSyncing;

Process::Process( const QList<QSharedPointer<Provider> > &providers, QObject *parent )
    : QObject( parent )
    , m_providers( providers )
    , m_matchedTracksPage( 0 )
    , m_matchedTracksModel( 0 )
{
    DEBUG_BLOCK
    Q_ASSERT( m_providers.count() > 0 );
    m_options.setSyncedFields( Meta::valRating | Meta::valFirstPlayed |
        Meta::valLastPlayed | Meta::valPlaycount | Meta::valLabel );
}

Process::~Process()
{
    DEBUG_BLOCK
}

void
Process::start()
{
    MatchTracksJob *job = new MatchTracksJob( m_providers );
    QString text = i18n( "Matching tracks..." );
    Amarok::Components::logger()->newProgressOperation( job, text, 100, job, SLOT(abort()) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(slotTracksMatched(ThreadWeaver::Job*)) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

void
Process::raise()
{
    if( !m_matchedTracksPage )
        return;
    m_matchedTracksPage->activateWindow();
    m_matchedTracksPage->raise();
}

void
Process::slotTracksMatched( ThreadWeaver::Job *job )
{
    DEBUG_BLOCK
    // won't be needed after this method returns; needs to be before early-returns to
    // prevent memory and refcounting leak
    job->deleteLater();
    MatchTracksJob *matchJob = dynamic_cast<MatchTracksJob *>( job );
    if( !matchJob )
    {
        error() << __PRETTY_FUNCTION__ << "Failed cast, should never happen";
        deleteLater();
        return;
    }
    if( !matchJob->success() )
    {
        warning() << __PRETTY_FUNCTION__ << "MatchTracksJob failed";
        deleteLater();
        return;
    }

    QList<qint64> columns = QList<qint64>() << Meta::valTitle << Meta::valRating <<
        Meta::valFirstPlayed << Meta::valLastPlayed << Meta::valPlaycount << Meta::valLabel;
    m_matchedTracksModel = new MatchedTracksModel( matchJob->matchedTuples(), columns, m_options, this );
    foreach( QSharedPointer<Provider> provider, m_providers )
    {
        if( !matchJob->uniqueTracks().value( provider.data() ).isEmpty() )
            m_uniqueTracksModels[ provider.data() ] = new SingleTracksModel(
                matchJob->uniqueTracks().value( provider.data() ), columns, this );
        if( !matchJob->excludedTracks().value( provider.data() ).isEmpty() )
            m_excludedTracksModels[ provider.data() ] = new SingleTracksModel(
                matchJob->excludedTracks().value( provider.data() ), columns, this );
    }

    m_matchedTracksPage = new MatchedTracksPage();
    m_matchedTracksPage->setAttribute( Qt::WA_DeleteOnClose );;
    connect( m_matchedTracksPage->matchedRadio, SIGNAL(toggled(bool)), SLOT(showMatchedTracks(bool)) );
    connect( m_matchedTracksPage->uniqueRadio, SIGNAL(toggled(bool)), SLOT(showUniqueTracks(bool)) );
    if( m_uniqueTracksModels.isEmpty() )
    {
        m_matchedTracksPage->uniqueRadio->setEnabled( false );
        m_matchedTracksPage->uniqueRadio->setToolTip( "There are no tracks unique to one "
            "of the sources participating in the synchronization" );
    }
    connect( m_matchedTracksPage->excludedRadio, SIGNAL(toggled(bool)), SLOT(showExcludedTracks(bool)) );
    if( m_excludedTracksModels.isEmpty() )
    {
        m_matchedTracksPage->excludedRadio->setEnabled( false );
        m_matchedTracksPage->excludedRadio->setToolTip( "There are no tracks excluded "
            "from synchronization" );
    }
    m_matchedTracksPage->matchedRadio->setChecked( true ); // calls showMatchedTracks()

    QHeaderView *header = m_matchedTracksPage->treeView->header();
    header->setStretchLastSection( false );
    header->setDefaultSectionSize( 80 );
    header->setResizeMode( 0, QHeaderView::Stretch );
    header->setResizeMode( 1, QHeaderView::ResizeToContents );
    header->setResizeMode( 2, QHeaderView::ResizeToContents );
    header->setResizeMode( 3, QHeaderView::ResizeToContents );
    header->setResizeMode( 4, QHeaderView::ResizeToContents );
    header->setResizeMode( 5, QHeaderView::Interactive );

    m_matchedTracksPage->show();
    m_matchedTracksPage->raise();
    connect( m_matchedTracksPage, SIGNAL(destroyed(QObject*)), SLOT(deleteLater()) );
}

void
Process::showMatchedTracks( bool really )
{
    if( !really )
        return;
    m_matchedTracksPage->treeView->setModel( m_matchedTracksModel );
    QComboBox *combo = m_matchedTracksPage->filterCombo;
    combo->clear();
    combo->addItem( i18n( "All tracks (TODO)" ) );
    combo->addItem( i18n( "Updates (TODO)" ) );
    combo->addItem( i18n( "Untouched tracks (TODO)" ) );
}

void
Process::showUniqueTracks( bool checked )
{
    QComboBox *combo = m_matchedTracksPage->filterCombo;
    if( checked )
    {
        showSingleTracks( m_uniqueTracksModels );
        connect( combo, SIGNAL(currentIndexChanged(int)), SLOT(changeUniqueTracksProvider(int)) );
    }
    else
        disconnect( combo, 0, this, SLOT(changeUniqueTracksProvider(int)) );
}

void
Process::showExcludedTracks( bool checked )
{
    QComboBox *combo = m_matchedTracksPage->filterCombo;
    if( checked )
    {
        showSingleTracks( m_excludedTracksModels );
        connect( combo, SIGNAL(currentIndexChanged(int)), SLOT(changeExcludedTracksProvider(int)) );
    }
    else
        disconnect( combo, 0, this, SLOT(changeExcludedTracksProvider(int)) );
}

void
Process::showSingleTracks( const QMap<const Provider *, QAbstractItemModel *> &models )
{
    QComboBox *combo = m_matchedTracksPage->filterCombo;
    const Provider *lastProvider =
        combo->itemData( combo->currentIndex() ).value<const Provider *>();
    combo->clear();
    int currentIndex = 0;
    int i = 0;
    foreach( const Provider *provider, models.keys() )
    {
        if( provider == lastProvider )
            currentIndex = i;
        combo->insertItem( i++, provider->icon(), provider->prettyName(),
                           QVariant::fromValue<const Provider *>( provider ) );
    }
    combo->setCurrentIndex( currentIndex );
    changeSingleTracksProvider( currentIndex, models );
}

void
Process::changeUniqueTracksProvider( int index )
{
    changeSingleTracksProvider( index, m_uniqueTracksModels );
}

void
Process::changeExcludedTracksProvider( int index )
{
    changeSingleTracksProvider( index, m_excludedTracksModels );
}

void
Process::changeSingleTracksProvider( int index, const QMap<const Provider *, QAbstractItemModel *> &models )
{
    const Provider *provider =
        m_matchedTracksPage->filterCombo->itemData( index ).value<const Provider *>();
    m_matchedTracksPage->treeView->setModel( models.value( provider ) );
}
