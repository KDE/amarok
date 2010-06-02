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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "JsonRpc.h"

#include "JsonToVariant.h"
#include "VariantToJson.h"

#include <QDebug>

namespace JsonQt
{
	JsonRpc::JsonRpc(QObject* parent) : QObject(parent) {}

	void JsonRpc::sendRequest(const QVariant& id, const QString& method, const QVariant& parameters)
	{
		Q_ASSERT(parameters.type() == QVariant::List || parameters.type() == QVariant::Map || parameters.isNull());
		QVariantMap request;
		request["jsonrpc"] = "2.0";
		request["method"] = method;
		request["id"] = id;

		if(!parameters.isNull())
		{
			request["params"] = parameters;
		}

		emit sendJson(VariantToJson::parse(request));
	}

	void JsonRpc::sendNotification(const QString& method, const QVariant& parameters)
	{
		Q_ASSERT(parameters.type() == QVariant::List || parameters.type() == QVariant::Map || parameters.isNull());
		QVariantMap request;
		request["jsonrpc"] = "2.0";
		request["method"] = method;
		// no ID for notifications.

		if(!parameters.isNull())
		{
			request["params"] = parameters;
		}

		emit sendJson(VariantToJson::parse(request));
	}

	void JsonRpc::sendResponse(const QVariant& id, const QVariant& result)
	{
		QVariantMap response;
		response["jsonrpc"] = "2.0";
		response["id"] = id;
		response["result"] = result;
		emit sendJson(VariantToJson::parse(response));
	}
	
	void JsonRpc::sendError(const QVariant& id, int errorCode, const QString& message, const QVariant& data)
	{
		QVariantMap error;
		error["code"] = errorCode;
		error["message"] = message;
		error["data"] = data;

		QVariantMap response;
		response["jsonrpc"] = "2.0";
		response["id"] = id;
		response["error"] = error;
		emit sendJson(VariantToJson::parse(response));
	}

	void JsonRpc::processJson(const QString& json)
	{
		QList<QVariantMap> objects;
		try
		{
			objects = JsonToVariant::multiParse(json);
		}
		catch(ParseException)
		{
			sendError(QVariant(), InvalidJson, "Parse error.");
			return;
		}
		Q_FOREACH(const QVariantMap& object, objects)
		{
			if(object.value("jsonrpc").toString() != "2.0")
			{
				sendError(object.value("id"), InvalidJsonRpc, "JSON-RPC version not specified or not supported.", object);
				continue;
			}

			// Notification or request
			if(object.contains("method"))
			{
				if(object.value("method").type() != QVariant::String)
				{
					sendError(object.value("id"), InvalidJsonRpc, "'method' member of request must be a string.", object);
					continue;
				}
				QString method = object.value("method").toString();

				QVariant parameters = object.value("params");

				if(parameters.isNull()) parameters = QVariantList();
				if(parameters.type() != QVariant::List && parameters.type() != QVariant::Map)
				{
					sendError(object.value("id"), InvalidJsonRpc, "'parameters' member of request must be omitted, a list, or an object.", object);
					continue;
				}

				// Request or notification
				if(object.contains("id"))
				{
					emit requestReceived(object.value("id"), method, parameters);
				}
				else
				{
					emit notificationReceived(method, parameters);
				}
				continue;
			}

			// Request successful
			if(object.contains("result"))
			{
				if(!object.contains("id"))
				{
					sendError(QVariant(), InvalidJsonRpc, "ID not specified in response.", object);
					continue;
				}

				emit responseReceived(object.value("id"), object.value("result"));
				continue;
			}

			// Request failed
			if(object.contains("error"))
			{
				if(!object.contains("id"))
				{
					sendError(QVariant(), InvalidJsonRpc, "ID not specified in response.", object);
					continue;
				}

				if(object.value("error").type() != QVariant::Map)
				{
					sendError(object.value("id"), InvalidJsonRpc, "'error' member is not an Error object.", object);
					continue;
				}

				QVariantMap error = object.value("error").toMap();
				if(error.value("code").type() != QVariant::Int)
				{
					sendError(object.value("id"), InvalidJsonRpc, "'code' member of error object is not an integer.", object);
					continue;
				}

				if(error.value("message").type() != QVariant::String)
				{
					sendError(object.value("id"), InvalidJsonRpc, "'message' member of error object is not a string.", object);
					continue;
				}

				emit errorReceived(object.value("id"), error.value("code").toInt(), error.value("message").toString(), error.value("data"));
				continue;
			}

			// Not a notification, request, or response
			sendError(object.value("id"), InvalidJsonRpc, "JSON object doesn't appear to be a JSON-RPC request, notification, or response.", object);
		}
	}
};
