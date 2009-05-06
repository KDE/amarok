#ifndef _JSONQT_JSON_RPC_ADAPTOR_H
#define _JSONQT_JSON_RPC_ADAPTOR_H

#include "JsonQtExport.h"

#include <QObject>

namespace JsonQt
{
	class JsonRpcAdaptorPrivate;
	/** Class exporting a QObject's public slots via JSON-RPC.
	 *
	 * This implements the JSON-RPC 2.0 proposal, as of 2008-11-15, located
	 * at http://groups.google.com/group/json-rpc/web/json-rpc-1-2-proposal.
	 *
	 * It also implements the introspection functionality of JSON-RPC 1.1.
	 *
	 * There are several Q_CLASSINFO declarations that this adaptor will pay
	 * attention to:
	 *
	 * - JsonQt-RPC-id: *REQUIRED*. This is the only required parameter; it
	 *   specifies a unique identifier for this service, in the format
	 *   described by RFC 3986.
	 *
	 * - JsonQt-RPC-name: the name of the service provided by the QObject.
	 *   If this is not specified, the class name will be used.
	 *
	 * - JsonQt-RPC-version: the version of the service provided by the QObject.
	 *   If this is omitted, it will be omitted from the introspection data.
	 *
	 * - JsonQt-RPC-summary: A short description of this service. If this
	 *   is omitted, it will be omitted from the introspection data.
	 *
	 * - JsonQt-RPC-help: An URL pointing to human-readable documentation
	 *   about the service. If this is omitted, it will be omitted from the
	 *   introspection data.
	 *
	 * The following types are supported for parameters and return values:
	 * - void return
	 * - bool
	 * - int
	 * - QString
	 * - QVariantList (which can contain any types supported by JsonQt)
	 * - QVariantMap (which can contain any types supported by JsonQt)
	 *
	 * @author Fred Emmott <mail@fredemmott.co.uk>
	 */
	class JSONQT_EXPORT JsonRpcAdaptor : public QObject
	{
		Q_OBJECT
		public:
			/** Construct a JsonRpcAdaptor.
			 * This will expose @param adapt via JSON.
			 */
			JsonRpcAdaptor(QObject* adapt, QObject* parent = NULL);
		public slots:
			/// Handle received JSON.
			void processJson(const QString& json);
		signals:
			/** When this is emitted, the JSON provided should be
			 * sent to the peer.
			 */
			void sendJson(const QString& json);
		private:
			JsonRpcAdaptorPrivate* d;
	};
};

#endif
