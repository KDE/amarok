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
