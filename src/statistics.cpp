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
#include "amarok.h"         //foreach macro
#include "collectiondb.h"
#include "statistics.h"

#include <kapplication.h>
#include <klocale.h>
#include <kwin.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qframe.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qtextedit.h>

#include <cmath>

//////////////////////////////////////////////////////////////////////////////////////////
/// CLASS Statistics
//////////////////////////////////////////////////////////////////////////////////////////

Statistics *Statistics::s_instance = 0;

Statistics::Statistics( QWidget *parent, const char *name )
    : KDialogBase( KDialogBase::Swallow, 0, parent, name, false, 0, Close )
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

    connect( m_gui->m_optionCombo, SIGNAL( activated(int) ), this, SLOT( loadDetails(int) ) );
    connect( m_gui->m_resultCombo, SIGNAL( activated(int) ), this, SLOT( resultCountChanged(int) ) );

    if( CollectionDB::instance()->isEmpty() )
    {
        m_gui->m_chooserFrame->hide();
        m_gui->m_infoFrame->hide();
    }
    else
        loadChooser();

    loadSummary();
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
    qb.addReturnFunctionValue( QueryBuilder::funcSum, QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    QStringList a = qb.run();
    if( !a.isEmpty() )
    {
        mainText += i18n("1 Play","%n Plays", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valURL );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Track","%n Tracks", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabArtist, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Artist","%n Artists", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabAlbum, QueryBuilder::valID );
    qb.setOptions( QueryBuilder::optRemoveDuplicates );
    a = qb.run();

    if( !a.isEmpty() )
    {
        mainText += i18n("1 Album","%n Albums", a[0].toInt());
        mainText += "<br>";
    }

    qb.clear();
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabGenre, QueryBuilder::valID );
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
    m_gui->m_optionCombo->insertItem( i18n("Track"),  TRACK );
    m_gui->m_optionCombo->insertItem( i18n("Artist"), ARTIST );
    m_gui->m_optionCombo->insertItem( i18n("Album"),  ALBUM );
    m_gui->m_optionCombo->insertItem( i18n("Genre"),  GENRE );

    m_gui->m_resultCombo->insertItem( QString::number(5)  );
    m_gui->m_resultCombo->insertItem( QString::number(10) );
    m_gui->m_resultCombo->insertItem( QString::number(25) );
    m_gui->m_resultCombo->insertItem( QString::number(50) );

    loadDetails( TRACK );
}

void
Statistics::loadDetails( int index ) //SLOT
{
    m_gui->m_btrView->setText( i18n("Please hold...") );
    m_gui->m_topCoverLabel->clear();
    m_gui->m_topCoverLabel->setFrameShape( QFrame::NoFrame );
    m_gui->m_bottomCoverLabel->clear();
    m_gui->m_bottomCoverLabel->setFrameShape( QFrame::NoFrame );

    m_dataUpper.clear();
    m_textUpper.clear();
    m_dataLower.clear();
    m_textLower.clear();

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
            buildGenreInfo();
            break;
    }
}

void
Statistics::resultCountChanged( int value ) //SLOT
{
    int v = m_gui->m_resultCombo->text( value ).toInt();
    if( v == m_resultCount )
        return;

    m_resultCount = v;
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

    QString text = "<b>" + i18n("Favorite Albums") + "</b><br>";
    for( uint i=0; i < faveAlbums.count(); i += qb.countReturnValues() )
    {
        text += "<i>" + faveAlbums[i] + "</i>" + i18n(" - " ) + faveAlbums[i+1]
             /*+ i18n(" (Score: ") + faveAlbums[i+2] + i18n(")")*/;
        if( i + qb.countReturnValues() != faveAlbums.count() )
             text += "<br>";
    }

    text += "<br><br>";

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

    text += "<b>" + i18n("Newest Albums") + "</b><br>";
    for( uint i=0; i < recentAlbums.count(); i += qb.countReturnValues() )
    {
        text += "<i>" + recentAlbums[i] + "</i>" + i18n(" - " ) + recentAlbums[i+1];
        if( i + qb.countReturnValues() != recentAlbums.count() )
             text += "<br>";
    }

    m_gui->m_btrView->setText( text );
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
    QString text = "<b>" + i18n("Favorite Songs") + "</b><br>";
    for( uint i=0; i < fave.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> - %2 (Score: %3)")
                    .arg( fave[i] )
                    .arg( fave[i+1] )
                    .arg( fave[i+2] );
        if( i + qb.countReturnValues() != fave.count() )
             text += "<br>";
    }

    text += "<br><br>";

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabSong, QueryBuilder::valTitle );
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnValue( QueryBuilder::tabStats, QueryBuilder::valPlayCounter );
    qb.sortBy( QueryBuilder::tabStats, QueryBuilder::valPlayCounter, true );
    qb.setLimit( 0, m_resultCount );
    QStringList mostPlayed = qb.run();

    ///Most Played
    text += "<b>" + i18n("Most Played Songs") + "</b><br>";
    for( uint i=0; i < mostPlayed.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> - %2 (Playcount: %3)")
                    .arg( mostPlayed[i] )
                    .arg( mostPlayed[i+1] )
                    .arg( mostPlayed[i+2] );
        if( i + qb.countReturnValues() != mostPlayed.count() )
             text += "<br>";
    }

    m_gui->m_btrView->setText( text );
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
    QString text = "<b>" + i18n("Favorite Artists") + "</b><br>";
    for( uint i=0; i < faveArtists.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> (Score: %2)")
                    .arg( faveArtists[i] )
                    .arg( faveArtists[i+1] );
        if( i + qb.countReturnValues() != faveArtists.count() )
             text += "<br>";
    }

    text += "<br><br>";

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabArtist, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.sortByFunction( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTrack, true );
    qb.groupBy( QueryBuilder::tabArtist, QueryBuilder::valName);
    qb.setLimit( 0, m_resultCount );
    QStringList mostSongs = qb.run();

    ///Artists with the Most Songs
    text += "<b>" + i18n("Artist Count") + "</b><br>"; // sebr: dominance an appropriate word?! :)
    for( uint i=0; i < mostSongs.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> (Count: %2)")
                    .arg( mostSongs[i] )
                    .arg( mostSongs[i+1] );
        if( i + qb.countReturnValues() != mostSongs.count() )
             text += "<br>";

        m_textLower.append( mostSongs[i] );
        m_dataLower.append( mostSongs[i+1].toDouble() );
    }

