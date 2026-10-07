// Anope IRC Services <https://www.anope.org/>
//
// Copyright (C) 2026 vIRCio contributors
//
// SPDX-License-Identifier: GPL-2.0-only

#include "module.h"

namespace
{
	constexpr const char *SWHOIS_TAG = "vircio-services-staff";
}

class ModuleVircioStaffWhois;

class StaffWhoisReloadTimer final
	: public Timer
{
	ModuleVircioStaffWhois *module;

public:
	StaffWhoisReloadTimer(ModuleVircioStaffWhois *m);

	bool Tick() override;
};

class ModuleVircioStaffWhois final
	: public Module
{
	Anope::string GetStaffWhois(User *user) const
	{
		if (!user || !user->IsServicesOper())
			return "";

		auto *account = user->Account();
		if (!account || !account->o || !account->o->ot)
			return "";

		const auto &opertype = account->o->ot->GetName();
		if (opertype.equals_ci("Services Root"))
			return "Services Root";
		if (opertype.equals_ci("Services Admin"))
			return "Services Admin";
		if (opertype.equals_ci("Services Oper"))
			return "Services Oper";

		return "";
	}

	BotInfo *GetSender() const
	{
		return Config ? Config->GetClient("NickServ") : nullptr;
	}

	void Remove(User *user)
	{
		if (!user || !IRCD || !UplinkSock || !IRCD->CanSendMultipleSWhois)
			return;

		if (auto *sender = GetSender())
			IRCD->SendSWhoisDel(sender, user, SWHOIS_TAG, "");
	}

	void Sync(User *user)
	{
		if (!user || !IRCD || !UplinkSock || !IRCD->CanSendMultipleSWhois)
			return;

		auto *sender = GetSender();
		if (!sender)
			return;

		const auto swhois = GetStaffWhois(user);
		if (swhois.empty())
			IRCD->SendSWhoisDel(sender, user, SWHOIS_TAG, "");
		else
			IRCD->SendSWhois(sender, user, SWHOIS_TAG, 0, swhois);
	}

	void QueueSync()
	{
		new StaffWhoisReloadTimer(this);
	}

public:
	ModuleVircioStaffWhois(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, THIRD)
	{
	}

	~ModuleVircioStaffWhois() override
	{
		for (const auto &[_, user] : UserListByNick)
			Remove(user);
	}

	void SyncAll()
	{
		for (const auto &[_, user] : UserListByNick)
			Sync(user);
	}

	void OnReload(Configuration::Conf &) override
	{
		// Oper assignments are applied after OnReload during a rehash.
		QueueSync();
	}

	void OnUplinkSync(Server *) override
	{
		SyncAll();
	}

	void OnUserLogin(User *user) override
	{
		Sync(user);
	}

	void OnNickLogout(User *user) override
	{
		Remove(user);
	}

	void OnUserModeSet(const MessageSource &, User *user, const Anope::string &mode) override
	{
		if (mode == "OPER")
			Sync(user);
	}

	void OnUserModeUnset(const MessageSource &, User *user, const Anope::string &mode) override
	{
		if (mode == "OPER")
			Sync(user);
		else if (mode == "REGISTERED")
			Remove(user);
	}

	void OnPostCommand(CommandSource &, Command *command, const std::vector<Anope::string> &) override
	{
		if (command->name == "operserv/oper")
			SyncAll();
	}

	void OnModuleLoad(User *, Module *module) override
	{
		if (module == this)
			SyncAll();
	}
};

StaffWhoisReloadTimer::StaffWhoisReloadTimer(ModuleVircioStaffWhois *m)
	: Timer(m, 1)
	, module(m)
{
}

bool StaffWhoisReloadTimer::Tick()
{
	module->SyncAll();
	return false;
}

MODULE_INIT(ModuleVircioStaffWhois)
