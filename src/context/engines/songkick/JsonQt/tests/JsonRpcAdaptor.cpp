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

#include "JsonRpcAdaptor.h"

#include "JsonRpc.h"

#include <QtTest/QtTest>

Q_DECLARE_METATYPE(QVariant);

class TestObject : public QObject
{
	Q_OBJECT;
	Q_CLASSINFO("JsonQt-RPC-name", "TestName");
	Q_CLASSINFO("JsonQt-RPC-id", "urn:data:test");
	Q_CLASSINFO("JsonQt-RPC-version", "9000.001");
	Q_CLASSINFO("JsonQt-RPC-summary", "Ponies");
	public:
		TestObject(QObject* parent) : QObject(parent) {}
	public slots:
		void functionOne(const QString& foo)
		{
			emit functionOneCalled(foo);
		}
		QString functionTwo()
		{
			emit functionTwoCalled();
			return "Mary had a little lamb.";
		}
		void functionThree(int one, bool two, QVariantList three, QVariantMap four)
		{
			emit functionThreeCalled(one, two, three, four);
		}
		void functionFour(const QString& foo, const QString& bar)
		{
			emit functionFourCalled(foo, bar);
		}
	signals:
		void functionOneCalled(const QString& foo);
		void functionTwoCalled();
		void functionThreeCalled(int, bool, QVariantList, QVariantMap);
		void functionFourCalled(QString, QString);
};

class JsonRpcAdaptor : public QObject
{
	Q_OBJECT
	private slots:
		void initTestCase()
		{
			qRegisterMetaType<QVariant>("QVariant");
			m_testObject = new TestObject(this);
			m_adaptor = new JsonQt::JsonRpcAdaptor(m_testObject, this);
			m_rpc = new JsonQt::JsonRpc(this);
			connect(
				m_rpc, SIGNAL(sendJson(const QString&)),
				m_adaptor, SLOT(processJson(const QString&))
			);
			connect(
				m_adaptor, SIGNAL(sendJson(const QString&)),
				m_rpc, SLOT(processJson(const QString&))
			);
		}

		void testFunctionOne()
		{
			QSignalSpy testObjectSpy(m_testObject, SIGNAL(functionOneCalled(const QString&)));
			QSignalSpy rpcSpy(m_rpc, SIGNAL(responseReceived(const QVariant&, const QVariant&)));

			QVariantMap parameters;
			parameters["foo"] = "bar";

			m_rpc->sendRequest(42, "functionOne", parameters);

			QCOMPARE(testObjectSpy.count(), 1);
			QCOMPARE(testObjectSpy.first().first().toString(), QString("bar"));
			QCOMPARE(rpcSpy.count(), 1);
			QCOMPARE(rpcSpy.first().value(0).value<QVariant>(), QVariant(42));
			QCOMPARE(rpcSpy.first().value(1).value<QVariant>(), QVariant());
		}

		void testFunctionFour()
		{
			QSignalSpy testObjectSpy(m_testObject, SIGNAL(functionFourCalled(QString, QString)));
			QVariantMap parameters;
			parameters["foo"] = "bar";
			parameters["bar"] = "baz";
			m_rpc->sendRequest(123, "functionFour", parameters);
			QCOMPARE(testObjectSpy.count(), 1);
			QCOMPARE(testObjectSpy.first().value(0).toString(), QString("bar"));
			QCOMPARE(testObjectSpy.first().value(1).toString(), QString("baz"));
		}

		void testFunctionTwo()
		{
			QSignalSpy testObjectSpy(m_testObject, SIGNAL(functionTwoCalled()));
			QSignalSpy rpcSpy(m_rpc, SIGNAL(responseReceived(const QVariant&, const QVariant&)));

			m_rpc->sendRequest(1337, "functionTwo");

			QCOMPARE(testObjectSpy.count(), 1);
			QCOMPARE(rpcSpy.count(), 1);
			QCOMPARE(rpcSpy.first().value(0).value<QVariant>(), QVariant(1337));
			QCOMPARE(rpcSpy.first().value(1).value<QVariant>().toString(), QString("Mary had a little lamb."));
		}

		void testFunctionThree()
		{
			QSignalSpy testObjectSpy(m_testObject, SIGNAL(functionThreeCalled(int, bool, QVariantList, QVariantMap)));
			QSignalSpy rpcSpy(m_rpc, SIGNAL(responseReceived(const QVariant&, const QVariant&)));

			QVariantList listParameter;
			QVariantMap nestedMap;
			nestedMap["foo"] = "bar";

			listParameter.append(nestedMap);
			listParameter.append("baz");

			QVariantList parameters;
			parameters.append(123);
			parameters.append(true);
			parameters.append(listParameter);
			parameters.append(nestedMap);

			m_rpc->sendRequest(42, "functionThree", parameters);

			QCOMPARE(testObjectSpy.count(), 1);
			QVariantList result = testObjectSpy.first();
			QCOMPARE(result.value(0), QVariant(123));
			QCOMPARE(result.value(1), QVariant(true));
			QCOMPARE(result.value(2), QVariant(listParameter));
			QCOMPARE(result.value(3), QVariant(nestedMap));

			QCOMPARE(rpcSpy.count(), 1);
			QCOMPARE(rpcSpy.first().value(0).value<QVariant>(), QVariant(42));
			QCOMPARE(rpcSpy.first().value(1).value<QVariant>(), QVariant());
		}

