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

#ifndef _JSONQT_JSON_TO_PROPERTIES_H
#define _JSONQT_JSON_TO_PROPERTIES_H

#include "ParseException.h"
#include "JsonQtExport.h"

#include <QObject>
#include <QString>

namespace JsonQt
{
	/** Class for setting the properties of a QObject from a JSON string.
	 * Note that this only pays attention to basic types in the top level
	 * object in the JSON.
	 *
	 * @author Fred Emmott <mail@fredemmott.co.uk>
	 */
	class JSONQT_EXPORT JsonToProperties
	{
		public:
			/** Main parsing function.
			 *
			 * @param json is a string containing JSON text.
			 * @param object is an object with properties to be
			 * 	filled from JSON.
			 * @throws ParseException if the string provided is not
			 * 	valid JSON (or at least this parser thinks it
			 * 	isn't ;) )
			 */
			static void parse(const QString& json, QObject* object)
				throw(ParseException);
		private:
			JsonToProperties();
	};
}

#endif
