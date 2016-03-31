/*
 * Copyright (c) 2011-2015 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <Swiften/Base/API.h>
#include <Swiften/Base/SafeByteArray.h>
#include <Swiften/Base/boost_bsignals.h>
#include <Swiften/Network/Connection.h>
#include <Swiften/TLS/TLSOptions.h>

namespace Swift {
    class HostAddressPort;
    class TLSContextFactory;
    class TLSContext;

    class SWIFTEN_API TLSConnection : public Connection {
        public:

            TLSConnection(Connection::ref connection, TLSContextFactory* tlsFactory, const TLSOptions&);
            virtual ~TLSConnection();

            virtual void listen() {assert(false);}
            virtual void connect(const HostAddressPort& address);
            virtual void disconnect();
            virtual void write(const SafeByteArray& data);

            virtual HostAddressPort getLocalAddress() const;
            virtual HostAddressPort getRemoteAddress() const;

            TLSContext* getTLSContext() const;

        private:
            void handleRawConnectFinished(bool error);
            void handleRawDisconnected(const boost::optional<Error>& error);
            void handleRawDataRead(boost::shared_ptr<SafeByteArray> data);
            void handleRawDataWritten();
            void handleTLSConnectFinished(bool error);
            void handleTLSDataForNetwork(const SafeByteArray& data);
            void handleTLSDataForApplication(const SafeByteArray& data);

        private:
            TLSContext* context;
            Connection::ref connection;
    };
}
