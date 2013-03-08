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

#include "MainWindow.h"
#include "MetaValues.h"
#include "core/interfaces/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "statsyncing/Config.h"
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
    connect( m_dialog.data(), SIGNAL(finished()), SLOT(slotSaveSizeAndDelete()) );

    /* we need to delete all QWidgets on application exit well before QApplication
     * is destroyed. We however don't set MainWindow as parent as this would make
     * StatSyncing dialog share taskbar entry with Amarok main window */
    connect( The::mainWindow(), SIGNAL(destroyed(QObject*)), SLOT(slotDeleteDialog()) );
}

Process::~Process()
{
    delete m_dialog.data(); // we cannot deleteLater, dialog references m_matchedTracksModel
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
        connect( m_providersPage.data(), SIGNAL(rejected()), SLOT(slotSaveSizeAndDelete()) );
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
    StatSyncing::Controller *controller = Amarok::Components::statSyncingController();
    if( !controller )
    {
        warning() << __PRETTY_FUNCTION__ << "StatSyncing::Controller disappeared";
        deleteLater();
        return;
    }

    // remove fields that are not writable:
    qint64 usedFields = m_checkedFields & m_providersModel->writableTrackStatsDataUnion();
    m_options.setSyncedFields( usedFields );
    m_options.setExcludedLabels( controller->config()->excludedLabels() );
    QList<qint64> columns = QList<qint64>() << Meta::valTitle;
    foreach( qint64 field, Controller::availableFields() )
    {
        if( field & usedFields )
            columns << field;
    }

    m_matchedTracksModel = new MatchedTracksModel( matchJob->matchedTuples(), columns,
                                                   m_options, this );
    QList<ScrobblingServicePtr> services = controller->scrobblingServices();
    // only fill in m_tracksToScrobble if there are actual scrobbling services available
    m_tracksToScrobble = services.isEmpty() ? TrackList() : matchJob->tracksToScrobble();

    if( m_matchedTracksModel->hasConflict() || m_mode == Interactive )
    {
        m_tracksPage = new MatchedTracksPage();
        MatchedTracksPage *page = m_tracksPage.data(); // convenience
        page->setProviders( matchJob->providers() );
        page->setMatchedTracksModel( m_matchedTracksModel );
        foreach( ProviderPtr provider, matchJob->providers() )
        {
            if( !matchJob->uniqueTracks().value( provider ).isEmpty() )
                page->addUniqueTracksModel( provider, new SingleTracksModel(
                        matchJob->uniqueTracks().value( provider ), columns, m_options, page ) );
            if( !matchJob->excludedTracks().value( provider ).isEmpty() )
                page->addExcludedTracksModel( provider, new SingleTracksModel(
                    matchJob->excludedTracks().value( provider ), columns, m_options, page ) );
        }
        page->setTracksToScrobble( m_tracksToScrobble, services );

        connect( page, SIGNAL(back()), SLOT(slotBack()) );
        connect( page, SIGNAL(accepted()), SLOT(slotSynchronize()) );
        connect( page, SIGNAL(rejected()), SLOT(slotSaveSizeAndDelete()) );
        m_dialog.data()->mainWidget()->hide(); // otherwise it may last as a ghost image
        m_dialog.data()->setMainWidget( page ); // takes ownership
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
    disconnect( m_dialog.data(), SIGNAL(finished()), this, SLOT(slotSaveSizeAndDelete()) );
    m_dialog.data()->close();

    SynchronizeTracksJob *job = new SynchronizeTracksJob(
            m_matchedTracksModel->matchedTuples(), m_tracksToScrobble, m_options );
    QString text = i18n( "Synchronizing Track Statistics" );
    Amarok::Components::logger()->newProgressOperation( job, text, 100, job, SLOT(abort()) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), SLOT(slotLogSynchronization(ThreadWeaver::Job*)) );
    connect( job, SIGNAL(done(ThreadWeaver::Job*)), job, SLOT(deleteLater()) );
    ThreadWeaver::Weaver::instance()->enqueue( job );
}

void
Process::slotLogSynchronization( ThreadWeaver::Job *job )
{
    deleteLater(); // our work is done
    SynchronizeTracksJob *syncJob = qobject_cast<SynchronizeTracksJob *>( job );
    if( !syncJob )
    {
        warning() << __PRETTY_FUNCTION__ << "syncJob is null";
        return;
    }

    int updatedTracksCount = syncJob->updatedTracksCount();
    QMap<ScrobblingServicePtr, QMap<ScrobblingService::ScrobbleError, int> > scrobbles =
            syncJob->scrobbles();

    QStringList providerNames;
    foreach( ProviderPtr provider, m_providersModel->selectedProviders() )
        providerNames << "<b>" + provider->prettyName() + "</b>";
    QString providers = providerNames.join( i18nc( "comma between list words", ", " ) );

    QStringList text = QStringList() << i18ncp( "%2 is a list of collection names",
            "Synchronization of %2 done. <b>One</b> track was updated.",
            "Synchronization of %2 done. <b>%1</b> tracks were updated.",
            updatedTracksCount, providers );

    int fromDistantPast = 0;
    int otherErrors = 0;
    foreach( const ScrobblingServicePtr &provider, scrobbles.keys() )
    {
        QString name = "<b>" + provider->prettyName() + "</b>";
        QMap<ScrobblingService::ScrobbleError, int> &providerScrobbles = scrobbles[ provider ];

        QMapIterator<ScrobblingService::ScrobbleError, int> it( providerScrobbles );
        while( it.hasNext() )
        {
            it.next();
            switch( it.key() )
            {
                case ScrobblingService::NoError:
                    text << i18np( "<b>One</b> track was scrobbled to %2.",
                            "<b>%1</b> tracks were scrobbled to %2.", it.value(), name );
                    break;
                case ScrobblingService::FromTheDistantPast:
                    fromDistantPast += it.value();
                    break;
                case ScrobblingService::TooShort:
                case ScrobblingService::BadMetadata:
                case ScrobblingService::FromTheFuture:
                case ScrobblingService::SkippedByUser:
                    otherErrors += it.value();
                    break;
            }
        }
    }
    if( fromDistantPast )
        text << i18np( "<b>One</b> track was last played in too distant past to be scrobbled.",
                       "<b>%1</b> tracks were last played in too distant past to be scrobbled.",
                       fromDistantPast );
    if( otherErrors )
        text << i18np( "Failed to scrobble <b>one</b> track.",
                       "Failed to scrobble <b>%1</b> tracks.", otherErrors );

    Amarok::Components::logger()->longMessage( text.join( "<br>\n" ) );
}

void
Process::slotSaveSizeAndDelete()
{
    if( m_dialog )
    {
        KConfigGroup group = Amarok::config( "StatSyncingDialog" );
        m_dialog.data()->saveDialogSize( group );
    }
    deleteLater();
}

void
Process::slotDeleteDialog()
{
    // we cannot use deleteLater(), we don't have spare eventloop iteration
    delete m_dialog.data();
}
