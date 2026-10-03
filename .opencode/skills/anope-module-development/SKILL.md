---
name: anope-module-development
description: Develop, port, review, debug, and maintain native-style vIRCio modules for Anope 2.1, targeting the 2.1.27 production baseline and InspIRCd 4+.
compatibility: opencode
metadata:
  project: "vIRCio-anope"
  baseline: "Anope 2.1.27"
  ircd: "InspIRCd 4+"
---

# Anope 2.1 Module Development

## Scope

Operational guide for vIRCio modules on Anope 2.1.27: new modules, legacy ports,
reviews, unload safety, and InspIRCd 4+ integrations. Read `AGENTS.md` first.
Do not use this skill to alter core, runtime, configuration, or secrets without
separate authorization.

## Version policy

Target exactly Anope 2.1.27 at `1dce1fd39a99016c6dc6d3d4a7c5ee11212dd02e`
on `vIRCio-2.1`; `2.1` tracks upstream. Do not silently use APIs from later
2.1 HEAD. Target InspIRCd 4+ only; do not preserve InspIRCd 3 compatibility.

## Source of truth

Local baseline source is authoritative over memory, old source, comments,
tutorials, or online material. Confirm signatures in headers, behavior in the
implementation, and patterns in official modules. If unproven, say “verify in
the target source before use”, not an invented rule.

## Architecture mental model

Prefer:

    native feature -> configuration -> official module -> Service/API ->
    custom THIRD module -> core patch last

Keep compilable custom source in `modules/third/`; keep its vIRCio documentation
and inventory in `vIRCio/modules/`. The latter is not a build directory.

## Module skeleton

`MODULE_INIT(Type)` exports `AnopeInit`, `AnopeFini`, and `AnopeVersion`. A
normal vIRCio module is minimal:

```cpp
class ModuleExample final
	: public Module
{
public:
	ModuleExample(const Anope::string &modname, const Anope::string &creator)
		: Module(modname, creator, THIRD)
	{
	}
};

MODULE_INIT(ModuleExample)
```

Members such as commands, Services, extensible items, and timers need clear
module ownership and RAII lifetimes.

## Module types

`ModType` defines `THIRD`, `VENDOR`, `EXTRA`, `DATABASE`, `ENCRYPTION`,
`PSEUDOCLIENT`, `PROTOCOL`, and `DEPRECATED`. A normal vIRCio module is
`THIRD`. Do not use `VENDOR`: it identifies a module made and shipped by Anope.
`EXTRA` describes an optional/extreme or externally-dependent module shown by
default in `/os modlist`; it is not a synonym for third-party. `Module` defaults
to `THIRD` and adds `THIRD` when the type is not `VENDOR`.

## Module lifecycle

The 2.1.27 loader performs:

    dlopen
    -> AnopeVersion exact-version check
    -> AnopeInit / MODULE_INIT factory
    -> construction (Module base, members, derived constructor body)
    -> OnReload(*Config)
    -> add module to every EventHandlers vector
    -> Serialize::CreateTypes()
    -> Prioritize()
    -> OnModuleLoad

The verified reload signature is:

```cpp
void OnReload(Configuration::Conf &conf) override
```

On unload, observers receive `OnModuleUnload`, then `AnopeFini` destroys the
module and members, `Module::~Module` runs, and the loader calls `dlclose`.
The destructor unsets module extensibles, detaches hooks, removes that module's
`IdentifyRequest` holds, deletes module-owned timers, removes the module from
its registry, and removes its localization domain when enabled. It does not
prove automatic cleanup of HTTP pages, external callbacks, or custom threads:
unregister, cancel, and join those explicitly.

## Events and hooks

`include/modules.h` has a historical comment saying modules must attach events.
The executable loader instead pushes every loaded module into all event vectors.
Do **not** add manual `ModuleManager::Attach` by default. Unimplemented hooks
throw `NotImplementedException`; dispatcher macros remove that module from the
specific event vector after it throws.

Implement only hooks whose documented contract fits. Use `Prioritize()` and
`ModuleManager::SetPriority` only when ordering changes semantics, stating the
specific conflicting event/module.

## EventReturn semantics

`EventReturn` is `EVENT_STOP`, `EVENT_CONTINUE`, or `EVENT_ALLOW`.
`FOREACH_RESULT` starts at `EVENT_CONTINUE` and stops at the first value other
than `EVENT_CONTINUE`. `EVENT_ALLOW` is therefore never generic “continue”; its
meaning depends on the individual hook contract. Some hooks treat it as an
access bypass while others only use `EVENT_STOP` as a veto.

## Service and ServiceReference

`Service` registers in its constructor and unregisters in its destructor.
Use `ServiceReference<T>` by type/name for a provider that may unload. It is a
`Reference<T>` which invalidates and resolves again; do not cache a durable raw
`Service*` to bypass it.

