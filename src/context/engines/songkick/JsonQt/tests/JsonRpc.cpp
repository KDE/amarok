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

#include "JsonRpc.h"
#include "JsonToVariant.h"

#include <QtTest/QtTest>
#include <QSignalSpy>

/// Check that JSON a gets error code b
#define ERROR_TEST(a, b) \
	JsonQt::JsonRpc t; \
	QSignalSpy jsonSpy(&t, SIGNAL(sendJson(const QString&))); \
	t.processJson(a); \
\
	QCOMPARE(jsonSpy.count(), 1); \
\
	QString json(jsonSpy.first().first().toString()); \
	QVariantMap response = JsonQt::JsonToVariant::parse(json).toMap(); \
\
	QVERIFY(response.contains("error")); \
	QCOMPARE(response.value("error").type(), QVariant::Map); \
\
	QVariantMap error = response.value("error").toMap(); \
	QVERIFY(error.contains("code")); \
	QVERIFY(error.contains("message")); \
\
	QCOMPARE(error.value("code").type(), QVariant::Int); \
	QCOMPARE(error.value("code").toInt(), static_cast<int>(b));

/// Get a QVariantMap from spy
#define GET_DATA() \
	QCOMPARE(spy.count(), 1); \
	QCOMPARE(spy.first().first().type(), QVariant::String); \
	QVariantMap data(JsonQt::JsonToVariant::parse(spy.first().first().toString()).toMap());

/// Setup client and server JsonRpc instances
#define SETUP_CLIENT_SERVER() \
	JsonQt::JsonRpc client; \
	JsonQt::JsonRpc server; \
\
	connect( \
		&client, SIGNAL(sendJson(const QString&)), \
		&server, SLOT(processJson(const QString&)) \
	);

Q_DECLARE_METATYPE(QVariant);

class JsonRpc : public QObject
{
	Q_OBJECT
	private slots:
		void initTestCase()
		{
			qRegisterMetaType<QVariant>("QVariant");
		}

		void testInvalidJson()
		{
			ERROR_TEST("{", JsonQt::JsonRpc::InvalidJson);
		}

		void testEmptyJsonObject()
		{
			ERROR_TEST("{}", JsonQt::JsonRpc::InvalidJsonRpc);
		}

