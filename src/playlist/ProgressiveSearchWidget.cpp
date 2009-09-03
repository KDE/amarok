/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "ProgressiveSearchWidget.h"

#include "Debug.h"
#include "playlist/PlaylistModel.h"

#include <KAction>
#include <KColorScheme>
#include <KHBox>
#include <KLocale>

#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QToolBar>
#include <QToolButton>

namespace Playlist
{

ProgressiveSearchWidget::ProgressiveSearchWidget( QWidget * parent )
    : KVBox( parent )
{
    DEBUG_BLOCK

    readConfig();

    KHBox *searchBox = new KHBox( this );
    m_warningLabel = new QLabel( i18n("Warning: tracks have been hidden in the playlist"), this );
    hideHiddenTracksWarning();

    m_searchEdit = new Amarok::LineEdit( searchBox );
    m_searchEdit->setClickMessage( i18n( "Search playlist" ) );
    m_searchEdit->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_searchEdit->setClearButtonShown( true );
    m_searchEdit->setFrame( true );
    m_searchEdit->setToolTip( i18n( "Start typing to progressively search through the playlist" ) );
    m_searchEdit->setFocusPolicy( Qt::ClickFocus ); // Without this, the widget goes into text input mode directly on startup

    connect( m_searchEdit, SIGNAL( textChanged( const QString & ) ), this, SLOT( slotFilterChanged(  const QString &  ) ) );
    connect( m_searchEdit, SIGNAL( returnPressed( const QString & ) ), this, SIGNAL( activateFilterResult() ) );
    connect( m_searchEdit, SIGNAL( downPressed() ), this, SIGNAL( downPressed() ) );

    QToolBar *toolbar = new QToolBar( searchBox );

    m_nextAction = new KAction( KIcon( "go-down" ), i18n( "&Next" ), this );
    connect( m_nextAction, SIGNAL( triggered() ), this, SLOT( slotNext() ) );

    m_previousAction = new KAction( KIcon( "go-up" ), i18n( "&Previous" ), this );
    connect( m_previousAction, SIGNAL( triggered() ), this, SLOT( slotPrevious() ) );

    toolbar->addAction( m_nextAction );
    toolbar->addAction( m_previousAction );

    m_nextAction->setEnabled( false );
    m_previousAction->setEnabled( false );

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

    KAction * showOnlyMatchesAction = new KAction( i18n( "Show only matches" ), this );
    showOnlyMatchesAction->setCheckable( true );
    connect( showOnlyMatchesAction, SIGNAL( toggled( bool ) ), this, SLOT( slotShowOnlyMatches( bool ) ) );

    showOnlyMatchesAction->setChecked( m_showOnlyMatches );
    m_menu->addAction( showOnlyMatchesAction );

    m_nextAction->setVisible( !m_showOnlyMatches );
    m_previousAction->setVisible( !m_showOnlyMatches );

    KAction *searchMenuAction = new KAction( KIcon( "preferences-other" ), i18n( "Search Preferences" ), this );
    searchMenuAction->setMenu( m_menu );
    toolbar->addAction( searchMenuAction );

    QToolButton *tbutton = qobject_cast<QToolButton*>(toolbar->widgetForAction( searchMenuAction ) );
    if( tbutton )
        tbutton->setPopupMode( QToolButton::InstantPopup );

    toolbar->setFixedHeight( m_searchEdit->sizeHint().height() );

    //make sure that this edit is cleared when the playlist is cleared:
    connect( Amarok::actionCollection()->action( "playlist_clear" ), SIGNAL( triggered() ), this, SLOT( slotFilterClear() ) );
}


ProgressiveSearchWidget::~ProgressiveSearchWidget()
{}

void ProgressiveSearchWidget::slotFilterChanged( const QString & filter )
{
    DEBUG_BLOCK

    //when the clear button is pressed, we get 2 calls to this slot... filter this out  as it messes with
    //resetting the view:
    if ( filter == m_lastFilter )
        return;

    debug() << "New filter: " << filter;

    m_lastFilter = filter;

    if( filter.isEmpty() )
    {
        m_nextAction->setEnabled( false );
        m_previousAction->setEnabled( false );

        QPalette p = m_searchEdit->palette();
        p.setColor( QPalette::Base, palette().color( QPalette::Base ) );
        m_searchEdit->setPalette( p );

        if( m_showOnlyMatches )
            hideHiddenTracksWarning();

        emit( filterCleared() );

        return;
    }

    emit( filterChanged( filter, m_searchFieldsMask, m_showOnlyMatches ) );
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

    if( m_showOnlyMatches )
        hideHiddenTracksWarning();
}

void ProgressiveSearchWidget::noMatch()
{
    m_nextAction->setEnabled( false );
    m_previousAction->setEnabled( false );

    const KStatefulBrush backgroundBrush( KColorScheme::View, KColorScheme::NegativeBackground );

    QPalette p = m_searchEdit->palette();
    p.setColor( QPalette::Base, backgroundBrush.brush( m_searchEdit ).color() );
    m_searchEdit->setPalette( p );

    if( m_showOnlyMatches )
        showHiddenTracksWarning();
}

void ProgressiveSearchWidget::showHiddenTracksWarning()
{
    m_warningLabel->show();
}

void ProgressiveSearchWidget::hideHiddenTracksWarning()
{
    m_warningLabel->hide();
}

void ProgressiveSearchWidget::slotSearchTracks( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchTrack;
    else
        m_searchFieldsMask ^= Playlist::MatchTrack;

    Amarok::config( "Playlist Search" ).writeEntry( "MatchTrack", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask, m_showOnlyMatches ) );
}

