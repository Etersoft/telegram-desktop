/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_user.h"

#include "observer_peer.h"
#include "storage/localstorage.h"
#include "data/data_session.h"
#include "ui/text_options.h"
#include "apiwrap.h"
#include "lang/lang_keys.h"

namespace {

// User with hidden last seen stays online in UI for such amount of seconds.
constexpr auto kSetOnlineAfterActivity = TimeId(30);

using UpdateFlag = Notify::PeerUpdate::Flag;

} // namespace

BotCommand::BotCommand(
	const QString &command,
	const QString &description)
: command(command)
, _description(description) {
}

bool BotCommand::setDescription(const QString &description) {
	if (_description != description) {
		_description = description;
		_descriptionText = Ui::Text::String();
		return true;
	}
	return false;
}

const Ui::Text::String &BotCommand::descriptionText() const {
	if (_descriptionText.isEmpty() && !_description.isEmpty()) {
		_descriptionText.setText(
			st::defaultTextStyle,
			_description,
			Ui::NameTextOptions());
	}
	return _descriptionText;
}

UserData::UserData(not_null<Data::Session*> owner, PeerId id)
: PeerData(owner, id) {
}

bool UserData::canShareThisContact() const {
	return canShareThisContactFast()
		|| !owner().findContactPhone(peerToUser(id)).isEmpty();
}

void UserData::setIsContact(bool is) {
	const auto status = is
		? ContactStatus::Contact
		: ContactStatus::NotContact;
	if (_contactStatus != status) {
		_contactStatus = status;
		Notify::peerUpdatedDelayed(
			this,
			Notify::PeerUpdate::Flag::UserIsContact);
	}
}

// see Local::readPeer as well
void UserData::setPhoto(const MTPUserProfilePhoto &photo) {
	if (photo.type() == mtpc_userProfilePhoto) {
		const auto &data = photo.c_userProfilePhoto();
		updateUserpic(data.vphoto_id().v, data.vdc_id().v, data.vphoto_small());
	} else {
		clearUserpic();
	}
}

QString UserData::unavailableReason() const {
	return _unavailableReason;
}

void UserData::setUnavailableReason(const QString &text) {
	if (_unavailableReason != text) {
		_unavailableReason = text;
		Notify::peerUpdatedDelayed(
			this,
			Notify::PeerUpdate::Flag::UnavailableReasonChanged);
	}
}

void UserData::setCommonChatsCount(int count) {
	if (_commonChatsCount != count) {
		_commonChatsCount = count;
		Notify::peerUpdatedDelayed(this, UpdateFlag::UserCommonChatsChanged);
	}
}

void UserData::setName(const QString &newFirstName, const QString &newLastName, const QString &newPhoneName, const QString &newUsername) {
	bool changeName = !newFirstName.isEmpty() || !newLastName.isEmpty();

	QString newFullName;
	if (changeName && newFirstName.trimmed().isEmpty()) {
		firstName = newLastName;
		lastName = QString();
		newFullName = firstName;
	} else {
		if (changeName) {
			firstName = newFirstName;
			lastName = newLastName;
		}
		newFullName = lastName.isEmpty() ? firstName : tr::lng_full_name(tr::now, lt_first_name, firstName, lt_last_name, lastName);
	}
	updateNameDelayed(newFullName, newPhoneName, newUsername);
}

void UserData::setPhone(const QString &newPhone) {
	if (_phone != newPhone) {
		_phone = newPhone;
	}
}

void UserData::setBotInfoVersion(int version) {
	if (version < 0) {
		if (botInfo) {
			if (!botInfo->commands.isEmpty()) {
				botInfo->commands.clear();
				Notify::botCommandsChanged(this);
			}
			botInfo = nullptr;
			Notify::userIsBotChanged(this);
		}
	} else if (!botInfo) {
		botInfo = std::make_unique<BotInfo>();
		botInfo->version = version;
		Notify::userIsBotChanged(this);
	} else if (botInfo->version < version) {
		if (!botInfo->commands.isEmpty()) {
			botInfo->commands.clear();
			Notify::botCommandsChanged(this);
		}
		botInfo->description.clear();
		botInfo->version = version;
		botInfo->inited = false;
	}
}

