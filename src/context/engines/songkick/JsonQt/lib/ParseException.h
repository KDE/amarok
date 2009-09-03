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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef _JSONQT_PARSE_EXCEPTION_H
#define _JSONQT_PARSE_EXCEPTION_H

#include "JsonQtExport.h"

#include <exception>
#include <QChar>
#include <QString>

namespace JsonQt
{
	/** Parsing exception class.
	 * Raised whenever JsonQt can't pass something it's been given, for
	 * whatever reason.
	 */
	class JSONQT_EXPORT ParseException : public std::exception
	{
		public:
			/** Create a ParseException.
			 * @param got is what was found in the string.
			 * @param expected is what the parser was expecting.
			 * @param remaining is what was left of the input.
			 */
			ParseException(
				const QString& got,
				const QString& expected,
				const QString& remaining
			) throw();
			~ParseException() throw();

			/// A single string providing information on the
			/// exception.
			virtual const char* what() const throw();

			/// What the parser found.
			QString got();
			/// What the parser was expecting.
			QString expected();
			/// The remaining unparsed input.
			QString remaining();
		private:
			QString m_got;
			QString m_expected;
			QString m_remaining;
	};
};

#endif
