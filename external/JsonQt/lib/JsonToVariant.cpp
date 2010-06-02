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

#include "JsonToVariant.h"

#include <QDebug>

#define FAIL(x) throw ParseException(*m_sym, x, remaining())

namespace JsonQt
{
	JsonToVariant::JsonToVariant(){}

	QVariant JsonToVariant::parse(const QString& json) throw(ParseException)
	{
		JsonToVariant parser;
		// Store the start and end of the string
		parser.m_next = json.constBegin();
		parser.m_sym = parser.m_next;
		parser.m_end = json.constEnd();
		// Parse any JSON value.
		return parser.parseValue();
	}

	QList<QVariantMap> JsonToVariant::multiParse(const QString& raw) throw(ParseException)
	{
		QList<QVariantMap> objects;
		QString json(raw.trimmed());

		JsonToVariant parser;
		// Store the start and end of the string
		parser.m_next = json.constBegin();
		parser.m_sym = parser.m_next;
		parser.m_end = json.constEnd();
		// A JSON Object is the top-level item in the parse tree
		do
		{
			objects.append(parser.parseObject());
		}
		while(parser.m_next != parser.m_end && parser.m_sym != parser.m_end);
		return objects;
	}

	QVariantMap JsonToVariant::parseObject()
	{
		/*
		 * object
		 * 	{}
		 * 	{ members }
		 */

		QVariantMap data;

		consume('{');
		if(peekNext() != '}')
			data = parseMembers();
		consume('}');

		return data;
	}

	QVariantMap JsonToVariant::parseMembers()
	{
		/*
		 * members
		 * 	pair
		 * 	pair , members
		 */

		QVariantMap data;
		QPair<QString, QVariant> pair;

		// loop instead of recursing
		do
		{
			// Grab a pair
			pair = parsePair();

			// Store it in our data
			data[pair.first] = pair.second;
		}
		while(tryConsume(',')); // Loop if we've got a list separator

		return data;
	}

	QPair<QString, QVariant> JsonToVariant::parsePair()
	{
		/*
		 * pair
		 * 	string : value
		 */

		QString key = parseString();
		consume(':');
		QVariant value = parseValue();

		return qMakePair(key, value);
	}

	QVariantList JsonToVariant::parseArray()
	{
		/*
		 * array
		 * 	[]
		 * 	[ elements ]
		 */

		QVariantList data;

		consume('[');
		if(peekNext() != ']')
			data = parseElements();
		consume(']');

		return data;
	}

	QVariantList JsonToVariant::parseElements()
	{
		/*
		 * elements
		 * 	value
		 * 	value , elements
		 */
		QVariantList data;

		// loop instead of recursing
		do
		{
			// Grab a value
			data += parseValue();
		}
		while(tryConsume(',')); // repeat if we've got a list separator

		return data;
	}

	QVariant JsonToVariant::parseValue()
	{
		/*
		 * value
		 * 	string
		 * 	number
		 * 	object
		 * 	array
		 * 	bool
		 * 	null
		 */

		tryConsume(':');

		// Lookahead to work out the type of value
		switch(peekNext().toAscii())
		{
			case '"':
				return parseString();
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
				return parseNumber();
			case '{':
				return parseObject();
			case '[':
				return parseArray();
			case 't': // true
			case 'f': // false
				return parseBool();
			case 'n': // null
				return parseNull();
			default:
				FAIL(QObject::tr("string, number, object, array, bool, or null"));
		}
	}

	QString JsonToVariant::parseString()
	{
		/*
		 * string
		 * 	""
		 * 	" chars "
		 */

		QString data;

		// Starting quotation marks
		consume('"');

		// If it's a non-empty string, grab the contents
		if(*m_next != '"')
			data = parseChars();

		// Ending quotation marks
		consume('"');

		return data;
	}

