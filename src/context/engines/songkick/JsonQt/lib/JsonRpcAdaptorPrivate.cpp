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

#include "JsonRpcAdaptorPrivate.h"

#include "VariantToJson.h"

#include <QDebug>
#include <QGenericArgument>
#include <QMetaClassInfo>
#include <QMetaObject>

namespace JsonQt
{
	JsonRpcAdaptorPrivate::JsonRpcAdaptorPrivate(QObject* adapt, QObject* parent) : QObject(parent)
	{
		m_adapted = adapt;
		connect(
			&m_jsonRpc,
			SIGNAL(sendJson(const QString&)),
			this,
			SIGNAL(sendJson(const QString&))
		);
		connect(
			&m_jsonRpc,
			SIGNAL(requestReceived(const QVariant&, const QString&, const QVariant&)),
			this,
			SLOT(requestReceived(const QVariant&, const QString&, const QVariant&))
		);
		populateServiceDescription();
	}

	void JsonRpcAdaptorPrivate::populateServiceDescription()
	{
		m_serviceDescription.clear();
		const QMetaObject* metaObject = m_adapted->metaObject();

		QString id = getClassInfo("JsonQt-RPC-id");
		Q_ASSERT(!id.isEmpty());
		m_serviceDescription["id"] = id;
		m_serviceDescription["sdversion"] = "1.0";
		QString name = getClassInfo("JsonQt-RPC-name");
		if(name.isEmpty()) name = metaObject->className();
		m_serviceDescription["name"] = name;

		QString version = getClassInfo("JsonQt-RPC-version");
		if(!version.isNull()) m_serviceDescription["version"] = version;

		QString summary = getClassInfo("JsonQt-RPC-summary");
		if(!summary.isNull()) m_serviceDescription["summary"] = summary;

		QString help = getClassInfo("JsonQt-RPC-help");
		if(!help.isNull()) m_serviceDescription["help"] = help;

		QMap<QString, QString> typeMap;
		typeMap[""] = "nil"; // void
		typeMap["bool"] = "bit";
		typeMap["int"] = "num";
		typeMap["QString"] = "str";
		// typeMap["QVariant"] = "any"; // not supported
		typeMap["QVariantList"] = "arr";
		typeMap["QVariantMap"] = "obj";

		QVariantList procs;

		for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
		{
			QMetaMethod method = metaObject->method(i);

			// Check we should export it
			if(method.access() != QMetaMethod::Public || method.methodType() != QMetaMethod::Slot) continue;

			QVariantMap proc;
			QString signature = method.signature();

			// Name
			QString methodName = signature.left(signature.indexOf('('));
			proc["name"] = methodName;

			// Return type
			if(!typeMap.contains(method.typeName()))
			{
				qDebug() << "Public slot" << signature << "has unknown return type" << method.typeName();
				continue;
			}
			proc["return"] = typeMap.value(method.typeName());

			// Parameters
			QVariantList parameters;
			bool badParameter = false;
			for(int i = 0; i < method.parameterNames().count(); ++i)
			{
				QVariantMap parameter;
				QString parameterName(method.parameterNames().at(i));
				parameter["name"] = parameterName;

				QString parameterType = method.parameterTypes().at(i);
				if(!typeMap.contains(method.parameterTypes().at(i)))
				{
					qDebug() << "Public slot" << signature << "has parameter" << parameterName << "with unknown type" << parameterType;
					badParameter = true;
					continue;
				}
				parameter["type"] = typeMap.value(parameterType);
				parameters.append(parameter);
				m_parameterIndices[methodName].insert(parameterName, i);
			}
			if(badParameter) continue;

			m_methods.insert(methodName, method);
			proc["params"] = parameters;

			// Done
			procs.append(proc);
		}
		m_serviceDescription["procs"] = procs;
	}

	QString JsonRpcAdaptorPrivate::getClassInfo(const char* name)
	{
		const QMetaObject* metaObject = m_adapted->metaObject();
		int index = metaObject->indexOfClassInfo(name);
		if(index == -1) return QString();
		return metaObject->classInfo(index).value();
	}

	void JsonRpcAdaptorPrivate::requestReceived(const QVariant& id, const QString& method, const QVariant& parameters)
	{
		if(method == "system.describe")
		{
			m_jsonRpc.sendResponse(id, m_serviceDescription);
			return;
		}
		ReturnData result = invokeMethod(method, parameters);
		if(result.succeeded)
		{
			m_jsonRpc.sendResponse(id, result.data);
		}
		else
		{
			m_jsonRpc.sendError(id, result.code, result.message, result.data);
		}
	}

