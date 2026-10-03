# vIRCio Anope Engineering

## Project scope

This repository is the vIRCio fork of Anope for InspIRCd 4.x. Keep the fork as close to upstream as possible:

    native feature -> configuration -> official module -> Service/API -> custom vIRCio module -> core patch last

## Repository layout

- `vIRCio/conf/`: versioned network configuration and documentation.
- `vIRCio/modules/`: vIRCio module documentation, inventory, and policy.
- `modules/third/`: native build location for compilable third-party/vIRCio C++ modules; CMake recursively scans `modules/`.

Do not expect CMake to build a module placed only in `vIRCio/modules/`.

## Production baseline

- Anope 2.1.27, commit `1dce1fd39a99016c6dc6d3d4a7c5ee11212dd02e`.
- vIRCio development branch: `vIRCio-2.1`.
- Clean upstream tracking branch: `2.1`.
- Target IRCd: InspIRCd 4.x.

Do not silently use APIs from a later 2.1 HEAD.

## Mandatory module-development skill

Before developing, porting, reviewing, or debugging a module, read and follow:

    .opencode/skills/anope-module-development/SKILL.md

`AGENTS.md` is repository policy; the skill is the source-backed API and lifecycle guide.

## Source of truth

The checked-out 2.1.27 source wins over old comments, tutorials, forums, legacy modules, and memory. Check headers and comparable official modules. If a rule is unproven, say to verify it in target source rather than inventing it.

## Upstream-first policy

Use a native feature, configuration, official module, or published Service/API before adding vIRCio code. A core change needs explicit proof that those paths do not meet the requirement.

## Version discipline

Preserve the closed 2.1.27 baseline and its tags. `2.1` follows upstream; vIRCio-specific work belongs on `vIRCio-2.1`. Port legacy behavior by intent, not by line-for-line architecture.

## vIRCio configuration policy

Version appropriate network configuration in `vIRCio/conf/`. Do not copy Anope 2.0 configuration wholesale. Never version passwords, tokens, database credentials, uplink passwords, or private keys.

## Custom module placement

Place compilable custom C++ source under `modules/third/`. Ordinary vIRCio modules use `THIRD`, never `VENDOR`; `EXTRA` is independent classification, not a synonym for third-party. Keep module documentation and inventory under `vIRCio/modules/`.

## Legacy porting policy

Read the entire old module, state its functional intent, find 2.1.27 equivalents, and redesign it using current APIs. Preserve needed behavior, not obsolete Anope or IRCd glue. Do not mechanically port historical Zombie architecture.

## Architecture rules

- Use object and protocol APIs; business modules must not send raw `Uplink::Send` S2S traffic.
- Use `ServiceReference<T>` for durable inter-module services and analyze non-owning pointer lifetime.
- Distinguish live `User`/`Channel` from persistent `NickCore`/`ChannelInfo`.
- Follow actual event contracts: `EVENT_ALLOW` is not generic continuation, and the current loader auto-attaches modules to event vectors.
- Use serialization and extensible data when appropriate; avoid parallel state maps when an extension belongs to an object.

## Security and secrets

Treat IRC, HTTP, RPC, and SQL input as untrusted. Validate authorization at the correct layer, bind/escape SQL values, and never log or reproduce secrets unnecessarily.

## Build/runtime boundaries

The runtime is `/home/vircio/anope`; the external operational document is `/home/vircio/anope.md`. Do not alter either, install, or start/stop Services without explicit authorization. Do not change runtime configuration as part of source work.

## Required verification

Before proposing module code, inspect exact target headers, lifecycle implementation, and comparable official modules. For authorized code changes, verify build and focused behavior, including reload/unload safety, permissions, and relevant InspIRCd 4+ integration.

## Git discipline

Inspect `git status --short` before editing and preserve unknown work. Do not commit, push, add, reset, rebase, clean, or move historical tags without explicit authorization.

## Definition of done

The smallest native-style solution is in the correct location, uses verified 2.1.27 APIs, has reviewed ownership/unload/configuration paths, contains no secrets, passes authorized verification, and leaves upstream core untouched unless a core exception was explicitly approved.
