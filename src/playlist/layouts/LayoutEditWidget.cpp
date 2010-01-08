/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "LayoutEditWidget.h"
#include "TokenDropTarget.h"
#include "playlist/PlaylistDefines.h"

#include "Debug.h"

#include <KHBox>
#include <KMessageBox>

#include <QCheckBox>
#include <QSpinBox>

using namespace Playlist;

LayoutEditWidget::LayoutEditWidget( QWidget *parent )
    : KVBox(parent)
{
    m_tokenFactory = new TokenWithLayoutFactory;
    m_dragstack = new TokenDropTarget( "application/x-amarok-tag-token", this );
    m_dragstack->setCustomTokenFactory( m_tokenFactory );
    connect ( m_dragstack, SIGNAL( focusReceived(QWidget*) ), this, SIGNAL( focusReceived(QWidget*) ) );
    connect ( m_dragstack, SIGNAL( changed() ), this, SIGNAL( changed() ) );

    m_showCoverCheckBox = new QCheckBox( i18n( "Show cover" ) , this );
}


LayoutEditWidget::~LayoutEditWidget()
{
//     delete m_tokenFactory; m_tokenFactory = 0;
}

void LayoutEditWidget::readLayout( Playlist::LayoutItemConfig config )
{
    DEBUG_BLOCK
    int rowCount = config.rows();

    delete m_showCoverCheckBox;
    m_showCoverCheckBox = new QCheckBox( i18n( "Show cover" ) , this );
    m_showCoverCheckBox->setChecked( config.showCover() );
    connect ( m_showCoverCheckBox, SIGNAL( stateChanged( int ) ), this, SIGNAL( changed() ) );

    m_dragstack->clear();

    for( int i = 0; i < rowCount; i++ )
    {
        //get the row config
        Playlist::LayoutItemConfigRow rowConfig = config.row( i );

        int elementCount = rowConfig.count();

        //FIXME! for now, each element get the same size. This needs extensions to the token stuff
        //qreal size = 1.0 / (qreal) elementCount;

        for( int j = 0; j < elementCount; j++ )
        {
            Playlist::LayoutItemConfigRowElement element = rowConfig.element( j );
            //check if the element has a valid value, will crash if trying to use it otherwise
            debug() << "value: " << element.value();
            if ( element.value()  == -1 )
            {

                error() << "Invalid element value '" << element.value() << "' in playlist layout.";
                KMessageBox::detailedError( this,
                                            i18n( "Invalid playlist layout." ),
                                            i18n( "Encountered an unknown element name while reading layout." ) );
                m_dragstack->clear();
                return;

            }

            TokenWithLayout *token =  new TokenWithLayout( columnNames[element.value()], iconNames[element.value()], element.value() );
            token->setBold( element.bold() );
            token->setItalic( element.italic() );
            token->setUnderline( element.underline() );
            token->setAlignment( element.alignment() );
            token->setWidth( element.size() * 100.0 );
            token->setPrefix( element.prefix() );
            token->setSuffix( element.suffix() );
            m_dragstack->insertToken( token, i, j );
            // Do all modifications on the token above that line, otherwise the dialog will think it's been modified by the user
            connect ( token, SIGNAL( changed() ), this, SIGNAL( changed() ) );
        }

    }
}

Playlist::LayoutItemConfig LayoutEditWidget::config()
{

    LayoutItemConfig config;
    config.setShowCover( m_showCoverCheckBox->isChecked() );

    int noOfRows = m_dragstack->rows();

    for( int i = 0; i < noOfRows; i++ )
    {

        LayoutItemConfigRow currentRowConfig;

        QList<Token *> tokens = m_dragstack->drags( i );

        foreach( Token * token, tokens ) {
            if ( TokenWithLayout *twl = dynamic_cast<TokenWithLayout *>( token ) )
            {
                qreal width = 0.0;
                if ( twl->widthForced() && twl->width() > 0.01) {
                    width = twl->width();
                }
                currentRowConfig.addElement( LayoutItemConfigRowElement( twl->value(), width,
                                                                         twl->bold(), twl->italic(), twl->underline(),
                                                                         twl->alignment(), twl->prefix(), twl->suffix() ) );
            }
        }

        config.addRow( currentRowConfig );

    }
    return config;
}

void LayoutEditWidget::clear()
{
    m_dragstack->clear();
}

#include "LayoutEditWidget.moc"

