/* LICENSE NOTICE
	Copyright (c) 2008, Frederick Emmott <mail@fredemmott.co.uk>

	Permission to use, copy, modify, and/or distribute this software for any
	purpose with or without fee is hereby granted, provided that the above
	copyright notice and this permission notice appear in all copies.

	THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
	WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
	ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
	WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
	ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
	OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
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
