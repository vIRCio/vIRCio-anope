// Anope IRC Services <https://www.anope.org/>
//
// Copyright (C) 2026 vIRCio contributors
//
// SPDX-License-Identifier: GPL-2.0-only

#include "module.h"

namespace
{
	constexpr const char *DEFAULT_TIMEZONE = "America/Sao_Paulo";
}

class ModuleVircioDefaultTimezone final
	: public Module
{
	ExtensibleRef<Anope::string> timezone;

	void ApplyDefault(NickCore *account)
	{
		if (!account || !timezone || timezone->HasExt(account))
			return;

		timezone->Set(account, DEFAULT_TIMEZONE);
	}

	void ApplyDefaults()
	{
		for (const auto &[_, account] : *NickCoreList)
			ApplyDefault(account);
	}

	EventReturn OnSetNickOption(CommandSource &source, Command *command,
		NickCore *account, const Anope::string &setting) override
	{
		if (!command || !setting.empty() ||
			(!command->name.equals_ci("nickserv/set/timezone") &&
			 !command->name.equals_ci("nickserv/saset/timezone")))
			return EVENT_CONTINUE;

		if (!timezone)
			return EVENT_CONTINUE;

		timezone->Set(account, DEFAULT_TIMEZONE);
		Log(account == source.GetAccount() ? LOG_COMMAND : LOG_ADMIN, source, command)
			<< "to change the timezone of " << account->display << " to " << DEFAULT_TIMEZONE;
		if (source.GetAccount() == account)
			source.Reply(_("Timezone changed to \002%s\002."), DEFAULT_TIMEZONE);
		else
			source.Reply(_("Timezone for \002%s\002 changed to \002%s\002."),
				account->display.c_str(), DEFAULT_TIMEZONE);

		return EVENT_STOP;
	}

public:
	ModuleVircioDefaultTimezone(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, THIRD)
		, timezone("timezone")
	{
	}

	void OnReload(Configuration::Conf &) override
	{
		if (!timezone)
			throw ModuleException("vircio_defaulttimezone requires ns_set_timezone");

		ApplyDefaults();
	}

	void OnPostInit() override
	{
		ApplyDefaults();
	}

	void OnNickCoreCreate(NickCore *account) override
	{
		ApplyDefault(account);
	}

	void OnUserLogin(User *user) override
	{
		ApplyDefault(user ? user->Account() : nullptr);
	}

	void OnModuleLoad(User *, Module *module) override
	{
		if (module == this)
			ApplyDefaults();
	}
};

MODULE_INIT(ModuleVircioDefaultTimezone)
