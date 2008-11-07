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
	};
}

#endif 
