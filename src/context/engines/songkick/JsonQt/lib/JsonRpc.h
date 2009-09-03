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

#ifndef _JSONQT_JSON_RPC_H
#define _JSONQT_JSON_RPC_H

#include "JsonQtExport.h"

#include <QObject>
#include <QVariant>

namespace JsonQt
{
	/** Class implementing JSON-RPC.
	 *
	 * This implements the JSON-RPC 2.0 proposal, as of 2008-11-15, located
	 * at http://groups.google.com/group/json-rpc/web/json-rpc-1-2-proposal.
	 *
	 * While 1.1 is more popular for web services, 2.0 is a much simpler
	 * protocol, and is not restricted to HTTP requests.
	 *
	 * Note that 'parameter' QVariant objects must be either null QVariants,
	 * contain a QVariantList, or a QVariantMap. If a QVariantList is used,
	 * it must be a list of all the parameters the method takes, in the
	 * correct order. If a QVariantMap is used, the keys must be strings
	 * exactly matching the parameter names of the method (case-sensitive).
	 *
	 * There are two IO members:
	 * - processJson - slot - parse the JSON string given, and act upon it
	 * - sendJson - signal - an action that an instance of JsonRpc has been
	 *   told to do requires that the specified JSON be sent to the other
	 *   peer.
	 *
	 * All other members are interfaces to JSON-RPC.
	 *
	 * 'id' parameters SHOULD NOT be NULL, for reasons defined in the
	 * specification. Also, it SHOULD be a scalar.
	 *
	 * @author Fred Emmott <mail@fredemmott.co.uk>
	 */
	class JSONQT_EXPORT JsonRpc : public QObject
	{
		Q_OBJECT
		public:
			Q_ENUMS(ErrorCode);
			/** List of standard JSON-RPC error codes.
			 * All error codes between ServerErrorLow and
			 * ServerErrorHigh are reserved for
			 * implementation-defined server errors.
			 *
			 * All codes between -32768 and -32000 are reserved
			 * either for the definitions below, or for future use.
			 *
			 * Other values may be used.
			 *
			 * InvalidJson and InvalidJsonRpc should only
			 * be used internally by this class.
			 *
			 */
			enum ErrorCode
			{
				InvalidJson     = -32700,
				InvalidJsonRpc  = -32600,
				MethodNotFound  = -32601,
				BadParameters   = -32602,
				InternalError   = -32603,
				ServerErrorLow  = -32099,
				ServerErrorHigh = -32000
			};

			/// Construct a JsonRpc object.
			JsonRpc(QObject* parent = NULL);

			/// Send a request, expecting a response.
			void sendRequest(const QVariant& id, const QString& method, const QVariant& parameters = QVariant());
			/// Send a request, not expecting a response.
			void sendNotification(const QString& method, const QVariant& parameters = QVariant());
			/// Respond to a request with success.
			void sendResponse(const QVariant& id, const QVariant& result = QVariant());
			/// Respond to a request with an error.
			void sendError(const QVariant& id, int errorCode, const QString& message, const QVariant& data = QVariant());
		public slots:
			/** Process a received JSON string, and emit the
			 * appropriate signals.
			 */
			void processJson(const QString& json);
		signals:
			/** Emitted when JsonRpc needs to send some JSON to the
			 * other peer.
			 */
			void sendJson(const QString& json);

			/** A notification has been received from the other peer.
			 * No response is necessary.
			 */
			void notificationReceived(const QString& method, const QVariant& parameters);
			/** A request has been received from the other peer.
			 * A response (with the same id) is required.
			 */
			void requestReceived(const QVariant& id, const QString& method, const QVariant& parameters);
			/// A successful response has been received from the other peer.
			void responseReceived(const QVariant& id, const QVariant& result);
			/// An error has been received from the other peer.
			void errorReceived(const QVariant& id, int errorCode, const QString& message, const QVariant& data);
	};
};

#endif
