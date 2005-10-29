/***************************************************************************
 * copyright            : (C) 2005 Seb Ruiz <me@sebruiz.net>               *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "amarokconfig.h"     //check if dynamic mode
#include "collectiondb.h"
#include "statistics.h"

#include <kapplication.h>
#include <klocale.h>
#include <knuminput.h>
#include <kwin.h>

#include <qcombobox.h>
#include <qframe.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qtabwidget.h>


//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS Statistics
//////////////////////////////////////////////////////////////////////////////////////////

Statistics *Statistics::s_instance = 0;

Statistics::Statistics( QWidget *parent, const char *name )
    : KDialogBase( KDialogBase::Swallow, 0, parent, name, false, 0, Ok )
    , m_gui( new StatisticsBase( this ) )
    , m_resultCount( 4 )
{
    s_instance = this;

    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    kapp->setTopWidget( this );
    setCaption( kapp->makeStdCaption( i18n("Statistics") ) );

    setMainWidget( m_gui );

    m_gui->m_tabWidget->setTabEnabled( m_gui->m_databaseTab, false );

    connect( m_gui->m_optionCombo,   SIGNAL( activated(int) ),    this, SLOT( loadDetails(int) ) );
    connect( m_gui->m_resultIntSpin, SIGNAL( valueChanged(int) ), this, SLOT( resultCountChanged(int) ) );

    loadSummary();
    loadChooser();

    QSize sz = sizeHint();
    setMinimumSize( kMax( 550, sz.width() ), kMax( 350, sz.height() ) );
    resize( sizeHint() );
}

Statistics::~Statistics()
{
    s_instance = 0;
}

void
Statistics::loadSummary()
{
    CollectionDB *db = CollectionDB::instance();

    QString mainText = "<div align=\"center\"><b>" + i18n("Collection Statistics") + "</b></div>";

    if( db->isEmpty() )
    {
        mainText += "<br>" + i18n("You need a collection to gather statistics") + "<br>";
        m_gui->m_mainInfoText->setText( mainText );
        return;
    }

    QueryBuilder qb;
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    QStringList a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Track","%n Tracks", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Artist","%n Artists", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Album","%n Albums", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue(QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Genre","%n Genres", a[0].toInt());
        mainText += "\n";
    }

    m_gui->m_mainInfoText->setText( mainText );
}

void
Statistics::loadChooser()
{
    loadDetails( m_gui->m_optionCombo->currentItem() );
    m_gui->m_resultIntSpin->setValue( m_resultCount );
}

void
Statistics::loadDetails( int index ) //SLOT
{
    m_gui->m_btrLabel->setText( i18n("Please hold...") );
    m_gui->m_bbrLabel->clear();
    m_gui->m_topCoverLabel->clear();
    m_gui->m_topCoverLabel->setFrameShape( QFrame::NoFrame );
    m_gui->m_bottomCoverLabel->clear();
    m_gui->m_bottomCoverLabel->setFrameShape( QFrame::NoFrame );

    switch( index )
    {
        case TRACK:
            buildTrackInfo();
            break;

        case ARTIST:
            buildArtistInfo();
            break;

        case ALBUM:
            buildAlbumInfo();
            break;

        case GENRE:
//             buildGenreInfo();
            break;
    }
}

void
Statistics::resultCountChanged( int value ) //SLOT
{
    if( value == m_resultCount )
        return;

    m_resultCount = value;
    loadDetails( m_gui->m_optionCombo->currentItem() );
}

void
Statistics::buildAlbumInfo()
{
    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore, true );
    qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
    qb.setLimit( 0, m_resultCount );
    QStringList faveAlbums = qb.run();

    ///Favourites
    QString image_fave = CollectionDB::instance()->albumImage( faveAlbums[1], faveAlbums[0], 100 );
    m_gui->m_topCoverLabel->setPixmap( QPixmap( image_fave ) );
    m_gui->m_topCoverLabel->setFrameShape( QFrame::StyledPanel );

    QString text = "<b>" + i18n("Favourite Albums") + "</b><br>";
    for( uint i=0; i < faveAlbums.count(); i += qb.countReturnValues() )
    {
        text += "<i>" + faveAlbums[i] + "</i>" + i18n(" - " ) + faveAlbums[i+1]
             /*+ i18n(" (Score: ") + faveAlbums[i+2] + i18n(")")*/;
        if( i + qb.countReturnValues() != faveAlbums.count() )
             text += "<br>";
    }
    m_gui->m_btrLabel->setText( text );

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabAlbum, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate );
    qb.sortByFunction( QueryBuilder::funcMax, QueryBuilder::tabSong, QueryBuilder::valCreateDate, true );
    qb.excludeMatch( QueryBuilder::tabAlbum, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valID);
    qb.groupBy( QueryBuilder::tabAlbum, QueryBuilder::valName);
    qb.setLimit( 0, m_resultCount );
    QStringList recentAlbums = qb.run();

    ///Newest
    QString image_recent = CollectionDB::instance()->albumImage( recentAlbums[1], recentAlbums[0], 100 );
    m_gui->m_bottomCoverLabel->setPixmap( QPixmap( image_recent ) );
    m_gui->m_bottomCoverLabel->setFrameShape( QFrame::StyledPanel );

    text = "<b>" + i18n("Newest Albums") + "</b><br>";
    for( uint i=0; i < recentAlbums.count(); i += qb.countReturnValues() )
    {
        text += "<i>" + recentAlbums[i] + "</i>" + i18n(" - " ) + recentAlbums[i+1];
        if( i + qb.countReturnValues() != recentAlbums.count() )
             text += "<br>";
    }
    m_gui->m_bbrLabel->setText( text );
}

