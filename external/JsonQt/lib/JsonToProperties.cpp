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