	JsonRpcAdaptorPrivate::ReturnData JsonRpcAdaptorPrivate::invokeMethod(const QString& methodName, const QVariant& parameters)
	{
		ReturnData ret;
		ret.succeeded = false;
		if(!m_methods.contains(methodName))
		{
			ret.code = JsonRpc::MethodNotFound;
			ret.message = QString("The method %1 does not exist.").arg(methodName);
			return ret;
		}
		QMetaMethod metaMethod(m_methods.value(methodName));

		int parameterCount;
		QVariantList parameterList;
		switch(parameters.type())
		{
			case QVariant::List:
				parameterList = parameters.toList();
				parameterCount = parameterList.count();
				break;
			case QVariant::Map:
				parameterCount = parameters.toMap().count();
				for(int i = 0; i < parameterCount; ++i)
				{
					parameterList.append(QVariant());
				}
				break;
			default:
				parameterCount = 0;
		}
		Q_ASSERT(parameterCount <= 9);

		ret.code = JsonRpc::BadParameters;
		if(parameterCount != metaMethod.parameterNames().count())
		{
			ret.message = "Parameter count mismatch.";
			return ret;
		}


		if(parameters.type() == QVariant::Map)
		{
			QMap<QString, int> parameterIndices = m_parameterIndices.value(methodName);
			QVariantMap parameterMap = parameters.toMap();
			for(QVariantMap::ConstIterator it = parameterMap.constBegin(); it != parameterMap.constEnd(); ++it)
			{
				if(!parameterIndices.contains(it.key()))
				{
					ret.message = QString("'%1' is not a parameter of method '%2'.").arg(it.key()).arg(methodName);
					return ret;
				}
				int index = parameterIndices.value(it.key());
				Q_ASSERT(index < parameterCount);
				parameterList[index] = it.value();
			}
		}

		QMap<QString, QVariant::Type> typeMap;
		typeMap[""] = QVariant::Invalid;
		typeMap["bool"] = QVariant::Bool;
		typeMap["int"] = QVariant::Int;
		typeMap["QString"] = QVariant::String;
		typeMap["QVariantList"] = QVariant::List;
		typeMap["QVariantMap"] = QVariant::Map;
		///@todo more types

		QVariant::Type returnType = typeMap.value(metaMethod.typeName());

		// QMetaObject::invokeMethod takes 10 generic arguments
		QGenericArgument arguments[10];
		for(int i = 0; i < parameterCount; ++i)
		{
			const QVariant& value = parameterList.value(i);
			if(typeMap.value(metaMethod.parameterTypes().at(i)) != value.type())
			{
				ret.message = QString("Value for parameter %1 was not of the correct type.").arg(QString(metaMethod.parameterNames().at(i)));
				return ret;
			}
			void* data = 0;
			switch(value.type())
			{
				case QVariant::Bool:
					data = new bool(value.toBool());
					arguments[i] = Q_ARG(bool, *static_cast<bool*>(data));
					break;
				case QVariant::Int:
					data = new int(value.toInt());
					arguments[i] = Q_ARG(int, *static_cast<int*>(data));
					break;
				case QVariant::String:
					data = new QString(value.toString());
					arguments[i] = Q_ARG(QString, *static_cast<QString*>(data));
					break;
				case QVariant::List:
					data = new QVariantList(value.toList());
					arguments[i] = Q_ARG(QVariantList, *static_cast<QVariantList*>(data));
					break;
				case QVariant::Map:
					data = new QVariantMap(value.toMap());
					arguments[i] = Q_ARG(QVariantMap, *static_cast<QVariantMap*>(data));
					break;
				default:
					break;
			}
		}

		QGenericReturnArgument returnValue;

		switch(returnType)
		{
			case QVariant::Bool:
				returnValue = Q_RETURN_ARG(bool, *(new bool));
				break;
			case QVariant::Int:
				returnValue = Q_RETURN_ARG(int, *(new int));
				break;
			case QVariant::String:
				returnValue = Q_RETURN_ARG(QString, *(new QString));
				break;
			case QVariant::List:
				returnValue = Q_RETURN_ARG(QVariantList, *(new QVariantList));
				break;
			case QVariant::Map:
				returnValue = Q_RETURN_ARG(QVariantMap, *(new QVariantMap));
				break;
			default:
				break;
		}

		bool success = m_adapted->metaObject()->invokeMethod(
			m_adapted,
			methodName.toLatin1().constData(),
			Qt::DirectConnection,
			returnValue,
			arguments[0],
			arguments[1],
			arguments[2],
			arguments[3],
			arguments[4],
			arguments[5],
			arguments[6],
			arguments[7],
			arguments[8],
			arguments[9]
		);

		// clean up memory allocation
		for(int i = 0; i < parameterCount; ++i)
		{
			QVariant::Type parameterType = parameterList.value(i).type();
			switch(parameterType)
			{
				case QVariant::Bool:
					delete static_cast<bool*>(arguments[i].data());
					break;
				case QVariant::Int:
					delete static_cast<int*>(arguments[i].data());
					break;
				case QVariant::String:
					delete static_cast<QString*>(arguments[i].data());
					break;
				case QVariant::List:
					delete static_cast<QVariantList*>(arguments[i].data());
					break;
				case QVariant::Map:
					delete static_cast<QVariantMap*>(arguments[i].data());
					break;
				default:
					break;
			}
		}

		if(success == false)
		{
			ret.code = JsonRpc::InternalError;
			ret.message = "Could not execute method.";
			return ret;
		}

		ret.succeeded = true;

		switch(returnType)
		{
			case QVariant::Bool:
				ret.data = *static_cast<bool*>(returnValue.data());
				break;
			case QVariant::Int:
				ret.data = *static_cast<int*>(returnValue.data());
				break;
			case QVariant::String:
				ret.data = *static_cast<QString*>(returnValue.data());
				break;
			case QVariant::List:
				ret.data = *static_cast<QVariantList*>(returnValue.data());
				break;
			case QVariant::Map:
				ret.data = *static_cast<QVariantMap*>(returnValue.data());
				break;
			default:
				break;
		}

		return ret;
	}

	void JsonRpcAdaptorPrivate::processJson(const QString& json)
	{
		m_jsonRpc.processJson(json);
	}
}
