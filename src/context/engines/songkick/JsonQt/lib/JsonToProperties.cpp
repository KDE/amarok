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
#include "JsonToProperties.h"

#include "JsonToVariant.h"

#include <QDebug>
#include <QMetaObject>
#include <QMetaProperty>

namespace JsonQt
{
	JsonToProperties::JsonToProperties()
	{
	}

	void JsonToProperties::parse(const QString& json, QObject* object)
		throw(ParseException)
	{
		QVariantMap dataMap = JsonToVariant::parse(json).toMap();

		const QMetaObject* meta = object->metaObject();
		for(
			int i = 0;
			i < meta->propertyCount();
			++i
		)
		{
			QMetaProperty property = meta->property(i);
			if(dataMap.contains(property.name()))
			{
				QVariant data = dataMap[property.name()];
				if(data.canConvert(property.type()))
					property.write(object, data);
				else
					qDebug() << QObject::tr("Found property %1 with incompatible data type.").arg(property.name());
			}
		}
	}
};
