/****************************************************************************************
 * Copyright (c) 2008 Frederick Emmott <mail@fredemmott.co.uk>                          *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ParseException.h"

// For QObject::tr
#include <QObject>

namespace JsonQt
{
	ParseException::ParseException(const QString& got, const QString& expected, const QString& remaining) throw()
		:
			m_got(got),
			m_expected(expected),
			m_remaining(remaining)
	{
	}

	ParseException::~ParseException() throw(){}

	const char* ParseException::what() const throw()
	{
		return qPrintable(QObject::tr("A parsing error occurred:\n\tGot: '%1'\n\tExpected: '%2'\n\tAt: '%3'").arg(m_got).arg(m_expected).arg(m_remaining));
	}

	QString ParseException::got() { return m_got; }
	QString ParseException::expected() { return m_expected; }
	QString ParseException::remaining() { return m_remaining; }
}
