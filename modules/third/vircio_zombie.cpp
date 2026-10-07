// Anope IRC Services <https://www.anope.org/>
//
// Copyright (C) 2026 vIRCio contributors
//
// SPDX-License-Identifier: GPL-2.0-only

#include "module.h"

namespace
{
	constexpr const char *ZOMBIE_MODE = "U_SERVICES_ZOMBIE";
}

class CommandOSZombie final
	: public Command
{
public:
	CommandOSZombie(Module *creator)
		: Command(creator, "operserv/zombie", 2, 2)
	{
		this->SetDesc(_("Manage the vIRCio Zombie quarantine"));
		this->SetSyntax(_("ADD \037nick-or-uid\037"));
		this->SetSyntax(_("DEL \037nick-or-uid\037"));
		this->RequireUser(true);
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		const auto &action = params[0];
		auto *target = User::Find(params[1]);
		if (!target)
		{
			source.Reply(NICK_X_NOT_IN_USE, params[1].c_str());
			return;
		}

		auto *mode = ModeManager::FindUserModeByName(ZOMBIE_MODE);
		if (!mode)
		{
			source.Reply(_("The vIRCio Zombie mode is not available from the IRCd."));
			return;
		}

		if (action.equals_ci("ADD"))
		{
			if (target->Account())
			{
				source.Reply(_("%s is already identified to an account and can not be quarantined."), target->nick.c_str());
				return;
			}

			if (target->HasMode(mode->name))
			{
				source.Reply(_("%s is already in Zombie quarantine."), target->nick.c_str());
				return;
			}

			target->SetMode(source.service, mode);
			Log(LOG_ADMIN, source, this) << "enabled Zombie quarantine on " << target->nick;
			source.Reply(_("Zombie quarantine enabled for %s."), target->nick.c_str());
		}
		else if (action.equals_ci("DEL"))
		{
			if (!target->HasMode(mode->name))
			{
				source.Reply(_("%s is not in Zombie quarantine."), target->nick.c_str());
				return;
			}

			target->RemoveMode(source.service, mode);
			Log(LOG_ADMIN, source, this) << "disabled Zombie quarantine on " << target->nick;
			source.Reply(_("Zombie quarantine disabled for %s."), target->nick.c_str());
		}
		else
			this->OnSyntaxError(source, action);
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_(
			"Enables or disables the vIRCio Zombie quarantine for an online user. "
			"The IRCd enforces the quarantine and automatically removes it after "
			"account authentication."
		));
		return true;
	}
};

class ModuleVircioZombie final
	: public Module
{
	CommandOSZombie commandoszombie;

public:
	ModuleVircioZombie(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, THIRD)
		, commandoszombie(this)
	{
	}
};

MODULE_INIT(ModuleVircioZombie)
