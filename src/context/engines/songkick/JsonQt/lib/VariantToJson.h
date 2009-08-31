/****************************************************************************************
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
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

#ifndef _JSONQT_VARIANT_TO_JSON_H
#define _JSONQT_VARIANT_TO_JSON_H

#include "JsonQtExport.h"

#include <QString>
#include <QStringList>
#include <QVariant>

namespace JsonQt
{
	/** Class for converting QVariants into JSON structures.
	 *
	 * The following variant types are supported:
	 * - QVariant::Bool
	 * - QVariant::String
	 * - QVariant::Double
	 * - QVariant::Int
	 * - QVariant::LongLong
	 * - QVariant::UInt
	 * - QVariant::ULongLong
	 * - QVariant::Invalid
	 * - QVariant::List             // JSON array
	 * - QVariant::Map              // JSON object
	 *
	 * @author Fred Emmott <mail@fredemmott.co.uk>
	 */
	class JSONQT_EXPORT VariantToJson
	{
		public:
			/// Parse a QVariant into JSON.
			static QString parse(const QVariantMap& data);
		private:
			static QString parseElement(const QVariant& element);
			static QString parseList(const QVariantList& list);
			static QString parseStringList(const QStringList& list);
	};
}

#endif 
