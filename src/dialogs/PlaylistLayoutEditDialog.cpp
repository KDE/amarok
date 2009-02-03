/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "PlaylistLayoutEditDialog.h"

#include "Debug.h"

#include "playlist/view/listview/LayoutManager.h"
#include "playlist/PlaylistDefines.h"

#include <KMessageBox>

using namespace Playlist;

PlaylistLayoutEditDialog::PlaylistLayoutEditDialog( QWidget *parent )
    : QDialog( parent )
{
    setupUi( this );

    tokenPool->addToken( new Token( columnNames[Album], iconNames[Album], Album ) );
    tokenPool->addToken( new Token( columnNames[AlbumArtist], iconNames[AlbumArtist], AlbumArtist ) );
    tokenPool->addToken( new Token( columnNames[Artist], iconNames[Artist], Artist ) );
    tokenPool->addToken( new Token( columnNames[Bitrate], iconNames[Bitrate], Bitrate ) );
    tokenPool->addToken( new Token( columnNames[Comment], iconNames[Comment], Comment ) );
    tokenPool->addToken( new Token( columnNames[Composer], iconNames[Composer], Composer ) );
    tokenPool->addToken( new Token( columnNames[DiscNumber], iconNames[DiscNumber], DiscNumber ) );
    tokenPool->addToken( new Token( columnNames[Directory], iconNames[Directory], Directory ) );
    tokenPool->addToken( new Token( columnNames[Filename], iconNames[Filename], Filename ) );
    tokenPool->addToken( new Token( columnNames[Filesize], iconNames[Filesize], Filesize ) );
    tokenPool->addToken( new Token( columnNames[GroupLength], iconNames[GroupLength], GroupLength ) );
    tokenPool->addToken( new Token( columnNames[GroupTracks], iconNames[GroupTracks], GroupTracks ) );
    tokenPool->addToken( new Token( columnNames[Length], iconNames[Length], Length ) );
    tokenPool->addToken( new Token( columnNames[PlaceHolder], iconNames[PlaceHolder], PlaceHolder ) );
    tokenPool->addToken( new Token( columnNames[PlayCount], iconNames[PlayCount], PlayCount ) );
    tokenPool->addToken( new Token( columnNames[Rating], iconNames[Rating], Rating ) );
    tokenPool->addToken( new Token( columnNames[Score], iconNames[Score], Score ) );
    tokenPool->addToken( new Token( columnNames[Source], iconNames[Source], Source ) );
    tokenPool->addToken( new Token( columnNames[Title], iconNames[Title], Title ) );
    tokenPool->addToken( new Token( columnNames[TitleWithTrackNum], iconNames[TitleWithTrackNum], TitleWithTrackNum ) );
    tokenPool->addToken( new Token( columnNames[TrackNumber], iconNames[TrackNumber], TrackNumber ) );
    tokenPool->addToken( new Token( columnNames[Type], iconNames[Type], Type ) );
    tokenPool->addToken( new Token( columnNames[Year], iconNames[Year], Year ) );


    //add an editor to each tab
    m_headEdit = new PlaylistItemEditWidget( 0 );
    m_bodyEdit = new PlaylistItemEditWidget( 0 );
    m_singleEdit = new PlaylistItemEditWidget( 0 );

    elementTabs->addTab( m_headEdit, i18n( "Head" ) );
    elementTabs->addTab( m_bodyEdit, i18n( "Body" ) );
    elementTabs->addTab( m_singleEdit, i18n( "Single" ) );

    elementTabs->removeTab( 0 );

    connect( previewButton, SIGNAL( clicked() ), this, SLOT( preview() ) );

}


PlaylistLayoutEditDialog::~PlaylistLayoutEditDialog()
{
}

void PlaylistLayoutEditDialog::setLayout( const QString &layoutName )
{
    m_layoutName = layoutName;
    nameEdit->setText( layoutName );

    PlaylistLayout layout = LayoutManager::instance()->layout( layoutName );
    m_headEdit->readLayout( layout.head() );
    m_bodyEdit->readLayout( layout.body() );
    m_singleEdit->readLayout( layout.single() );
}

void PlaylistLayoutEditDialog::preview()
{

    PlaylistLayout layout;

    PrettyItemConfig headConfig = m_headEdit->config() ;
    headConfig.setActiveIndicatorRow( -1 );
    
    layout.setHead( headConfig );
    layout.setBody( m_bodyEdit->config() );
    layout.setSingle( m_singleEdit->config() );

    LayoutManager::instance()->setPreviewLayout( layout );
    
}

void PlaylistLayoutEditDialog::accept()
{
    DEBUG_BLOCK

    if ( LayoutManager::instance()->isDefaultLayout( nameEdit->text() ) ) {
        KMessageBox::sorry( this,
                            i18n( "The layout '%1' is one of the default layouts and cannot be overwritten. Please select a different name.", nameEdit->text() ), i18n( "Reserved Layout Name" ) );
        return;
    }
            
    PlaylistLayout layout;

    PrettyItemConfig headConfig = m_headEdit->config();
    headConfig.setActiveIndicatorRow( -1 );
    
    
    layout.setHead( headConfig );
    layout.setBody( m_bodyEdit->config() );
    layout.setSingle( m_singleEdit->config() );


    
    LayoutManager::instance()->addUserLayout( nameEdit->text(), layout);
    QDialog::accept();
}


