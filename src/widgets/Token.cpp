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

#include <QHBoxLayout>

Token*
TokenBuilder::buildToken( const QString &element ) const 
{
    if( element == Token::tokenElement( Token::Track ) )
        return new Token( Token::Track );
    if( element == Token::tokenElement( Token::Title ) )
        return new Token( Token::Title );
    if( element == Token::tokenElement( Token::Artist ) )
        return new Token( Token::Artist );
    if( element == Token::tokenElement( Token::Composer ) )
        return new Token( Token::Composer );
    if( element == Token::tokenElement( Token::Year ) )
        return new Token( Token::Year );
    if( element == Token::tokenElement( Token::Album ) )
        return new Token( Token::Album );
    if( element == Token::tokenElement( Token::Comment ) )
        return new Token( Token::Comment );
    if( element == Token::tokenElement( Token::Genre ) )
        return new Token( Token::Genre );
    if( element == Token::tokenElement( Token::FileType ) )
        return new Token( Token::Folder );
    if( element == Token::tokenElement( Token::Initial ) )
        return new Token( Token::Initial );
    if( element == Token::tokenElement( Token::DiscNumber ) )
        return new Token( Token::DiscNumber );
    if( element == Token::tokenElement( Token::Space ) )
        return new Token( Token::Space );
    if( element == Token::tokenElement( Token::Slash ) )
        return new Token( Token::Slash );
    if( element == Token::tokenElement( Token::Dot ) )
        return new Token( Token::Dot );
    if( element == Token::tokenElement( Token::Dash ) )
        return new Token( Token::Dash );
    if( element == Token::tokenElement( Token::Underscore ) )
        return new Token( Token::Underscore );

    warning() << "Could not build token for element string: " << element;
    return 0;
}

Token::Token( Type type, QWidget *parent )
    : QFrame( parent )
    , m_type( type )
{
    m_label = new QLabel( this );
    m_label->setAlignment( Qt::AlignCenter );
    m_label->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_label->setText( prettyText() );

    QHBoxLayout *hlayout = new QHBoxLayout( this );
    setLayout( hlayout );
    
    m_iconContainer = new QLabel( this );
    m_iconContainer->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
    setIcon();

    setContentsMargins( 0, 0, 0, 0 );

    hlayout->setContentsMargins( 0, 0, 0, 0 );
    hlayout->addWidget( m_iconContainer );
    hlayout->addWidget( m_label );
    
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

QString
Token::text() const
{
    switch( m_type )
    {
        case Unknown:
            return i18n( "Unknown" );
        case Ignore:
            return i18n( "Ignore field" );
        case Track:
            return i18n( "Track" );
        case Title:
            return i18n( "Title" );
        case Artist:
            return i18n( "Artist" );
        case Composer:
            return i18n( "Composer" );
        case Year:
            return i18n( "Year" );
        case Album:
            return i18n( "Album" );
        case Comment:
            return i18n( "Comment" );
        case Genre:
            return i18n( "Genre" );
        case FileType:
            return i18n( "File type" );
        case Folder:
            return i18n( "Collection root" );
        case Initial:
            return i18n( "Artist initial" );
        case DiscNumber:
            return i18n( "Disc number" );
        case Space:
            return i18n( "<space>" );
        case Slash:
            return QString( "/" );
        case Dot:
            return QString( "." );
        case Dash:
            return QString( "-" );
        case Underscore:
            return QString( "_" );
    }
    return QString();
} 

QString
Token::prettyText() const
{
    switch( m_type )
    {
        case Space:
            return QString(" ");
        default:
            return text();
    }
}

QString
Token::tokenElement() const
{
    return tokenElement( m_type );
}

// Static. Accessed by the builder when creating an object
QString
Token::tokenElement( Type type )
{
    switch( type )
    {
        case Unknown:
            return QString();
        case Ignore:
            return QString( "%ignore" );
        case Track:
            return QString( "%track" );
        case Title:
            return QString( "%title" );
        case Artist:
            return QString( "%artist" );
        case Composer:
            return QString( "%composer" );
        case Year:
            return QString( "%year" );
        case Album:
            return QString( "%album" );
        case Comment:
            return QString( "%comment" );
        case Genre:
            return QString( "%genre" );
        case FileType:
            return QString( "%filetype" );
        case Folder:
            return QString( "%folder" );
        case Initial:
            return QString( "%initial" );
        case DiscNumber:
            return QString( "%discnumber" );
        case Space:
            return QString( " " );
        case Slash:
            return QString( "/" );
        case Dot:
            return QString( "." );
        case Dash:
            return QString( "-" );
        case Underscore:
            return QString( "_" );
    }
    return QString();
} 

QString
Token::iconName() const
{
    switch( m_type )
    {
        case Space:
            return QString( "space" );
        case Slash:
            return QString( "slash" );
        case Dot:
            return QString( "dot" );
        case Dash:
            return QString( "dash" );
        case Underscore:
            return QString( "underscore" );
        default:
            return tokenElement().mid( 1 );// remove '%'
    }
    return QString();
}

void
Token::setIcon()
{
    QPixmap pixmap = QPixmap( KIcon( "filename-" + iconName() + "-amarok" ).pixmap( 16, 16 ) );
    m_iconContainer->setPixmap( pixmap );
}
