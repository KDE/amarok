/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "ProgressiveSearchWidget.h"

#include "Debug.h"
#include "playlist/PlaylistModel.h"

#include <KAction>
#include <KLineEdit>
#include <KLocale>
#include <KColorScheme>

#include <QMenu>
#include <QToolBar>

ProgressiveSearchWidget::ProgressiveSearchWidget( QWidget * parent )
    : KHBox( parent )
{

    readConfig();

    m_searchEdit = new KLineEdit( this );
    m_searchEdit->setClickMessage( i18n( "Search playlist" ) );
    m_searchEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_searchEdit->setClearButtonShown( true );
    m_searchEdit->setFrame( true );
    m_searchEdit->setToolTip( i18n( "Start typing to progressively search through the playlist" ) );

    connect( m_searchEdit, SIGNAL( textChanged( const QString & ) ), this, SLOT( slotFilterChanged(  const QString &  ) ) );

    QToolBar * toolbar = new QToolBar( this );

    m_nextAction = new KAction( KIcon( "go-down" ), i18n( "&Next" ), this );
    connect( m_nextAction, SIGNAL( triggered() ), this, SLOT( slotNext() ) );
    
    m_previousAction = new KAction( KIcon( "go-up" ), i18n( "&Previous" ), this );
    connect( m_previousAction, SIGNAL( triggered() ), this, SLOT( slotPrevious() ) );

    toolbar->addAction( m_nextAction );
    toolbar->addAction( m_previousAction );

    m_nextAction->setEnabled( true );
    m_previousAction->setEnabled( true );

    m_menu = new QMenu( this );

    KAction * searchTracksAction = new KAction( i18n( "Tracks" ), this );
    searchTracksAction->setCheckable( true );
    connect( searchTracksAction, SIGNAL( toggled( bool ) ), this, SLOT( slotSearchTracks( bool ) ) );
    if( m_searchFieldsMask & Playlist::MatchTrack )
        searchTracksAction->setChecked( true );
    m_menu->addAction( searchTracksAction );

    KAction * searchAlbumsAction = new KAction( i18n( "Albums" ), this );
    searchAlbumsAction->setCheckable( true );
    connect( searchAlbumsAction, SIGNAL( toggled( bool ) ), this, SLOT( slotSearchAlbums( bool ) ) );
    if( m_searchFieldsMask & Playlist::MatchAlbum )
        searchAlbumsAction->setChecked( true );
    m_menu->addAction( searchAlbumsAction );
    
    KAction * searchArtistsAction = new KAction( i18n( "Artists" ), this );
    searchArtistsAction->setCheckable( true );
    connect( searchArtistsAction, SIGNAL( toggled( bool ) ), this, SLOT( slotSearchArtists( bool ) ) );
    if( m_searchFieldsMask & Playlist::MatchArtist )
        searchArtistsAction->setChecked( true );
    m_menu->addAction( searchArtistsAction );

    KAction * searchGenreAction = new KAction( i18n( "Genre" ), this );
    searchGenreAction->setCheckable( true );
    connect( searchGenreAction, SIGNAL( toggled( bool ) ), this, SLOT( slotSearchGenre( bool ) ) );
    if( m_searchFieldsMask & Playlist::MatchGenre )
        searchGenreAction->setChecked( true );
    m_menu->addAction( searchGenreAction );

    KAction * searchComposersAction = new KAction( i18n( "Composers" ), this );
    searchComposersAction->setCheckable( true );
    connect( searchComposersAction, SIGNAL( toggled( bool ) ), this, SLOT( slotSearchComposers( bool ) ) );
    if( m_searchFieldsMask & Playlist::MatchComposer )
        searchComposersAction->setChecked( true );
    m_menu->addAction( searchComposersAction );


    KAction * searchYearsAction = new KAction( i18n( "Years" ), this );
    searchYearsAction->setCheckable( true );
    connect( searchYearsAction, SIGNAL( toggled( bool ) ), this, SLOT( slotSearchYears( bool ) ) );
    if( m_searchFieldsMask & Playlist::MatchYear)
        searchYearsAction->setChecked( true );
    m_menu->addAction( searchYearsAction );

    m_menu->addSeparator();

    KAction * playOnlyMatchesAction = new KAction( i18n( "Play only matches" ), this );
    playOnlyMatchesAction->setCheckable( true );
    connect( playOnlyMatchesAction, SIGNAL( toggled( bool ) ), this, SLOT( slotPlayOnlyMatches( bool ) ) );

    KConfigGroup config = Amarok::config("Playlist Search");
    playOnlyMatchesAction->setChecked( config.readEntry( "PlayOnlyMatches", true ) );
    m_menu->addAction( playOnlyMatchesAction );

    KAction * searchMenuAction = new KAction(KIcon( "preferences-other" ), i18n( "Search Preferences" ), this );

    searchMenuAction->setMenu( m_menu );
    

    toolbar->addAction( searchMenuAction );

}