- Short, obvious lifetime: a raw `T*` can be idiomatic.
- Durable `Base` target: `Reference<T>`.
- Durable `Serializable` target: `Serialize::Reference<T>`.
- Inter-module provider: `ServiceReference<T>`.
- Explicit local ownership: RAII and `unique_ptr` where appropriate.

Do not require `shared_ptr` as generic style.

## Commands

`Command` is a Service. Verified API:

```cpp
class CommandExample final : public Command
{
public:
	CommandExample(Module *creator)
		: Command(creator, "example/do", 1, 1)
	{
		SetDesc(_("Do the example action"));
		SetSyntax(_("\037target\037"));
	}

	void Execute(CommandSource &source,
		const std::vector<Anope::string> &params) override
	{
	}

	bool OnHelp(CommandSource &source, const Anope::string &) override
	{
		SendSyntax(source);
		return true;
	}
};
```

Command permission comes from configured `CommandInfo` and is checked through
`source.HasCommand`. `HasPriv(...)` is a separate Services-oper privilege;
`IsServicesOper()` identifies a Services oper; `IsOper()` concerns IRC oper
mode for a live user (or maps an account source to Services-oper status);
channel authority comes from `AccessFor`/founder checks; account identity is
`NickCore`. Do not use `if (!source.IsOper())` as universal authorization.

## Permissions and OperType

`OperType` has globbed command and privilege lists. `Oper` independently may
require Services login and IRC `+o`. Pick the actual authorization model, use
comparable official modules for sensitive operations, and use appropriate `Log`
context without exposing credentials.

## Configuration and rehash

`OnReload(Configuration::Conf &conf)` receives the configuration being built.
Use `conf.GetModule(this)`. `Block` has verified `Get<T>()`, `GetBlock()`,
`GetBlocks()`, and `CountBlock()` APIs.

```cpp
void OnReload(Configuration::Conf &conf) override
{
	const auto &block = conf.GetModule(this);
	auto enabled = block.Get<bool>("enabled", "yes");
	// Parse and validate temporary state; commit only after success.
}
```

For complex configuration, parse -> validate -> commit is a vIRCio robustness
policy, not a formal core requirement. Rehashes that replace resources must
preserve a valid state until replacement succeeds where the subsystem allows.

## Object model

- `User`: live IRC user.
- `NickAlias`: persistent registered nickname.
- `NickCore`: persistent account, possibly with many aliases.
- `Channel`: live IRC channel.
- `ChannelInfo`: persistent registered channel; `c` is its live channel when one exists.

Never invent `RegisteredChannel`. Do not treat `User` as an account or `Channel`
as `ChannelInfo`.

## Lifetime and ownership

Core objects can disappear during events, callbacks, rehash, or module unload.
Define ownership, deletion path, and callback lifetime before retaining a
pointer. `Reference<T>` invalidates when its `Base` target dies.
`Serialize::Reference<T>` also queues updates on access and is the durable form
for serializable data. Do not delete core-owned objects through non-owning views.

## Extensible data

Verified types are `Extensible`, `ExtensibleItem<T>`,
`PrimitiveExtensibleItem<T>`, `SerializableExtensibleItem<T>`, and
`ExtensibleRef<T>`. An extensible item is a module-owned Service.

Use an extensible item for object-local state. Use `SerializableExtensibleItem`
for persistent data on a `Serializable`. `ns_register` provides the verified
persistent bool pattern:

```cpp
SerializableExtensibleItem<bool> unconfirmed;

// member initializer: unconfirmed(this, "UNCONFIRMED")
// use: nc->Extend<bool>("UNCONFIRMED");
```

Check extension name ownership and unload semantics. Prefer it over a parallel
`std::map` when the data belongs to an extensible core object.

## Serialization and persistence

Use `Serializable` for natural Anope persistent objects. Custom serializable
types need `Serialize::Type` and lifecycle review: loader calls
`Serialize::CreateTypes()` after module construction/configuration. Persist only
data with defined identity, migration, and unload/reload behavior.

## Protocol abstraction

Business modules use object APIs, modes, and `IRCDProto` methods/capabilities.
Do not emit raw `Uplink::Send` S2S traffic unless doing genuine protocol-layer
work with no public abstraction. Raw sends in `modules/protocol/inspircd.cpp`
are adapter implementation, not precedent for ordinary modules.

## InspIRCd 4+ integration

The current adapter constructs its protocol as `"InspIRCd 4+"` and requires
spanningtree protocol 1206+. Build only against that contract. Check advertised
capabilities and `IRCD` flags before optional behavior; add no v3 branches.

## Modes

