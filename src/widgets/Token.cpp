/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#include "Token.h"
#include "FilenameLayoutWidget.h"
#include "Debug.h"

#include <KColorScheme>
#include <KIcon>

Token::Token( const QString &string, QWidget *parent )
    : QFrame( parent )
{
    m_myCount = qobject_cast< FilenameLayoutWidget * >( parent )->getTokenCount();
    m_label = new QLabel( this );
    hlayout = new QHBoxLayout( this );
    setLayout( hlayout );
    m_iconContainer = new QLabel( this );
    
    setContentsMargins( 0, 0, 0, 0 );
    hlayout->setContentsMargins( 0, 0, 0, 0 );
    hlayout->addWidget( m_iconContainer );
    hlayout->addWidget( m_label );
    setString( string );
    m_label->setAlignment( Qt::AlignCenter );

    const uint borderColor = static_cast<uint>( KColorScheme( QPalette::Active ).decoration( KColorScheme::HoverColor ).color().rgb() );
    const QString hexBorder = QString::number( borderColor, 16 ).remove( 0, 2 ); // remove the Alpha channel (first two hex bits)
    
    setStyleSheet( "Token {\
        color: palette( Base );\
        border: 2px solid #" + hexBorder + ";\
        border-radius: 4px;\
        padding: 2px;\
    }" );

    QFontMetrics metric( font() );
    QSize size = metric.size( Qt::TextSingleLine, m_label->text() );
    QSizePolicy sizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_label->setSizePolicy( sizePolicy );
    
    m_iconContainer->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred ) );
    setIcon();
}

//Access for m_tokenString, private.
void
Token::setString( const QString &string )
{
    m_label->setText( string );
    if( string == i18n( "Track" ) )
    {
        m_tokenString = "track";
    }
    else if( string == i18n( "Title" ) )
    {
        m_tokenString = "title";
    }
    else if( string == i18n( "Artist" ) )
    {
        m_tokenString = "artist";
    }
    else if( string == i18n( "Composer" ) )
    {
        m_tokenString = "composer";
    }
    else if( string == i18n( "Year" ) )
    {
        m_tokenString = "year";
    }
    else if( string == i18n( "Album" ) )
    {
        m_tokenString = "album";
    }
    else if( string == i18n( "Comment" ) )
    {
        m_tokenString = "comment";
    }
    else if( string == i18n( "Genre" ) )
    {
        m_tokenString = "genre";
    }
    else if( string == i18n( "File type" ) )
    {
        m_tokenString = "filetype";
    }
    else if( string == i18n( "Ignore field" ) )
    {
        m_tokenString = "ignore";
    }
    else if( string == i18n( "Collection root" ) )
    {
        m_tokenString = "folder";
    }
    else if( string == i18n( "Artist initial" ) )
    {
        m_tokenString = "initial";
    }
    else if( string == i18n( "Disc number" ) )
    {
        m_tokenString = "discnumber";
    }
    else if( string == i18n( "<space>" ) )
    {
        m_tokenString = "space";
        m_label->setText(" ");
    }
    else if( string == "/" )
        m_tokenString = "slash";
    else if( string == "." )
        m_tokenString = "dot";
    else if( string == "-" )
        m_tokenString = "dash";
    else if( string == "_" )
        m_tokenString = "underscore";
} 

//Access for m_tokenString, public.
QString
Token::getString()
{
    return m_tokenString;
}

QString
Token::getLabel()
{
    return m_label->text();
}

void
Token::setIcon()
{
    QPixmap pixmap = QPixmap( KIcon( "filename-" + m_tokenString + "-amarok" ).pixmap( 16, 16 ) );
    m_iconContainer->setPixmap( pixmap );
}

void //does this do any good?
Token::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED( event );
    hlayout->update(); //does this do any good?
}