ProgressiveSearchWidget::~ProgressiveSearchWidget()
{
}

void ProgressiveSearchWidget::slotFilterChanged( const QString & filter )
{
    DEBUG_BLOCK
    debug() << "New filter: " << filter;

    if( filter.isEmpty() ) {
        
        m_nextAction->setEnabled( false );
        m_previousAction->setEnabled( false );

        QPalette p = m_searchEdit->palette();
        p.setColor( QPalette::Base, palette().color( QPalette::Base ) );
        m_searchEdit->setPalette( p );

        emit( filterCleared() );
        
    } else
        emit( filterChanged( filter, m_searchFieldsMask ) );

}

void ProgressiveSearchWidget::slotNext()
{
    DEBUG_BLOCK
    emit( next( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::slotPrevious()
{
    DEBUG_BLOCK
    emit( previous( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::match()
{
    m_nextAction->setEnabled( true );
    m_previousAction->setEnabled( true );

    QPalette p = m_searchEdit->palette();
    p.setColor( QPalette::Base, palette().color( QPalette::Base ) );
    m_searchEdit->setPalette( p );

}

void ProgressiveSearchWidget::noMatch()
{
    m_nextAction->setEnabled( false );
    m_previousAction->setEnabled( false );


    KStatefulBrush backgroundBrush( KColorScheme::View,KColorScheme::NegativeBackground );

    QPalette p = m_searchEdit->palette();
    p.setColor( QPalette::Base, backgroundBrush.brush( m_searchEdit ).color() );
    m_searchEdit->setPalette( p );
}


void ProgressiveSearchWidget::slotSearchTracks( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchTrack;
    else
        m_searchFieldsMask ^= Playlist::MatchTrack;

    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "MatchTrack", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::slotSearchArtists( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchArtist;
    else
        m_searchFieldsMask ^= Playlist::MatchArtist;

    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "MatchArtist", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::slotSearchAlbums( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchAlbum;
    else
        m_searchFieldsMask ^= Playlist::MatchAlbum;

    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "MatchAlbum", search );
    
    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::slotSearchGenre( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchGenre;
    else
        m_searchFieldsMask ^= Playlist::MatchGenre;

    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "MatchGenre", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::slotSearchComposers( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchComposer;
    else
        m_searchFieldsMask ^= Playlist::MatchComposer;

    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "MatchComposer", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::slotSearchYears( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchYear;
    else
        m_searchFieldsMask ^= Playlist::MatchYear;

    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "MatchYear", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask ) );
}

void ProgressiveSearchWidget::readConfig()
{
    m_searchFieldsMask = 0;
    
    KConfigGroup config = Amarok::config("Playlist Search");
    if( config.readEntry( "MatchTrack", true ) )
        m_searchFieldsMask |= Playlist::MatchTrack;
    if( config.readEntry( "MatchArtist", true ) )
        m_searchFieldsMask |= Playlist::MatchArtist;
    if( config.readEntry( "MatchAlbum", true ) )
        m_searchFieldsMask |= Playlist::MatchAlbum;
    if( config.readEntry( "MatchGenre", false ) )
        m_searchFieldsMask |= Playlist::MatchGenre;
    if( config.readEntry( "MatchComposer", false ) )
        m_searchFieldsMask |= Playlist::MatchComposer;
    if( config.readEntry( "MatchYear", false ) )
        m_searchFieldsMask |= Playlist::MatchYear;
}

void ProgressiveSearchWidget::slotPlayOnlyMatches( bool onlyMatches )
{
    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "PlayOnlyMatches", onlyMatches );

    emit( playOnlyMatches( onlyMatches ) );
}


#include "ProgressiveSearchWidget.moc"