		void testInvalidFunction()
		{
			QSignalSpy rpcSpy(m_rpc, SIGNAL(errorReceived(QVariant, int, QString, QVariant)));
			m_rpc->sendRequest("123", "DOES NOT EXIST");
			QCOMPARE(rpcSpy.count(), 1);
			QCOMPARE(rpcSpy.first().value(0).value<QVariant>(), QVariant("123"));
			QCOMPARE(rpcSpy.first().value(1), QVariant(JsonQt::JsonRpc::MethodNotFound));
		}

		void testBadArgumentCount()
		{
			QSignalSpy rpcSpy(m_rpc, SIGNAL(errorReceived(QVariant, int, QString, QVariant)));
			m_rpc->sendRequest("123", "functionOne");
			QCOMPARE(rpcSpy.count(), 1);
			QCOMPARE(rpcSpy.first().value(0).value<QVariant>(), QVariant("123"));
			QCOMPARE(rpcSpy.first().value(1), QVariant(JsonQt::JsonRpc::BadParameters));
		}

		void testWrongArgumentTypes()
		{
			QSignalSpy rpcSpy(m_rpc, SIGNAL(errorReceived(QVariant, int, QString, QVariant)));
			QVariantMap parameters;
			parameters["foo"] = 123;
			m_rpc->sendRequest("123", "functionOne", parameters);
			QCOMPARE(rpcSpy.count(), 1);
			QCOMPARE(rpcSpy.first().value(0).value<QVariant>(), QVariant("123"));
			QCOMPARE(rpcSpy.first().value(1), QVariant(JsonQt::JsonRpc::BadParameters));
		}

		void testIntropection()
		{
			QSignalSpy rpcSpy(m_rpc, SIGNAL(responseReceived(const QVariant&, const QVariant&)));
			m_rpc->sendRequest(QVariant(), "system.describe");
			QCOMPARE(rpcSpy.count(), 1);
			QVariantMap introspectionData = rpcSpy.first().value(1).value<QVariant>().toMap();
			QVERIFY(!introspectionData.isEmpty());

			// Service data
			QCOMPARE(introspectionData.value("sdversion").toString(), QString("1.0")); // Service Description version
			QCOMPARE(introspectionData.value("name").toString(), QString("TestName"));
			QCOMPARE(introspectionData.value("id").toString(), QString("urn:data:test"));
			QCOMPARE(introspectionData.value("version").toString(), QString("9000.001"));
			QCOMPARE(introspectionData.value("summary").toString(), QString("Ponies"));
			QVERIFY(!introspectionData.contains("help"));
			QVERIFY(introspectionData.contains("procs"));

			// Method data
			QVariantList methods = introspectionData.value("procs").toList();
			QCOMPARE(methods.count(), 4);
			QVariantMap functionOne = methods.takeFirst().toMap();
			QVariantMap functionTwo = methods.takeFirst().toMap();
			QVariantMap functionThree = methods.takeFirst().toMap();
			QVariantMap functionFour = methods.takeFirst().toMap();

			QVariantList params;
			QVariantMap param;

			// functionOne
			QCOMPARE(functionOne.value("name").toString(), QString("functionOne"));
			QCOMPARE(functionOne.value("return").toString(), QString("nil"));

			params = functionOne.value("params").toList();
			QCOMPARE(params.count(), 1);

			param = params.first().toMap();
			QCOMPARE(param.value("name").toString(), QString("foo"));
			QCOMPARE(param.value("type").toString(), QString("str"));

			// functionTwo
			QCOMPARE(functionTwo.value("name").toString(), QString("functionTwo"));
			QCOMPARE(functionTwo.value("return").toString(), QString("str"));

			params = functionTwo.value("params").toList();
			QCOMPARE(params.count(), 0);

			// functionThree
			QCOMPARE(functionThree.value("name").toString(), QString("functionThree"));
			QCOMPARE(functionThree.value("return").toString(), QString("nil"));

			params = functionThree.value("params").toList();
			QCOMPARE(params.count(), 4);

			param = params.takeFirst().toMap();
			QCOMPARE(param.value("name").toString(), QString("one"));
			QCOMPARE(param.value("type").toString(), QString("num"));
			
			param = params.takeFirst().toMap();
			QCOMPARE(param.value("name").toString(), QString("two"));
			QCOMPARE(param.value("type").toString(), QString("bit"));
			
			param = params.takeFirst().toMap();
			QCOMPARE(param.value("name").toString(), QString("three"));
			QCOMPARE(param.value("type").toString(), QString("arr"));
			
			param = params.takeFirst().toMap();
			QCOMPARE(param.value("name").toString(), QString("four"));
			QCOMPARE(param.value("type").toString(), QString("obj"));

			// functionFour
			QCOMPARE(functionFour.value("name").toString(), QString("functionFour"));
			QCOMPARE(functionFour.value("return").toString(), QString("nil"));

			params = functionFour.value("params").toList();
			QCOMPARE(params.count(), 2);

			param = params.takeFirst().toMap();
			QCOMPARE(param.value("name").toString(), QString("foo"));
			QCOMPARE(param.value("type").toString(), QString("str"));

			param = params.takeFirst().toMap();
			QCOMPARE(param.value("name").toString(), QString("bar"));
			QCOMPARE(param.value("type").toString(), QString("str"));
		}
	private:
		JsonQt::JsonRpc* m_rpc;
		JsonQt::JsonRpcAdaptor* m_adaptor;
		TestObject* m_testObject;
};

QTEST_MAIN(JsonRpcAdaptor);

#include "moc_JsonRpcAdaptor.cxx"
