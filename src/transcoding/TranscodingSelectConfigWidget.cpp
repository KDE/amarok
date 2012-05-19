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

#include <KIcon>
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
    if( savedConfiguration.isValid() && !savedConfiguration.isJustCopy() )
        addItem( KIcon( "audio-x-generic" ), i18nc( "An option in combo box to always transcode; "
            "%1 are transcoding options", "Always (%1)", savedConfiguration.prettyName() ),
            DontChange );

    if( !savedConfiguration.isValid() )
        addItem( KIcon( "view-choose" ), i18n( "Ask before each transfer" ), DontChange );
    else
        addItem( KIcon( "view-choose" ), i18n( "Ask before each transfer" ), Forget );

    if( savedConfiguration.isValid() && savedConfiguration.isJustCopy() )
    {
        addItem( KIcon( "edit-copy" ), i18n( "Never" ), DontChange );
        setCurrentIndex( count() - 1 );
    }
    else
        addItem( KIcon( "edit-copy" ), i18n( "Never" ), JustCopy );

    m_passedChoice = savedConfiguration;
}

Configuration
SelectConfigWidget::currentChoice() const
{
    Configuration invalid( INVALID );
    if( currentIndex() < 0 )
        return invalid;
    Choice choice = Choice( itemData( currentIndex() ).toInt() );
    switch( choice )
    {
        case DontChange:
            return m_passedChoice;
        case JustCopy:
            return Configuration( JUST_COPY );
        case Forget:
            return invalid;
    }
    return invalid;
}

bool
SelectConfigWidget::hasChanged() const
{
    return currentIndex() < 0 || itemData( currentIndex() ).toInt() != DontChange;
}

#include "TranscodingSelectConfigWidget.moc"
