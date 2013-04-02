/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicBrainzTagDialog"

#include "MusicBrainzTagger.h"

#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "ui_MusicBrainzTagger.h"

#include <KMessageBox>
#include <QTimer>
#include <QToolBar>

MusicBrainzTagger::MusicBrainzTagger( const Meta::TrackList &tracks,
                                      QWidget *parent )
    : KDialog( parent )
    , ui( new Ui::MusicBrainzTagger() )
{
    DEBUG_BLOCK
    foreach( Meta::TrackPtr track, tracks)
    {
        if( !track->playableUrl().toLocalFile().isEmpty() )
            m_tracks << track;
    }
    ui->setupUi( mainWidget() );

    init();
    search();
}

MusicBrainzTagger::~MusicBrainzTagger()
{
    delete mb_finder;
#ifdef HAVE_LIBOFA
    delete mdns_finder;
#endif
    delete ui;
}

void
MusicBrainzTagger::init()
{
    DEBUG_BLOCK
    setButtons( KDialog::None );
    setAttribute( Qt::WA_DeleteOnClose );
    setMinimumSize( 480, 300 );

    if( m_tracks.count() > 1 )
    {
        QToolBar *toolBar = new QToolBar( this );
        toolBar->addAction( i18n( "Expand All" ), ui->treeView_Result, SLOT(expandAll()) );
        toolBar->addAction( i18n( "Collapse All" ), ui->treeView_Result, SLOT(collapseAll()) );
        toolBar->addAction( i18n( "Expand Unchosen" ), ui->treeView_Result, SLOT(expandUnChosen()) );
        toolBar->addAction( i18n( "Collapse Chosen" ), ui->treeView_Result, SLOT(collapseChosen()) );
        ui->verticalLayout->insertWidget( 0, toolBar );
    }

    ui->progressBar->hide();

    mb_finder = new MusicBrainzFinder( this );
    q_resultsModel = new MusicBrainzTagsModel( this );
    q_resultsModelDelegate = new MusicBrainzTagsModelDelegate( this );
    ui->treeView_Result->setModel( q_resultsModel );
    ui->treeView_Result->setItemDelegateForColumn( 0, q_resultsModelDelegate );
    ui->treeView_Result->header()->setClickable( true );

#ifdef HAVE_LIBOFA
    mdns_finder = new MusicDNSFinder( this );
    connect( mdns_finder, SIGNAL(trackFound(Meta::TrackPtr,QString)),
             mb_finder, SLOT(lookUpByPUID(Meta::TrackPtr,QString)) );
    connect( mdns_finder, SIGNAL(progressStep()), SLOT(progressStep()) );
    connect( mdns_finder, SIGNAL(done()), this, SLOT(mdnsSearchDone()) );
#endif
    connect( mb_finder, SIGNAL(done()), SLOT(searchDone()) );
    connect( mb_finder, SIGNAL(trackFound(Meta::TrackPtr,QVariantMap)),
             q_resultsModel, SLOT(addTrack(Meta::TrackPtr,QVariantMap)) );
    connect( mb_finder, SIGNAL(progressStep()), SLOT(progressStep()) );
    connect( ui->treeView_Result->header(), SIGNAL(sectionClicked(int)),
             q_resultsModel, SLOT(selectAll(int)) );
    connect( ui->pushButton_saveAndClose, SIGNAL(clicked(bool)), SLOT(saveAndExit()) );
    connect( ui->pushButton_cancel, SIGNAL(clicked(bool)), SLOT(reject()) );
}

void
MusicBrainzTagger::search()
{
    ui->progressBar->setRange( 0, m_tracks.count() * 2 );
    ui->progressBar->setValue( 0 );
    ui->horizontalSpacer->changeSize( 0, 0, QSizePolicy::Ignored );
    ui->progressBar->show();
    mb_finder->run( m_tracks );
#ifdef HAVE_LIBOFA
    mdns_searchDone = false;
    mdns_finder->run( m_tracks );
#endif
}

void
MusicBrainzTagger::saveAndExit()
{
    QMap < Meta::TrackPtr, QVariantMap > result = q_resultsModel->getAllChecked();

    if( !result.isEmpty() )
        emit sendResult( result );

    accept();
}

void
MusicBrainzTagger::searchDone()
{
    DEBUG_BLOCK
#ifdef HAVE_LIBOFA
    if( !mdns_searchDone )
        return;
#endif
    ui->horizontalSpacer->changeSize( 0, 0, QSizePolicy::Expanding );
    ui->progressBar->hide();
    ui->treeView_Result->expandAll();
    ui->treeView_Result->header()->resizeSections( QHeaderView::ResizeToContents );
}

#ifdef HAVE_LIBOFA
void
MusicBrainzTagger::mdnsSearchDone()
{
    DEBUG_BLOCK
    mdns_searchDone = true;
    if( !mb_finder->isRunning() )
        searchDone();
}
#endif

void
MusicBrainzTagger::progressStep()
{
    ui->progressBar->setValue( ui->progressBar->value() + 1 );
}

#include "MusicBrainzTagger.moc"

