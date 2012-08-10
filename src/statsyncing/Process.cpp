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
#include "core/interfaces/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "statsyncing/Controller.h"
#include "statsyncing/jobs/MatchTracksJob.h"
#include "statsyncing/jobs/SynchronizeTracksJob.h"
#include "statsyncing/models/MatchedTracksModel.h"
#include "statsyncing/models/ProvidersModel.h"
#include "statsyncing/models/SingleTracksModel.h"
#include "statsyncing/ui/ChooseProvidersPage.h"
#include "statsyncing/ui/MatchedTracksPage.h"

#include <ThreadWeaver/Weaver>

using namespace StatSyncing;

Process::Process( const ProviderPtrList &providers, const ProviderPtrSet &preSelectedProviders,
                  qint64 checkedFields, Process::Mode mode, QObject *parent )
    : QObject( parent )
    , m_mode( mode )
    , m_providersModel( new ProvidersModel( providers, preSelectedProviders, this ) )
    , m_checkedFields( checkedFields )
    , m_matchedTracksModel( 0 )
    , m_dialog( new KDialog() )
{
    m_dialog.data()->setCaption( i18n( "Synchronize Statistics" ) );
    m_dialog.data()->setButtons( KDialog::None );
    m_dialog.data()->setInitialSize( QSize( 860, 500 ) );
    m_dialog.data()->restoreDialogSize( Amarok::config( "StatSyncingDialog" ) );
    // delete this process when user hits the close button
    connect( m_dialog.data(), SIGNAL(finished()), SLOT(deleteLater()) );
}

Process::~Process()
{
    if( m_dialog )
    {
        KConfigGroup group = Amarok::config( "StatSyncingDialog" );
        m_dialog.data()->saveDialogSize( group );
        delete m_dialog.data(); // we cannot deleteLater, dialog references m_matchedTracksModel
    }
}

void
Process::start()
{
    if( m_mode == Interactive )
    {
        m_providersPage = new ChooseProvidersPage();
        m_providersPage.data()->setFields( Controller::availableFields(), m_checkedFields );
        m_providersPage.data()->setProvidersModel( m_providersModel, m_providersModel->selectionModel() );

        connect( m_providersPage.data(), SIGNAL(accepted()), SLOT(slotMatchTracks()) );
        connect( m_providersPage.data(), SIGNAL(rejected()), SLOT(deleteLater()) );
        m_dialog.data()->mainWidget()->hide(); // otherwise it may last as a ghost image
        m_dialog.data()->setMainWidget( m_providersPage.data() ); // takes ownership
        raise();
    }
    else if( m_checkedFields )
        slotMatchTracks();
}

void
Process::raise()
{
    if( m_providersPage || m_tracksPage )
    {
        m_dialog.data()->show();
        m_dialog.data()->activateWindow();
        m_dialog.data()->raise();
    }
    else
        m_mode = Interactive; // schedule dialog should be shown when something happens
}

void
Process::slotMatchTracks()
{
    MatchTracksJob *job = new MatchTracksJob( m_providersModel->selectedProviders() );
    QString text = i18n( "Matching Tracks for Statistics Synchronization" );
    if( m_providersPage )
    {
        ChooseProvidersPage *page = m_providersPage.data(); // too lazy to type
        m_checkedFields = page->checkedFields();

        page->disableControls();
        page->setProgressBarText( text );
        connect( job, SIGNAL(totalSteps(int)), page, SLOT(setProgressBarMaximum(int)) );
        connect( job, SIGNAL(incrementProgress()), page, SLOT(progressBarIncrementProgress()) );
        connect( page, SIGNAL(rejected()), job, SLOT(abort()) );
        connect( m_dialog.data(), SIGNAL(finished()), job, SLOT(abort()) );
    }
    else // background operation
    {
        Amarok::Components::logger()->newProgressOperation( job, text, 100, job, SLOT(abort()) );
    }

    connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(slotTracksMatched(ThreadWeaver::Job*)) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

void
Process::slotTracksMatched( ThreadWeaver::Job *job )
{
    // won't be needed after this method returns; needs to be before early-returns to
    // prevent memory and refcounting leak
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

    // remove fields that are not writable:
    qint64 usedFields = m_checkedFields & m_providersModel->writableTrackStatsDataUnion();
    m_options.setSyncedFields( usedFields );
    QList<qint64> columns = QList<qint64>() << Meta::valTitle;
    foreach( qint64 field, Controller::availableFields() )
    {
        if( field & usedFields )
            columns << field;
    }

    m_matchedTracksModel = new MatchedTracksModel( matchJob->matchedTuples(), columns,
                                                   m_options, this );

    if( m_matchedTracksModel->hasConflict() || m_mode == Interactive )
    {
        m_tracksPage = new MatchedTracksPage();
        m_tracksPage.data()->setProviders( matchJob->providers() );
        m_tracksPage.data()->setMatchedTracksModel( m_matchedTracksModel );
        foreach( ProviderPtr provider, matchJob->providers() )
        {
            if( !matchJob->uniqueTracks().value( provider ).isEmpty() )
                m_tracksPage.data()->addUniqueTracksModel( provider, new SingleTracksModel(
                        matchJob->uniqueTracks().value( provider ), columns, m_tracksPage.data() ) );
            if( !matchJob->excludedTracks().value( provider ).isEmpty() )
                m_tracksPage.data()->addExcludedTracksModel( provider, new SingleTracksModel(
                    matchJob->excludedTracks().value( provider ), columns, m_tracksPage.data() ) );
        }

        connect( m_tracksPage.data(), SIGNAL(back()), SLOT(slotBack()) );
        connect( m_tracksPage.data(), SIGNAL(accepted()), SLOT(slotSynchronize()) );
        connect( m_tracksPage.data(), SIGNAL(rejected()), SLOT(deleteLater()) );
        m_dialog.data()->mainWidget()->hide(); // otherwise it may last as a ghost image
        m_dialog.data()->setMainWidget( m_tracksPage.data() ); // takes ownership
        raise();
    }
    else // NonInteractive mode without conflict
        slotSynchronize();
}

void
Process::slotBack()
{
    m_mode = Interactive; // reset mode, we're interactive from this point
    start();
}

void
Process::slotSynchronize()
{
    // disconnect, otherwise we prematurely delete Process and thus m_matchedTracksModel
    disconnect( m_dialog.data(), SIGNAL(finished()), this, SLOT(deleteLater()) );
    m_dialog.data()->close();

    SynchronizeTracksJob *job =
            new SynchronizeTracksJob( m_matchedTracksModel->matchedTuples(), m_options );
    QString text = i18n( "Synchronizing Track Statistics" );
    Amarok::Components::logger()->newProgressOperation( job, text, 100, job, SLOT(abort()) );
    connect( job, SIGNAL(endProgressOperation(QObject*,int)),
             SLOT(slotLogSynchronization(QObject*,int)) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(deleteLater()) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

void
Process::slotLogSynchronization( QObject *job, int updatedTracksCount )
{
    Q_UNUSED( job )
    QStringList providerNames;
    foreach( ProviderPtr provider, m_providersModel->selectedProviders() )
        providerNames << provider->prettyName();
    QString providers = providerNames.join( i18nc( "comma between list words", ", " ) );
    QString text = i18ncp( "%2 is a list of collection names", "Synchronization of %2 "
            "done. One track was updated.", "Synchronization of %2 done. %1 tracks were "
            "updated.", updatedTracksCount, providers );
    Amarok::Components::logger()->longMessage( text );
}