	QString JsonToVariant::parseChars()
	{
		/*
		 * chars
		 * 	char
		 * 	char chars
		 */

		QString data;

		// chars contains at least one char
		data = parseChar();

		while(peekNext() != '"')
			data.append( parseChar() );
		return data;
	}

	QChar JsonToVariant::parseChar()
	{
		/*
		 * char
		 * 	any character except for ", \, or control characters
		 *	\"
		 *	\\
		 *	\/
		 *	\b
		 *	\f
		 *	\n
		 *	\r
		 *	\t
		 *	\u four-hex-digits
		 */

		// Grab the next character, without skipping whitespace
		consume(false);

		// We're not allowed unescaped quotation marks
		if(*m_sym == '"')
			FAIL(QObject::tr("Any unicode character except for \" or JSON escape sequences"));
		
		// But some escape sequences are allowed
		if(*m_sym == '\\')
		{
			QString digits;
			switch(consume().toAscii())
			{
				case '"':
					return '"';
				case '\\':
					return '\\';
				case '/':
					return '/';
				case 'b':
					return '\b';
				case 'f':
					return '\f';
				case 'n':
					return '\n';
				case 'r':
					return '\r';
				case 't':
					return '\t';
				case 'u':
					// Unicode 4-digit hex
					for(int i = 0; i < 4; ++i)
					{
						digits += parseHexDigit();
					}
					return QChar(digits.toInt(0, 16));
				default:
					FAIL("[\"\\/bfnrtu]");
			}
		}
		return *m_sym;
	}

	QVariant JsonToVariant::parseNumber()
	{
		/*
		 * number
		 * 	int
		 * 	int frac
		 * 	int exp
		 * 	int frac exp
		 */

		// Every JSON number starts with an int
		QString data = parseInt();

		// Lookahead for fractions and exponents
	
		// Do we have a fraction?
		if(*m_next == '.') data += parseFrac();
		// Do we have an exponent?
		if(*m_next == 'e' || *m_next == 'E') data += parseExp();

		// Try several return types...
		bool ok;
		QVariant ret;

		ret = data.toInt(&ok); if(ok) return ret;
		ret = data.toLongLong(&ok); if(ok) return ret;
		ret = data.toDouble(&ok); if(ok) return ret;

		// If this point is reached, don't know how to convert the string
		// to an integer.
		Q_ASSERT(false);
		return QVariant();
	}

	QString JsonToVariant::parseInt()
	{
		/*
		 * int
		 * 	digit
		 * 	digit1-9 digits
		 * 	- digit
		 * 	- digit1-9 digits
		 */

		QString data;

		// Match any negation mark
		if(tryConsume('-'))
			data = '-';

		// Grab the first digit...
		QChar firstDigit = parseDigit();
		data += firstDigit;
		// ...if it's not zero...
		if(firstDigit != '0')
		{
			// ... try and add more digits.
			try { data += parseDigits(); }
			catch(ParseException)
			{
				// Catch, as more digits are entirely optional
				// Roll back.
				m_next = m_sym--;
			}
		}
		return data;
	}

	QString JsonToVariant::parseFrac()
	{
		/*
		 * frac
		 * 	. digits
		 */

		consume('.');
		return QString(".%1").arg(parseDigits());
	}

	QString JsonToVariant::parseExp()
	{
		/*
		 * exp
		 * 	e digits
		 */

		QString data;
		data = parseE();
		data += parseDigits();
		return data;
	}

	QString JsonToVariant::parseDigits()
	{
		/*
		 * digits
		 * 	digit
		 * 	digit digits
		 */

		QString data;

		// Digits has at least one digit...
		data += parseDigit();

		// ... try and get some more
		// Loop instead of recurse
		Q_FOREVER
		{
			try { data += parseDigit(); }
			catch(ParseException)
			{
				m_next = m_sym--; // roll back
				break;
			}
		}
		return data;
	}

