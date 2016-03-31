/*
 * Copyright (c) 2010-2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <map>

#include <boost/shared_ptr.hpp>

#include <Swiften/Elements/Stanza.h>
#include <Swiften/JID/JID.h>

namespace Swift {
    class ServerSession;

    class ServerStanzaRouter {
        public:
            ServerStanzaRouter();

            bool routeStanza(boost::shared_ptr<Stanza>);

            void addClientSession(ServerSession*);
            void removeClientSession(ServerSession*);

        private:
            std::vector<ServerSession*> clientSessions_;
    };
}
