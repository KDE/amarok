/****************************************************************************************
 * Copyright (c) 2005 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2011 Kevin Funk <krf@electrostorm.net>                                 *
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


#include "TimeLabel.h"

#include "amarokconfig.h"
#include "EngineController.h"

#include <KLocalizedString>

#include <QFontDatabase>
#include <QFontMetrics>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>

TimeLabel::TimeLabel(QWidget* parent)
    : QLabel( QStringLiteral(" 0:00:00 "), parent )
{
    setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
}

QSize
TimeLabel::sizeHint() const
{
    return fontMetrics().boundingRect( QLocale().negativeSign() + QLocale().toString( QTime( 0, 0, 0 ) ) ).size();
}

void
TimeLabel::setShowTime(bool showTime) {
    m_showTime = showTime;
    if( !showTime )
    {
        QLabel::setText( QStringLiteral("") );
    }
}

bool TimeLabel::showTime() const
{
    return m_showTime;
}

void
TimeLabel::setText(const QString& text)
{
    if( m_showTime )
        QLabel::setText( text );
}