void UserData::setBotInfo(const MTPBotInfo &info) {
	switch (info.type()) {
	case mtpc_botInfo: {
		const auto &d(info.c_botInfo());
		if (peerFromUser(d.vuser_id().v) != id || !botInfo) return;

		QString desc = qs(d.vdescription());
		if (botInfo->description != desc) {
			botInfo->description = desc;
			botInfo->text = Ui::Text::String(st::msgMinWidth);
		}

		auto &v = d.vcommands().v;
		botInfo->commands.reserve(v.size());
		auto changedCommands = false;
		int32 j = 0;
		for (const auto &command : v) {
			command.match([&](const MTPDbotCommand &data) {
				const auto cmd = qs(data.vcommand());
				const auto desc = qs(data.vdescription());
				if (botInfo->commands.size() <= j) {
					botInfo->commands.push_back(BotCommand(cmd, desc));
					changedCommands = true;
				} else {
					if (botInfo->commands[j].command != cmd) {
						botInfo->commands[j].command = cmd;
						changedCommands = true;
					}
					if (botInfo->commands[j].setDescription(desc)) {
						changedCommands = true;
					}
				}
				++j;
			});
		}
		while (j < botInfo->commands.size()) {
			botInfo->commands.pop_back();
			changedCommands = true;
		}

		botInfo->inited = true;

		if (changedCommands) {
			Notify::botCommandsChanged(this);
		}
	} break;
	}
}

void UserData::setNameOrPhone(const QString &newNameOrPhone) {
	if (nameOrPhone != newNameOrPhone) {
		nameOrPhone = newNameOrPhone;
		phoneText.setText(
			st::msgNameStyle,
			nameOrPhone,
			Ui::NameTextOptions());
	}
}

void UserData::madeAction(TimeId when) {
	if (isBot() || isServiceUser() || when <= 0) {
		return;
	} else if (onlineTill <= 0 && -onlineTill < when) {
		onlineTill = -when - kSetOnlineAfterActivity;
		Notify::peerUpdatedDelayed(this, Notify::PeerUpdate::Flag::UserOnlineChanged);
	} else if (onlineTill > 0 && onlineTill < when + 1) {
		onlineTill = when + kSetOnlineAfterActivity;
		Notify::peerUpdatedDelayed(this, Notify::PeerUpdate::Flag::UserOnlineChanged);
	}
}

void UserData::setAccessHash(uint64 accessHash) {
	if (accessHash == kInaccessibleAccessHashOld) {
		_accessHash = 0;
//		_flags.add(MTPDuser_ClientFlag::f_inaccessible | 0);
		_flags.add(MTPDuser::Flag::f_deleted);
	} else {
		_accessHash = accessHash;
	}
}

void UserData::setIsBlocked(bool is) {
	const auto status = is
		? BlockStatus::Blocked
		: BlockStatus::NotBlocked;
	if (_blockStatus != status) {
		_blockStatus = status;
		if (is) {
			_fullFlags.add(MTPDuserFull::Flag::f_blocked);
		} else {
			_fullFlags.remove(MTPDuserFull::Flag::f_blocked);
		}
		Notify::peerUpdatedDelayed(this, UpdateFlag::UserIsBlocked);
	}
}

void UserData::setCallsStatus(CallsStatus callsStatus) {
	if (callsStatus != _callsStatus) {
		_callsStatus = callsStatus;
		Notify::peerUpdatedDelayed(this, UpdateFlag::UserHasCalls);
	}
}

bool UserData::hasCalls() const {
	return (callsStatus() != CallsStatus::Disabled)
		&& (callsStatus() != CallsStatus::Unknown);
}

namespace Data {

void ApplyUserUpdate(not_null<UserData*> user, const MTPDuserFull &update) {
	user->owner().processUser(update.vuser());
	if (const auto photo = update.vprofile_photo()) {
		user->owner().processPhoto(*photo);
	}
	const auto settings = update.vsettings().match([&](
			const MTPDpeerSettings &data) {
		return data.vflags().v;
	});
	user->setSettings(settings);
	user->session().api().applyNotifySettings(
		MTP_inputNotifyPeer(user->input),
		update.vnotify_settings());

	if (const auto info = update.vbot_info()) {
		user->setBotInfo(*info);
	} else {
		user->setBotInfoVersion(-1);
	}
	if (const auto pinned = update.vpinned_msg_id()) {
		user->setPinnedMessageId(pinned->v);
	} else {
		user->clearPinnedMessage();
	}
	user->setFullFlags(update.vflags().v);
	user->setIsBlocked(update.is_blocked());
	user->setCallsStatus(update.is_phone_calls_private()
		? UserData::CallsStatus::Private
		: update.is_phone_calls_available()
		? UserData::CallsStatus::Enabled
		: UserData::CallsStatus::Disabled);
	user->setAbout(qs(update.vabout().value_or_empty()));
	user->setCommonChatsCount(update.vcommon_chats_count().v);
	user->checkFolder(update.vfolder_id().value_or_empty());
	user->fullUpdated();
}

} // namespace Data