//     m_gui->m_topCoverLabel->setText( i18n("<b>Artist Count</b>") );
//     drawPie( m_gui->m_bottomCoverLabel, m_dataLower );

    m_gui->m_btrView->setText( text );
}

void
Statistics::buildGenreInfo()
{
    QueryBuilder qb;
    qb.clear();
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore );
    qb.sortByFunction( QueryBuilder::funcAvg, QueryBuilder::tabStats, QueryBuilder::valScore, true );
    qb.excludeMatch( QueryBuilder::tabGenre, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.setLimit( 0, m_resultCount );
    QStringList faveGenres = qb.run();

    ///Favourites
    QString text = "<b>" + i18n("Favorite Genres") + "</b><br>";
    for( uint i=0; i < faveGenres.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> (Score: %2)")
                    .arg( faveGenres[i] )
                    .arg( faveGenres[i+1] );
        if( i + qb.countReturnValues() != faveGenres.count() )
             text += "<br>";
    }

    text += "<br><br>";

    qb.clear();
    qb.addReturnValue( QueryBuilder::tabGenre, QueryBuilder::valName );
    qb.addReturnFunctionValue( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTrack );
    qb.sortByFunction( QueryBuilder::funcCount, QueryBuilder::tabSong, QueryBuilder::valTrack, true );
    qb.excludeMatch( QueryBuilder::tabGenre, i18n( "Unknown" ) );
    qb.groupBy( QueryBuilder::tabGenre, QueryBuilder::valName);
    qb.setLimit( 0, m_resultCount );
    QStringList mostGenres = qb.run();

    ///Genres with the Most Songs
    text += "<b>" + i18n("Genre Count") + "</b><br>";
    for( uint i=0; i < mostGenres.count(); i += qb.countReturnValues() )
    {
        text += i18n("<i>%1</i> (Count: %2)")
                    .arg( mostGenres[i] )
                    .arg( mostGenres[i+1] );
        if( i + qb.countReturnValues() != mostGenres.count() )
             text += "<br>";

        m_textLower.append( mostGenres[i] );
        m_dataLower.append( mostGenres[i+1].toDouble() );
    }

//     m_gui->m_topCoverLabel->setText( i18n("<b>Genre Count</b>") );
//     drawPie( m_gui->m_bottomCoverLabel, m_dataLower );

    m_gui->m_btrView->setText( text );
}

void
Statistics::drawPie( QLabel *parent, QValueList<double> data, QStringList /*text*/ )
{
    double total = 0.0;

    foreachType( QValueList<double>, data )
        total += *it;

    if( !total ) return;

    //TODO: Make resizeable
    const int w = 200;
    const int h = 200;

    const int xd = w - w/8;
    const int yd = h - h/8;

    QPixmap pm;
    pm.resize( w, h);
    pm.fill  ( backgroundColor() );

    QPainter p( &pm );

    int apos = 0;
    int i = 0;

    foreachType( QValueList<double>, data )
    {
        ///Draw graph
        QColor c;

        c.setHsv( ( i * 255) / data.count(), 255, 255 );// rainbow effect
        p.setBrush( c );                                // solid fill with color c

        int a = int( ( (*it) * 360.0 ) / total * 16.0 + 0.5);
        p.drawPie( 0, h/10, xd, yd, -apos, -a );
        apos += a;
        i++;
    }

    parent->setPixmap( pm );
}

#include "statistics.moc"