void ProgressiveSearchWidget::slotSearchArtists( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchArtist;
    else
        m_searchFieldsMask ^= Playlist::MatchArtist;

    Amarok::config( "Playlist Search" ).writeEntry( "MatchArtist", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask, m_showOnlyMatches ) );
}

void ProgressiveSearchWidget::slotSearchAlbums( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchAlbum;
    else
        m_searchFieldsMask ^= Playlist::MatchAlbum;

    Amarok::config( "Playlist Search" ).writeEntry( "MatchAlbum", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask, m_showOnlyMatches ) );
}

void ProgressiveSearchWidget::slotSearchGenre( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchGenre;
    else
        m_searchFieldsMask ^= Playlist::MatchGenre;

    Amarok::config( "Playlist Search" ).writeEntry( "MatchGenre", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask, m_showOnlyMatches ) );
}

void ProgressiveSearchWidget::slotSearchComposers( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchComposer;
    else
        m_searchFieldsMask ^= Playlist::MatchComposer;

    Amarok::config( "Playlist Search" ).writeEntry( "MatchComposer", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask, m_showOnlyMatches ) );
}

void ProgressiveSearchWidget::slotSearchYears( bool search )
{
    if( search )
        m_searchFieldsMask |= Playlist::MatchYear;
    else
        m_searchFieldsMask ^= Playlist::MatchYear;

    Amarok::config( "Playlist Search" ).writeEntry( "MatchYear", search );

    if( !m_searchEdit->text().isEmpty() )
        emit( filterChanged( m_searchEdit->text(), m_searchFieldsMask, m_showOnlyMatches ) );
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

    m_showOnlyMatches = config.readEntry( "ShowOnlyMatches", false );
}

void ProgressiveSearchWidget::slotShowOnlyMatches( bool onlyMatches )
{
    KConfigGroup config = Amarok::config( "Playlist Search" );
    config.writeEntry( "ShowOnlyMatches", onlyMatches );

    m_showOnlyMatches = onlyMatches;

    m_nextAction->setVisible( !onlyMatches );
    m_previousAction->setVisible( !onlyMatches );

    emit( showOnlyMatches( onlyMatches ) );
}

void
ProgressiveSearchWidget::keyPressEvent( QKeyEvent *event )
{
    if( event->matches( QKeySequence::FindNext ) )
    {
        event->accept();
        slotNext();
    }
    else if( event->matches( QKeySequence::FindPrevious ) )
    {
        event->accept();
        slotPrevious();
    }
    else
    {
        event->ignore();
        KHBox::keyPressEvent( event );
    }
}

void ProgressiveSearchWidget::slotFilterClear()
{
    m_searchEdit->setText( QString() );
}

}   //namespace Playlist

#include "ProgressiveSearchWidget.moc"
