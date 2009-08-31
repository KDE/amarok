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

#ifndef _JSONQT_JSON_TO_VARIANT_H
#define _JSONQT_JSON_TO_VARIANT_H

#include "ParseException.h"

#include "JsonQtExport.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QVariant>

/** Qt-based JSON handling.
 * This is based on a recursive descent parser with 1-character lookahed,
 * and follows the structure of the grammar presented on json.org as closely
 * as possible, to avoid mistakes, and hopefully to make the code more readable.
 *
 * @author Fred Emmott <mail@fredemmott.co.uk>
 */
namespace JsonQt
{
	/** Class for converting JSON strings to QVariant-based structures.
	 *
	 * @author Fred Emmott <mail@fredemmott.co.uk>
	 */
	class JSONQT_EXPORT JsonToVariant
	{
		public:
			/** Main parsing function.
			 *
			 * @param json is a string containing JSON text.
			 * @returns A QVariant-representation of the JSON
			 * 	structure.
			 * @throws ParseException if the string provided is not
			 * 	valid JSON (or at least this parser thinks it
			 * 	isn't ;) )
			 */
			static QVariant parse(const QString& json) throw (ParseException);

			/** Parse multiple objects in one string.
			 * This is useful when working on streams where
			 * one-chunk-per-json-object is not guaranteed.
			 */
			static QList<QVariantMap> multiParse(const QString& json) throw(ParseException);
		private:
			JsonToVariant();
			// Parsers for types given on JSON.org
			QVariantMap parseObject();
			QVariantMap parseMembers();
			QPair<QString, QVariant> parsePair();
			QVariantList parseArray();
			QVariantList parseElements();
			QVariant parseValue();
			QString parseString();
			QString parseChars();
			QChar parseChar();
			QVariant parseNumber();
			QString parseInt();
			QString parseFrac();
			QString parseExp();
			QString parseDigits();
			QString parseE();

			// Parsers for types implied on JSON.org
			QChar parseDigit();
			QChar parseHexDigit();
			bool parseBool();
			QVariant parseNull();

			// Internal functions
			/// The unparsed part of the input string.
			inline QString remaining();

			/** Consume the next character.
			 * Advances m_sym to the next character, and returns it.
			 * Optionally skips over whitespace.
			 *
			 * @param skipWhitespace controls if whitespace is
			 * 	ignored.
			 * @returns *m_sym
			 * @throws ParseException if the end of the string is
			 *	reached.
			 */
			QChar consume(bool skipWhitespace = true)
				throw(ParseException);

			/** Consume the next character, and check for equality
			 * with the specified character.
			 *
			 * @param wanted is the character to compare to.
			 * @throws ParseException if the end of the string is
			 * 	reached, or the characaters are not equal.
			 */
			void consume(QChar wanted) throw(ParseException);

			/// Convenience function for consume(QChar).
			void consume(char wanted) throw(ParseException);

			/** Attempt to consume the specified string.
			 * This attempts to consume length(wanted) characters,
			 * including whitespace, and checks that they are equal
			 * to the characters in the same position in the
			 * specified string.
			 *
			 * @param wanted is the string to attempt to consume.
			 * @throws ParseException if the end of the string is
			 * 	reached, or the string comparisson fails.
			 */
			void consume(QString wanted) throw(ParseException);

			/** Try to consume a single character, without throw.
			 * This will try to read a single character, and
			 * compares to the specified character.
			 *
			 * @param wanted is the character to compare to.
			 * @returns true if the character specified was
			 * 	successfully consumed.
			 * @returns false if the end of the string was reached,
			 * 	or the characters were not equal.
			 */
			bool tryConsume(QChar wanted) throw();

			/** Return the next symbol.
			 * Optionally skips whitespace.
			 * 
			 * @param skipWhitespace sets the whitespace handling.
			 * @returns the next symbol.
			 * @throws ParseException if the end of the string is
			 * 	reached.
			 */
			QChar peekNext(bool skipWhitespace = true)
				throw(ParseException);

			// Variables

			/// Iterator pointing at the current symbol.
			QString::ConstIterator m_sym;
			/// Iterator pointing at the next symbol.
			QString::ConstIterator m_next;
			/// Iterator pointing at the end of the string.
			QString::ConstIterator m_end;
	};
}

#endif
