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

#include "TranscodingSelectConfigWidget.h"

#include <QIcon>
#include <KLocalizedString>

using namespace Transcoding;


SelectConfigWidget::SelectConfigWidget( QWidget *parent )
    : QComboBox( parent )
    , m_passedChoice( INVALID )
{
}

void
SelectConfigWidget::fillInChoices( const Configuration &savedConfiguration )
{
    clear();
    addItem( QIcon::fromTheme( "edit-copy" ), i18n( "Never" ), JustCopy );
    addItem( QIcon::fromTheme( "view-choose" ), i18n( "Ask before each transfer" ), Invalid );
    if( savedConfiguration.isValid() )
    {
        if( !savedConfiguration.isJustCopy() )
        {
            Configuration temp = savedConfiguration;
            temp.setTrackSelection( Configuration::TranscodeAll );
            addItem( QIcon::fromTheme( "audio-x-generic" ), temp.prettyName(),
                    TranscodeAll );
            temp.setTrackSelection( Configuration::TranscodeUnlessSameType );
            addItem( QIcon::fromTheme( "audio-x-generic" ), temp.prettyName(),
                    TranscodeUnlessSameType );
            temp.setTrackSelection( Configuration::TranscodeOnlyIfNeeded );
            addItem( QIcon::fromTheme( "audio-x-generic" ),temp.prettyName(),
                    TranscodeOnlyIfNeeded );
            setCurrentIndex( savedConfiguration.trackSelection() + 2 );
        }
    }
    else
        setCurrentIndex( count() - 1 );

    m_passedChoice = savedConfiguration;
}

Configuration
SelectConfigWidget::currentChoice() const
{
    Configuration invalid( INVALID, m_passedChoice.trackSelection() );
    Configuration passedChoice = m_passedChoice;
    if( currentIndex() < 0 )
        return invalid;
    Choice choice = Choice( itemData( currentIndex() ).toInt() );
    switch( choice )
    {
        case JustCopy:
            return Configuration( JUST_COPY );
        case Invalid:
            return invalid;
        case TranscodeAll:
            passedChoice.setTrackSelection( Configuration::TranscodeAll );
            return passedChoice;
        case TranscodeUnlessSameType:
            passedChoice.setTrackSelection( Configuration::TranscodeUnlessSameType );
            return passedChoice;
        case TranscodeOnlyIfNeeded:
            passedChoice.setTrackSelection( Configuration::TranscodeOnlyIfNeeded );
            return passedChoice;
    }
    return invalid;
}

bool
SelectConfigWidget::hasChanged() const
{
    return currentIndex() < 0 || m_passedChoice != currentChoice();
}

