#include "JsonRpcAdaptor.h"
#include "JsonRpcAdaptorPrivate.h"

namespace JsonQt
{

	JsonRpcAdaptor::JsonRpcAdaptor(QObject* adapt, QObject* parent)
		:
			QObject(parent),
			d(new JsonRpcAdaptorPrivate(adapt, this))
	{
		connect(d, SIGNAL(sendJson(const QString&)), this, SIGNAL(sendJson(const QString&)));
	}

	void JsonRpcAdaptor::processJson(const QString& json)
	{
		d->processJson(json);
	}
};
