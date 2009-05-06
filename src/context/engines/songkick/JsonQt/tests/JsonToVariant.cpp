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
#include "JsonToVariant.h"

#include <QtTest/QtTest>

class JsonToVariant : public QObject
{
	Q_OBJECT;
	private slots:
		void initTestCase()
		{
			// Variable use of whitespace intentional
			QVariant data = JsonQt::JsonToVariant::parse(
				"{"
					"\"MyString\": \"foo\","
					"\"MyInt\" :123 ,"
					"\"MyBigNum\" : 42e10 , "
					"\"MyLongNum\" : 8589934592,"
					"\"MyFraction\" : 2.718,"
					"\"MyArray\" : [1, \"2\", {\"3\": 4}],"
					"\"MyObject\" : {\"a\" : 1, \"b\" : 2 },"
					"\"MyEmptyObject\" : { },"
					"\"MyBool\": true,"
					"\"END\" : [\"OF\", \"TEST\"]"
				"}"
			);
			QCOMPARE(data.type(), QVariant::Map);
			m_parsed = data.toMap();
		}

		void testStringWithEscapedForwardSlash()
		{
			QVariant data = JsonQt::JsonToVariant::parse("\"\\/\"");
			QCOMPARE(data.type(), QVariant::String);
			QCOMPARE(data.toString(), QString("/"));
		}
		
		void testMulti()
		{
			QList<QVariantMap> parsed = JsonQt::JsonToVariant::multiParse(
				"{"
					"\"foo\": \"bar\""
				"}"
				"{"
					"\"x\": \"y\""
				"}"
			);
			QCOMPARE(parsed.count(), 2);
			QCOMPARE(parsed.at(0).value("foo"), QVariant("bar"));
			QCOMPARE(parsed.at(1).value("x"), QVariant("y"));
		}

		void testEmpty()
		{
			bool thrown = false;
			try
			{
				JsonQt::JsonToVariant::parse("");
			}
			catch(JsonQt::ParseException e)
			{
				thrown = true;
			}
			QVERIFY(thrown);
		}

		void testString()
		{
			QVariant data = m_parsed["MyString"];
			QCOMPARE(data.type(), QVariant::String);
			QCOMPARE(data.toString(), QString("foo"));
			QCOMPARE(data, JsonQt::JsonToVariant::parse("\"foo\""));
		}

		void testInt()
		{
			QVariant data = m_parsed["MyInt"];
			QCOMPARE(data.type(), QVariant::Int);
			QCOMPARE(data.toInt(), 123);
			QCOMPARE(data, JsonQt::JsonToVariant::parse("123"));
		}

		void testBigNum()
		{
			QVariant data = m_parsed["MyBigNum"];
			QCOMPARE(data.type(), QVariant::Double);
			QCOMPARE(data.toDouble(), 42e10);
			QCOMPARE(data, JsonQt::JsonToVariant::parse("42e10"));
		}

		void testLongNum()
		{
			QVariant data = m_parsed["MyLongNum"];
			QCOMPARE(data.type(), QVariant::LongLong);
			QCOMPARE(data.value<qint64>(), Q_INT64_C(8589934592));
			QCOMPARE(data, JsonQt::JsonToVariant::parse("8589934592"));
		}

		void testFraction()
		{
			QVariant data = m_parsed["MyFraction"];
			QCOMPARE(data.type(), QVariant::Double);
			QCOMPARE(data.toDouble(), double(2.718));
			QCOMPARE(data, JsonQt::JsonToVariant::parse("2.718"));
		}

		void testArray()
		{
			QVariant root = m_parsed["MyArray"];
			QCOMPARE(root.type(), QVariant::List);

			QVariantList rootData = root.toList();
			QCOMPARE(rootData[0].type(), QVariant::Int);
			QCOMPARE(rootData[0].toInt(), 1);
			QCOMPARE(rootData[1].type(), QVariant::String);
			QCOMPARE(rootData[1].toString(), QString("2"));

			QCOMPARE(rootData[2].type(), QVariant::Map);
			QVariantMap childData = rootData[2].toMap();
			QCOMPARE(childData["3"].type(), QVariant::Int);
			QCOMPARE(childData["3"].toInt(), 4);

			QCOMPARE(root, JsonQt::JsonToVariant::parse("[1, \"2\", {\"3\": 4}]"));
		}

		void testObject()
		{
			QVariant root = m_parsed["MyObject"];
			QCOMPARE(root.type(), QVariant::Map);

			QVariantMap rootData = root.toMap();

			QCOMPARE(rootData["a"].toInt(), 1);
			QCOMPARE(rootData["b"].toInt(), 2);

			QCOMPARE(root, JsonQt::JsonToVariant::parse("{\"a\" : 1, \"b\" : 2 }"));
		}

		void testEmptyObject()
		{
			QVariant root = m_parsed["MyEmptyObject"];
			QCOMPARE(root.type(), QVariant::Map);
			QCOMPARE(root.value<QVariantMap>().size(), 0);
			QCOMPARE(root, JsonQt::JsonToVariant::parse("{}"));
		}
	private:
		QVariantMap m_parsed;
};

QTEST_MAIN(JsonToVariant);

#include "moc_JsonToVariant.cxx"