	QString JsonToVariant::parseE()
	{
		/*
		 * e
		 * 	e
		 * 	e+
		 * 	e-
		 * 	E
		 * 	E+
		 * 	E-
		 */

		// Try and grab an 'e' or 'E'
		if(consume(false).toLower() == 'e')
		{
			// digits in follow[e]
			if(m_next->isDigit())
				return "e";

			// Hopefully the next is a + or -
			// grab another chracter...
			consume(false);
			// If it's not + or -, fail
			if(*m_sym != '+' && *m_sym != '-')
				FAIL("+ | -");

			// Otherwise, return e[+-]
			return QString("e%1").arg(*m_sym);
		}
		else
			FAIL("e | E");
	}


	QChar JsonToVariant::parseDigit()
	{
		/*
		 * digit
		 * 	[0-9]
		 */

		if(!consume(false).isDigit())
			FAIL("[0-9]");
		return *m_sym;
	}

	QChar JsonToVariant::parseHexDigit()
	{
		/*
		 * hexdigit
		 * 	[0-9a-fA-F]
		 */

		QChar character = consume().toLower();
		if(character.isDigit() || (character >= 'a' && character <= 'f'))
			return character;
		FAIL("[0-9a-fA-F]");
	}

	bool JsonToVariant::parseBool()
	{
		/*
		 * bool
		 * 	true
		 * 	false
		 */

		switch(peekNext().toAscii())
		{
			case 't':
				consume(QString("true"));
				return true;
			case 'f':
				consume(QString("false"));
				return false;
			default:
				consume(false);
				FAIL("true | false");
		}
	}

	QVariant JsonToVariant::parseNull()
	{
		/*
		 * null
		 * 	null
		 */

		consume(QString("null"));
		return QVariant();
	}

	QString JsonToVariant::remaining()
	{
		QString data;

		QString::ConstIterator it = m_sym;
		while(it != m_end) data += *it++;

		return data;
	}

	QChar JsonToVariant::consume(bool skipWhitespace) throw(ParseException)
	{
		// Read a character...
		do
		{
			if(m_next == m_end)
			{
				throw ParseException("EOF", "symbol", remaining());
			}
			m_sym = m_next++;
		}
		//...and loop while we get whitespace, if it's being skipped
		while(skipWhitespace && m_sym->isSpace());

		// Just for convenience...
		return *m_sym;
	}

	void JsonToVariant::consume(QChar wanted) throw(ParseException)
	{
		// Grab a char(ignoring whitespace), and if it's not what's
		// expected, throw
		if(consume() != wanted)
		{
			FAIL(wanted);
		}
	}

	void JsonToVariant::consume(char wanted) throw(ParseException)
	{
		// Convenience function for the above
		consume(QChar(wanted));
	}

	void JsonToVariant::consume(QString wanted) throw(ParseException)
	{
		// Keep track of where we start...
		QString::ConstIterator it = m_sym;
		// Grab wanted.length() characters, and compare them to the
		// character in the appropriate position in the parameter
		for(int i = 0; i < wanted.length(); ++i)
			if(consume(false) != wanted[i])
			{
				// If it doesn't match, roll back, and throw a
				// parse exception
				m_sym = it;
				m_next = ++it;
				FAIL(wanted);
			}
	}

	bool JsonToVariant::tryConsume(QChar wanted) throw()
	{
		// Grab the next character
		try
		{
			consume();
		}
		catch(ParseException)
		{
			// End-Of-String, rollback and return false
			m_next = m_sym--;
			return false;
		}

		// Check if it's what we want
		if(*m_sym != wanted)
		{
			// nope, something else, rollback and return false
			m_next = m_sym--;
			return false;
		}
		return true;
	}

	QChar JsonToVariant::peekNext(bool skipWhitespace) throw(ParseException)
	{
		QString::ConstIterator it = m_sym;
		do
		{
			++it;
			if(it == m_end)
			{
				FAIL("symbol");
			}
		}
		while(skipWhitespace && it->isSpace());
		return *it;
	}
}