void
Statistics::buildTrackInfo()
{
    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valScore, true );
    qb.setLimit( 0, m_resultCount );
    QStringList fave = qb.run();

    ///Favourites
    QString text = "<b>" + i18n("Favourite Songs") + "</b><br>";
    for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> - %2 (Score: %3)")
                    .arg( fave[i] )
                    .arg( fave[i+1] )
                    .arg( fave[i+2] );
        if( i + qb.countReturnValues() != fave.count() )
             text += "<br>";
    }
    m_gui->m_btrLabel->setText( text );

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
    qb.setLimit( 0, m_resultCount );
    QStringList mostPlayed = qb.run();

    ///Most Played
    text = "<b>" + i18n("Most Played Songs") + "</b><br>";
    for( uint i=0; i < mostPlayed.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> - %2 (Playcount: %3)")
                    .arg( mostPlayed[i] )
                    .arg( mostPlayed[i+1] )
                    .arg( mostPlayed[i+2] );
        if( i + qb.countReturnValues() != mostPlayed.count() )
             text += "<br>";
    }
    m_gui->m_bbrLabel->setText( text );
}

void
Statistics::buildArtistInfo()
{
    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore, true );
    qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
    qb.setLimit( 0, m_resultCount );
    QStringList faveArtists = qb.run();

    ///Favourites
    QString text = "<b>" + i18n("Favourite Artists") + "</b><br>";
    for( uint i=0; i < faveArtists.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> (Score: %2)")
                    .arg( faveArtists[i] )
                    .arg( faveArtists[i+1] );
        if( i + qb.countReturnValues() != faveArtists.count() )
             text += "<br>";
    }
    m_gui->m_btrLabel->setText( text );

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.sortByFunction( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTrack, true );
    qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
    qb.setLimit( 0, m_resultCount );
    QStringList mostSongs = qb.run();

    ///Artists with the Most Songs
    text = "<b>" + i18n("Artist Dominance") + "</b><br>"; // sebr: dominance an appropriate word?! :)
    for( uint i=0; i < mostSongs.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> (Count: %2)")
                    .arg( mostSongs[i] )
                    .arg( mostSongs[i+1] );
        if( i + qb.countReturnValues() != mostSongs.count() )
             text += "<br>";
    }
    m_gui->m_bbrLabel->setText( text );
}

#include "statistics.moc"