Use `ModeManager` and `UserMode`/`ChannelMode`, not hand-written wire messages.
Resolve mode names/chars and support from actual adapter/network behavior. Use
normal user/channel mode APIs; methods explicitly marked internal do not notify
the IRCd.

## HTTP

`HTTP::Provider` is a Service and explicitly owns `RegisterPage` and
`UnregisterPage`. A module must unregister its page during rehash replacement
and destruction while the provider reference is valid. `Module::~Module` does
not prove this cleanup. Validate request input, authorization, and response
encoding/escaping for the relevant content type.

## JSON-RPC

`RPC::Event` is a `Service` registered under `RPC_EVENT` (`"RPC::Event"`). Its
verified virtual method is:

```cpp
bool Run(RPC::ServiceInterface *iface, HTTP::Client *client,
	RPC::Request &request) override;
```

Add a method by deriving `RPC::Event`, normally as a module member. Do not
invent `OnRPCEvent` or patch `jsonrpc.cpp` only to add a method.

```cpp
class ExampleRPCEvent final : public RPC::Event
{
public:
	ExampleRPCEvent(Module *owner) : RPC::Event(owner, "vircio.example", 0) { }

	bool Run(RPC::ServiceInterface *, HTTP::Client *, RPC::Request &request) override
	{
		auto &result = request.Root<RPC::Map>();
		result.Reply("ok", true);
		return true;
	}
};
```

`jsonrpc.cpp` dispatches `ServiceReference<RPC::Event>(RPC_EVENT, request.name)`,
passes positional input in `request.data`, and returns `RPC::Value`,
`RPC::Array`, or `RPC::Map` via `request.Root()`. Tokens use a `Bearer ` header,
base64-decoded token data, and method globs (including negative `~` globs).
Critically, the transport invokes `CanExecute` only when its token list is
nonempty: verify this source behavior and configuration before exposing an
endpoint. Never log tokens or Authorization headers.

## SASL and encryption

SASL protocol/service interfaces are Services and have `ServiceReference`s in
`include/modules/nickserv/sasl.h`. Use those interfaces instead of custom
protocol messages. Encryption providers are Services; use verified provider
operations and do not log plaintext credentials.

## Database and SQL

Use `Serializable` for Anope object persistence and `SQL::Provider` for
application SQL. `SQL::Query::SetValue` provides named values; the official
MySQL provider escapes marked values while building a query. Never interpolate
untrusted input directly into SQL.

`SQL::Interface` callbacks belong to a module and must remain safe when it
unloads. MySQL is a concrete async pattern: blocking work in a worker -> result
queue -> `Pipe` -> main-thread callback. Verify other providers independently;
do not assume all SQL backends use threads.

## Timers

`Timer(Module*, time_t)` associates a timer with a module; module destruction
calls `TimerManager::DeleteTimersFor`. `Tick()` returns `bool` to continue or
stop. Still design referenced objects and external work for invalidation.

## Sockets and threading

Treat normal core/object-model access as main-loop-affine unless source proves
cross-thread support. Anope is neither universally thread-safe nor universally
single-threaded. `Thread` offers exit signaling, `Start()`, and `Join()`;
`Exit()` still requires joining. A custom worker needs exit signaling,
cancellation, join, callback lifetime, and unload sequencing. Never detach a
worker. Use `mysql.cpp` as the concrete worker -> queue -> Pipe -> main-thread
pattern, not proof that arbitrary core use is safe in workers.

## Logging

Use `Log` with the semantically correct `LogType` and source/context. Separate
operator audit from diagnostics. Do not log protocol secrets, passwords, tokens,
database credentials, authentication data, or unnecessary private user data.

## Internationalization

Follow official code: wrap user-visible text in `_()` and use `CommandSource`
translation/reply helpers. Keep identifiers, configuration keys, Service names,
and RPC method names stable and untranslated.

## Security and trust boundaries

Validate IRC, HTTP, RPC, configuration, and SQL input. Check object existence
and lifetime after lookup, use the exact authorization model, reject malformed
input, and minimize disclosed errors. Never version, echo, or log passwords,
tokens, database credentials, uplink passwords, or private keys.

## CMake and build

Anope 2.1.27 requires C++17 (`CMAKE_CXX_STANDARD 17`), not C++20. The build
recursively scans `modules/`, and `modules/third/README` assigns that directory
to modules not shipped with Anope. Place compilable vIRCio source there.

A one-file module without external dependencies needs no extra CMake file. For
a dependency, copy a verified official inline pattern:

```cpp
/// BEGIN CMAKE
/// target_link_libraries(${SO} PRIVATE "dependency")
/// END CMAKE
```

Do not modify top-level `CMakeLists.txt` just for a normal third-party module.
Do not build, install, or operate runtime Services unless explicitly authorized.

## Coding style

