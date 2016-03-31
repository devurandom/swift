/*
 * Copyright (c) 2010-2016 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <string>

#include <boost/shared_ptr.hpp>

#include <Swiften/Base/boost_bsignals.h>
#include <Swiften/Elements/RosterItemPayload.h>
#include <Swiften/JID/JID.h>

namespace Swift {
    class LinkLocalService;
    class LinkLocalServiceBrowser;
    class RosterPayload;
    class Presence;

    class LinkLocalPresenceManager : public boost::bsignals::trackable {
        public:
            LinkLocalPresenceManager(LinkLocalServiceBrowser*);

            boost::shared_ptr<RosterPayload> getRoster() const;
            std::vector<boost::shared_ptr<Presence> > getAllPresence() const;

            boost::optional<LinkLocalService> getServiceForJID(const JID&) const;

            boost::signal<void (boost::shared_ptr<RosterPayload>)> onRosterChanged;
            boost::signal<void (boost::shared_ptr<Presence>)> onPresenceChanged;

        private:
            void handleServiceAdded(const LinkLocalService&);
            void handleServiceChanged(const LinkLocalService&);
            void handleServiceRemoved(const LinkLocalService&);

            RosterItemPayload getRosterItem(const LinkLocalService& service) const;
            std::string getRosterName(const LinkLocalService& service) const;
            boost::shared_ptr<Presence> getPresence(const LinkLocalService& service) const;

        private:
            LinkLocalServiceBrowser* browser;
    };
}
