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
