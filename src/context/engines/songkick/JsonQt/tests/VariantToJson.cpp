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

#include "VariantToJson.h"
#include "JsonToVariant.h"

#include <QtTest/QtTest>

class VariantToJson : public QObject
{
	Q_OBJECT
	private slots:
		void testFlat()
		{
			QVariantMap map;
			map["string_foo"] = "foo";
			map["bool_true"] = true;
			map["int_42"] = 42;
			map["double_pi"] = 3.14159;

			QString json(JsonQt::VariantToJson::parse(map));
			QCOMPARE(map, JsonQt::JsonToVariant::parse(json).toMap());
		}
		void testComplex()
		{
			QVariantMap map;
			map["string_foo"] = "foo";
			map["bool_true"] = true;
			map["int_42"] = 42;
			map["double_pi"] = 3.14159;
			map["recurse"] = map;
			QVariantList list;
			list.append("zero");
			list.append("one");
			map["list"] = list;

			QStringList list2;
			for(int i = 0; i < 25; i++)
				list2.append(QString("element").append(QString::number(i)));
			map["list2"] = list2;

			QString json(JsonQt::VariantToJson::parse(map));
			QCOMPARE(map, JsonQt::JsonToVariant::parse(json).toMap());
		}
};

QTEST_MAIN(VariantToJson);

#include "moc_VariantToJson.cxx"
