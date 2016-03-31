/*
 * Copyright (c) 2010-2015 Isode Limited.
 * All rights reserved.
 * See the COPYING file for more information.
 */

#pragma once

#include <set>
#include <string>

#include <boost/shared_ptr.hpp>

#include <Swiften/Avatars/AvatarManager.h>
#include <Swiften/Base/boost_bsignals.h>
#include <Swiften/Elements/ErrorPayload.h>
#include <Swiften/Elements/Presence.h>
#include <Swiften/Elements/RosterPayload.h>
#include <Swiften/Elements/VCard.h>
#include <Swiften/JID/JID.h>

#include <Swift/Controllers/Roster/ContactRosterItem.h>
#include <Swift/Controllers/UIEvents/UIEvent.h>

namespace Swift {
    class AvatarManager;
    class ClientBlockListManager;
    class EntityCapsProvider;
    class EventController;
    class FileTransferManager;
    class FileTransferOverview;
    class IQRouter;
    class MainWindow;
    class MainWindowFactory;
    class NickManager;
    class NickResolver;
    class OfflineRosterFilter;
    class PresenceOracle;
    class Roster;
    class RosterGroupExpandinessPersister;
    class RosterVCardProvider;
    class SettingsProvider;
    class SubscriptionManager;
    class SubscriptionRequestEvent;
    class UIEventStream;
    class VCardManager;
    class XMPPRoster;
    class XMPPRosterItem;

    class RosterController {
        public:
            RosterController(const JID& jid, XMPPRoster* xmppRoster, AvatarManager* avatarManager, MainWindowFactory* mainWindowFactory, NickManager* nickManager, NickResolver* nickResolver, PresenceOracle* presenceOracle, SubscriptionManager* subscriptionManager, EventController* eventController, UIEventStream* uiEventStream, IQRouter* iqRouter, SettingsProvider* settings, EntityCapsProvider* entityCapsProvider, FileTransferOverview* fileTransferOverview, ClientBlockListManager* clientBlockListManager, VCardManager* vcardManager);
            ~RosterController();
            void showRosterWindow();
            void setJID(const JID& jid) { myJID_ = jid; }
            MainWindow* getWindow() {return mainWindow_;}
            boost::signal<void (StatusShow::Type, const std::string&)> onChangeStatusRequest;
            boost::signal<void ()> onSignOutRequest;
            void handleOwnVCardChanged(VCard::ref vcard);
            void handleAvatarChanged(const JID& jid);
            void handlePresenceChanged(Presence::ref presence);
            void setEnabled(bool enabled);

            boost::optional<XMPPRosterItem> getItem(const JID&) const;
            std::set<std::string> getGroups() const;

            void setContactGroups(const JID& jid, const std::vector<std::string>& groups);
            void updateItem(const XMPPRosterItem&);

            void initBlockingCommand();

        private:
            void handleOnJIDAdded(const JID &jid);
            void handleRosterCleared();
            void handleOnJIDRemoved(const JID &jid);
            void handleOnJIDUpdated(const JID &jid, const std::string& oldName, const std::vector<std::string>& oldGroups);
            void handleStartChatRequest(const JID& contact);
            void handleChangeStatusRequest(StatusShow::Type show, const std::string &statusText);
            void handleShowOfflineToggled(bool state);
            void handleIncomingPresence(boost::shared_ptr<Presence> newPresence);
            void handleSubscriptionRequest(const JID& jid, const std::string& message);
            void handleSubscriptionRequestAccepted(SubscriptionRequestEvent* event);
            void handleSubscriptionRequestDeclined(SubscriptionRequestEvent* event);
            void handleUIEvent(boost::shared_ptr<UIEvent> event);
            void handleRosterItemUpdated(ErrorPayload::ref error, boost::shared_ptr<RosterPayload> rosterPayload);
            void handleRosterSetError(ErrorPayload::ref error, boost::shared_ptr<RosterPayload> rosterPayload);
            void applyAllPresenceTo(const JID& jid);
            void handleEditProfileRequest();
            void handleOnCapsChanged(const JID& jid);
            void handleSettingChanged(const std::string& settingPath);

            void handleBlockingStateChanged();
            void handleBlockingItemAdded(const JID& jid);
            void handleBlockingItemRemoved(const JID& jid);

            JID myJID_;
            XMPPRoster* xmppRoster_;
            MainWindowFactory* mainWindowFactory_;
            MainWindow* mainWindow_;
            Roster* roster_;
            OfflineRosterFilter* offlineFilter_;
            VCardManager* vcardManager_;
            AvatarManager* avatarManager_;
            NickManager* nickManager_;
            NickResolver* nickResolver_;
            PresenceOracle* presenceOracle_;
            SubscriptionManager* subscriptionManager_;
            EventController* eventController_;
            RosterGroupExpandinessPersister* expandiness_;
            IQRouter* iqRouter_;
            SettingsProvider* settings_;
            UIEventStream* uiEventStream_;
            EntityCapsProvider* entityCapsManager_;
            FileTransferOverview* ftOverview_;
            ClientBlockListManager* clientBlockListManager_;
            RosterVCardProvider* rosterVCardProvider_;
            boost::shared_ptr<ContactRosterItem> ownContact_;

            boost::bsignals::scoped_connection blockingOnStateChangedConnection_;
            boost::bsignals::scoped_connection blockingOnItemAddedConnection_;
            boost::bsignals::scoped_connection blockingOnItemRemovedConnection_;
            boost::bsignals::scoped_connection changeStatusConnection_;
            boost::bsignals::scoped_connection signOutConnection_;
            boost::bsignals::scoped_connection uiEventConnection_;
    };
}