Make code look like Anope 2.1.27: C++17, tabs, Allman braces, PascalCase
classes/methods, initializer lists, `final` where appropriate, `const`, early
returns, natural `auto`, range-for, and structured bindings. Raw pointers are
common non-owning views; use `unique_ptr` for explicit ownership when fitting.
Do not impose C++20, concepts, shared pointers everywhere, or external style.

## Porting legacy modules

Never port Anope 2.0 by changing includes until it compiles. Required flow:

    read the entire old module
    -> describe functional intent
    -> separate business logic, old Anope glue, old IRCd glue
    -> identify persistence and dependencies
    -> find a 2.1.27 native/official/Service/extension equivalent
    -> remove obsolete behavior
    -> redesign
    -> implement

Classify each part: `KEEP CONCEPT`, `REPLACE WITH CORE API`, `REPLACE WITH
OFFICIAL MODULE`, `REPLACE WITH SERVICE`, `REPLACE WITH EXTENSION`, `REPLACE
WITH SERIALIZATION`, `DELETE OBSOLETE COMPATIBILITY`, or `REIMPLEMENT`. Port
future Zombie intent, not historic architecture.

## Anti-patterns

Reject:

- `VENDOR` for vIRCio, or `EXTRA` as third-party synonym;
- `OnReload(Configuration::Conf *)`;
- manual attachment as a default in this baseline;
- `EVENT_ALLOW` as generic continuation;
- durable raw `Service*` where `ServiceReference` fits;
- `User` as account, `Channel` as `ChannelInfo`, or invented `RegisteredChannel`;
- long-lived raw pointers without analysis or parallel maps where extensibles fit;
- raw S2S in business modules or InspIRCd 3 compatibility;
- untrusted SQL interpolation;
- arbitrary core use in workers or a thread without cancellation/join;
- HTTP Page without unregistering;
- invented RPC hooks or patching `jsonrpc.cpp` for a method;
- secrets in logs;
- core patch before existing APIs are checked;
- silently using later 2.1 HEAD API.

## Implementation workflow

1. Confirm baseline and inspect primary source.
2. State behavior, ownership, trust boundary, and unload path.
3. Exhaust native feature, configuration, official module, and Service/API.
4. Design the smallest `THIRD` module in `modules/third/` if still needed.
5. Compare with official 2.1.27 modules.
6. Implement explicit rehash, lifetime, serialization, and callback rules.
7. Review permissions, validation, logging, and secret handling.
8. Run only authorized focused verification.

## Review checklist

- Is the feature already native, configurable, official, or a Service?
- Is every API/signature verified against 2.1.27?
- Does it use `THIRD`, correct placement, and no unnecessary core patch?
- Are live/persistent objects and reference lifetimes correct?
- Are configuration, unload, timers, HTTP pages, SQL callbacks, and threads safe?
- Are event return and priority semantics correct for the exact hook?
- Is authorization distinct from IRC oper, Services oper, account, and channel access?
- Are inputs validated and secrets omitted from logs?
- Does it avoid accidental InspIRCd 3 or raw-protocol coupling?

## Build/test checklist

When authorized, run the repository-prescribed build and test load, nominal
behavior, and a relevant error/permission path. Test valid rehash for
configurable modules and unload/reload safety for unloadable modules. For
InspIRCd-sensitive work test 4+ integration; for distributed state test two
peers, burst, split/relink, and deletion; for SQL/threaded work test failure
and provider/module removal paths. Do not install, start, stop, or change
`/home/vircio/anope` without authorization.

## Primary source map

| Concept | Primary source |
| --- | --- |
| Module API/events | `include/modules.h` |
| Loader/lifecycle | `src/modulemanager.cpp`, `src/module.cpp` |
| Services | `include/service.h` |
| References | `include/base.h`, `include/serialize.h` |
| Commands | `include/commands.h`, `src/command.cpp` |
| Configuration | `include/config.h`, `src/config.cpp` |
| Accounts | `include/account.h` |
| Registered channels | `include/regchannel.h` |
| Extensible data | `include/extensible.h` |
| Serialization | `include/serialize.h`, `src/serialize.cpp` |
| Protocol | `include/protocol.h`, `modules/protocol/inspircd.cpp` |
| Modes | `include/modes.h` |
| HTTP | `include/modules/httpd.h` |
| RPC | `include/modules/rpc.h`, `modules/rpc/jsonrpc.cpp`, `modules/rpc/rpc_system.cpp` |
| SASL/encryption | `include/modules/nickserv/sasl.h`, `include/modules/encryption.h` |
| SQL | `include/modules/sql.h`, `modules/extra/mysql.cpp` |
| Timers | `include/timers.h` |
| Threading | `include/threadengine.h`, `src/threadengine.cpp` |
| Build/third-party placement | `CMakeLists.txt`, `modules/CMakeLists.txt`, `modules/third/README` |
