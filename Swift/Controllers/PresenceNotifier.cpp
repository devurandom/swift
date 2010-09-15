/*
 * Copyright (c) 2010 Remko Tronçon
 * Licensed under the GNU General Public License v3.
 * See Documentation/Licenses/GPLv3.txt for more information.
 */

#include "Swift/Controllers/PresenceNotifier.h"

#include <boost/bind.hpp>

#include "Swiften/Client/StanzaChannel.h"
#include "Swiften/Base/ByteArray.h"
#include "Swiften/MUC/MUCRegistry.h"
#include "Swiften/Roster/XMPPRoster.h"
#include "Swiften/Presence/PresenceOracle.h"
#include "Swiften/Network/TimerFactory.h"

namespace Swift {

PresenceNotifier::PresenceNotifier(StanzaChannel* stanzaChannel, Notifier* notifier, const MUCRegistry* mucRegistry, AvatarManager* avatarManager, const XMPPRoster* roster, const PresenceOracle* presenceOracle, TimerFactory* timerFactory) : stanzaChannel(stanzaChannel), notifier(notifier), mucRegistry(mucRegistry), avatarManager(avatarManager), roster(roster), presenceOracle(presenceOracle), timerFactory(timerFactory) {
	justInitialized = true;
	inQuietPeriod = false;
	stanzaChannel->onPresenceReceived.connect(boost::bind(&PresenceNotifier::handlePresenceReceived, this, _1));
	stanzaChannel->onAvailableChanged.connect(boost::bind(&PresenceNotifier::handleStanzaChannelAvailableChanged, this, _1));
	setInitialQuietPeriodMS(3000);
}

PresenceNotifier::~PresenceNotifier() {
	if (timer) {
		timer->stop();
		timer->onTick.disconnect(boost::bind(&PresenceNotifier::handleTimerTick, this));
		timer.reset();
	}
	stanzaChannel->onAvailableChanged.disconnect(boost::bind(&PresenceNotifier::handleStanzaChannelAvailableChanged, this, _1));
	stanzaChannel->onPresenceReceived.disconnect(boost::bind(&PresenceNotifier::handlePresenceReceived, this, _1));
}

void PresenceNotifier::handlePresenceReceived(boost::shared_ptr<Presence> presence) {
	JID from = presence->getFrom();

	if (mucRegistry->isMUC(from.toBare())) {
		return;
	}

	if (justInitialized) {
		justInitialized = false;
		if (timer) {
			inQuietPeriod = true;
		}
	}

	if (inQuietPeriod) {
		timer->stop();
		timer->start();
		return;
	}

	std::set<JID>::iterator i = availableUsers.find(from);
	if (presence->isAvailable()) {
		if (i != availableUsers.end()) {
			showNotification(from, Notifier::ContactStatusChange);
		}
		else {
			showNotification(from, Notifier::ContactAvailable);
			availableUsers.insert(from);
		}
	}
	else {
		if (i != availableUsers.end()) {
			showNotification(from, Notifier::ContactUnavailable);
			availableUsers.erase(i);
		}
	}
}

void PresenceNotifier::handleStanzaChannelAvailableChanged(bool available) {
	if (available) {
		availableUsers.clear();
		justInitialized = true;
		if (timer) {
			timer->stop();
		}
	}
}

void PresenceNotifier::showNotification(const JID& jid, Notifier::Type type) {
	String name = roster->getNameForJID(jid);
	if (name.isEmpty()) {
		name = jid.toBare().toString();
	}
	String title = name + " (" + getStatusType(jid) + ")";
	String message = getStatusMessage(jid);
	notifier->showMessage(type, title, message, avatarManager->getAvatar(jid), boost::bind(&PresenceNotifier::handleNotificationActivated, this, jid));
}

void PresenceNotifier::handleNotificationActivated(JID jid) {
	onNotificationActivated(jid);
}

String PresenceNotifier::getStatusType(const JID& jid) const {
	Presence::ref presence = presenceOracle->getLastPresence(jid);
	if (presence) {
		return StatusShow::typeToFriendlyName(presence->getShow());
	}
	else {
		return "Unavailable";
	}
}

String PresenceNotifier::getStatusMessage(const JID& jid) const {
	Presence::ref presence = presenceOracle->getLastPresence(jid);
	if (presence) {
		return presence->getStatus();
	}
	else {
		return String();
	}
}

void PresenceNotifier::setInitialQuietPeriodMS(int ms) {
	if (timer) {
		timer->stop();
		timer->onTick.disconnect(boost::bind(&PresenceNotifier::handleTimerTick, this));
		timer.reset();
	}
	if (ms > 0) {
		timer = timerFactory->createTimer(ms);
		timer->onTick.connect(boost::bind(&PresenceNotifier::handleTimerTick, this));
	}
}

void PresenceNotifier::handleTimerTick() {
	inQuietPeriod = false;
	timer->stop();
}


}