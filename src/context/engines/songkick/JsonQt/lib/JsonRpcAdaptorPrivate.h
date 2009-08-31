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

#ifndef _JSONQT_JSON_RPC_ADAPTOR_PRIVATE_H
#define _JSONQT_JSON_RPC_ADAPTOR_PRIVATE_H

#include "JsonRpc.h"

#include <QMetaMethod>
#include <QObject>
#include <QStringList>

namespace JsonQt
{
	class JsonRpcAdaptorPrivate : public QObject
	{
		Q_OBJECT
		public:
			JsonRpcAdaptorPrivate(QObject* adapt, QObject* parent);
		signals:
			void sendJson(const QString& json);
		public slots:
			void processJson(const QString& json);
		private slots:
			void requestReceived(const QVariant& id, const QString& method, const QVariant& parameters);
		private:
			struct ReturnData
			{
				bool succeeded;
				int code;
				QString message;
				QVariant data;
			};

			void populateServiceDescription();
			QString getClassInfo(const char* name);
			ReturnData invokeMethod(const QString& method, const QVariant& parameters);

			QMap<QString, QMetaMethod> m_methods;
			QMap<QString, QMap<QString, int> > m_parameterIndices;
			QVariantMap m_serviceDescription;
			JsonRpc m_jsonRpc;
			QObject* m_adapted;
	};
}

#endif