		void testSimpleNotificationReceived()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(notificationReceived(const QString&, const QVariant&)));
			t.processJson(
				"{"
				"\"jsonrpc\": \"2.0\","
				"\"method\": \"ping\""
				"}"
			);
			QCOMPARE(spy.count(), 1);
			QCOMPARE(spy.first().first().toString(), QString("ping"));
		}

		void testSimpleRequestReceived()
		{
			JsonQt::JsonRpc t;
			QSignalSpy requestSpy(&t, SIGNAL(requestReceived(const QVariant&, const QString&, const QVariant&)));
			t.processJson(
				"{"
				"\"jsonrpc\": \"2.0\","
				"\"method\": \"ping\","
				"\"id\": 9001"
				"}"
			);
			QCOMPARE(requestSpy.count(), 1);
			// method
			QCOMPARE(requestSpy.first().value(1).toString(), QString("ping"));
			// id
			QVariant id = requestSpy.first().value(0).value<QVariant>();
			QCOMPARE(id.type(), QVariant::Int);
			QCOMPARE(id.toInt(), 9001);
		}

		void testErrorReceived()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(errorReceived(const QVariant&, int, const QString&, const QVariant&)));
			t.processJson(
				"{"
				"\"jsonrpc\": \"2.0\","
				"\"error\":"
					"{"
						"\"code\": 1234,"
						"\"message\": \"foo\","
						"\"data\": \"bar\""
					"},"
				"\"id\": 9001"
				"}"
			);
			QCOMPARE(spy.count(), 1);

			// id
			QVariant id = spy.first().first().value<QVariant>();
			QCOMPARE(id.type(), QVariant::Int);
			QCOMPARE(id.toInt(), 9001);
			// code
			QCOMPARE(spy.first().value(1).type(), QVariant::Int);
			QCOMPARE(spy.first().value(1).toInt(), 1234);
			// message
			QCOMPARE(spy.first().value(2).type(), QVariant::String);
			QCOMPARE(spy.first().value(2).toString(), QString("foo"));
			// data
			QVariant data = spy.first().value(3).value<QVariant>();
			QCOMPARE(data.type(), QVariant::String);
			QCOMPARE(data.toString(), QString("bar"));
		}

		void testJsonRpcVersion()
		{
			QString testString = 
				"{"
				"\"jsonrpc\": \"9001\","
				"\"method\": \"ping\""
				"}";
			ERROR_TEST(testString, JsonQt::JsonRpc::InvalidJsonRpc);
		}

		void testComplexNotificationReceived()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(notificationReceived(const QString&, const QVariant&)));
			t.processJson(
				"{"
				"\"jsonrpc\": \"2.0\","
				"\"method\": \"ping\","
				"\"params\":"
					"{"
						"\"foo\": \"bar\","
						"\"123\": 456"
					"}"
				"}"
			);
			QCOMPARE(spy.count(), 1);

			QCOMPARE(spy.first().first().toString(), QString("ping"));
			QVariantMap parameters = spy.first().value(1).value<QVariant>().toMap();

			QVERIFY(parameters.contains("foo"));
			QCOMPARE(parameters.value("foo").type(), QVariant::String);
			QCOMPARE(parameters.value("foo").toString(), QString("bar"));
			QVERIFY(parameters.contains("123"));
			QCOMPARE(parameters.value("123").type(), QVariant::Int);
			QCOMPARE(parameters.value("123").toInt(), 456);
		}

		void testSendingSimpleNotification()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(sendJson(const QString&)));
			t.sendNotification("foo");
			GET_DATA();
			QCOMPARE(data.value("method").toString(), QString("foo"));
			QVERIFY(!data.contains("id"));
			QVERIFY(!data.contains("params"));
		}

		void testSendingSimpleRequest()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(sendJson(const QString&)));
			t.sendRequest(QVariant(123), "foo");
			GET_DATA();
			QCOMPARE(data.value("method").toString(), QString("foo"));
			QCOMPARE(data.value("id").toInt(), 123);
			QVERIFY(!data.contains("params"));
		}

		void testSendingResponse()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(sendJson(const QString&)));
			t.sendResponse(QVariant(123), "foo");
			GET_DATA();
			QCOMPARE(data.value("result").toString(), QString("foo"));
			QCOMPARE(data.value("id").toInt(), 123);
		}

		void testSendingError()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(sendJson(const QString&)));
			t.sendError(QVariant(123), 456, "foo", QVariant(789));
			GET_DATA();
			QCOMPARE(data.value("id").toInt(), 123);
			QCOMPARE(data.value("error").type(), QVariant::Map);
			QVariantMap error = data.value("error").toMap();
			QCOMPARE(error.value("code").toInt(), 456);
			QCOMPARE(error.value("message").toString(), QString("foo"));
			QCOMPARE(error.value("data").toInt(), 789);

		}

		void testSendingComplexNotification()
		{
			JsonQt::JsonRpc t;
			QSignalSpy spy(&t, SIGNAL(sendJson(const QString&)));
			QVariantMap params;
			params["bar"] = "baz";
			t.sendNotification("foo", params);
			GET_DATA();
			QCOMPARE(data.value("method").toString(), QString("foo"));
			QVERIFY(!data.contains("id"));
			QCOMPARE(data.value("params").toMap(), params);
		}

		void testNotificationBothEnds()
		{
			SETUP_CLIENT_SERVER();
			QSignalSpy spy(&server, SIGNAL(notificationReceived(const QString&, const QVariant&)));

			QVariantMap params;
			params["bar"] = "baz";

			client.sendNotification("foo", params);

			QCOMPARE(spy.count(), 1);
			QCOMPARE(spy.first().value(0).toString(), QString("foo"));
			QCOMPARE(spy.first().value(1).value<QVariant>().toMap(), params);
		}

		void testRequestBothEnds()
		{
			SETUP_CLIENT_SERVER();
			QSignalSpy spy(&server, SIGNAL(requestReceived(const QVariant&, const QString&, const QVariant&)));

			QVariantMap params;
			params["bar"] = "baz";

			client.sendRequest(123, "foo", params);

			QCOMPARE(spy.count(), 1);
			QCOMPARE(spy.first().value(0).value<QVariant>(), QVariant::fromValue<QVariant>(123));
			QCOMPARE(spy.first().value(1).toString(), QString("foo"));
			QCOMPARE(spy.first().value(2).value<QVariant>().toMap(), params);
		}

		void testResponseBothEnds()
		{
			SETUP_CLIENT_SERVER();
			QSignalSpy spy(&server, SIGNAL(responseReceived(const QVariant&, const QVariant&)));

			client.sendResponse(123, "foo");

			QCOMPARE(spy.count(), 1);
			QCOMPARE(spy.first().value(0).value<QVariant>(), QVariant::fromValue<QVariant>(123));
			QCOMPARE(spy.first().value(1).value<QVariant>().toString(), QString("foo"));
		}

		void testErrorBothEnds()
		{
			SETUP_CLIENT_SERVER();
			QSignalSpy spy(&server, SIGNAL(errorReceived(const QVariant&, int, const QString&, const QVariant&)));

			client.sendError(123, 456, "789", "foo");

			QCOMPARE(spy.count(), 1);
			QCOMPARE(spy.first().value(0).value<QVariant>(), QVariant::fromValue<QVariant>(123));
			QCOMPARE(spy.first().value(1).toInt(), 456);
			QCOMPARE(spy.first().value(2).toString(), QString("789"));
			QCOMPARE(spy.first().value(3).value<QVariant>().toString(), QString("foo"));
		}

		void testJsonErrorBothEnds()
		{
			SETUP_CLIENT_SERVER();
			QSignalSpy spy(&server, SIGNAL(errorReceived(const QVariant&, int, const QString&, const QVariant&)));
			client.processJson("{");
			QCOMPARE(spy.count(), 1);
			QCOMPARE(spy.first().value(1).toInt(), static_cast<int>(JsonQt::JsonRpc::InvalidJson));
		}
};

QTEST_MAIN(JsonRpc);

#include "moc_JsonRpc.cxx"
