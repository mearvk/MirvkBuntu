# Open-Source UI Source Registry

This directory records and, where practical, vendors open-source UI foundations for the White Edition authority/source-control interface.

## Selection

The initial best-of-breed references are:

1. **Cockpit** — operating-system administration, storage, processes, logs, networking, virtualization, and host discovery. Cockpit describes itself as an interactive server administration interface that operates directly against a Linux system. Its repository lists LGPL-2.1+, GPL-3.0+, BSD-3-Clause, CC-BY-SA-3.0, and MIT material, so any vendored code must retain its individual license notices. Source: https://github.com/cockpit-project/cockpit
2. **OpenBao UI** — identity-oriented access control, policy, secrets, auditability, and a dedicated web UI. The OpenBao repository states that its UI is under `ui/` and documents its Ember application. The project is MPL-2.0. Source: https://github.com/openbao/openbao
3. **GitLab Web IDE / UI patterns** — source-control-centric editing, change review, branch selection, staging, commits, and merge-request workflows. The Web IDE documentation explicitly describes source-control operations including modified-file review, branch creation/switching, and commits. Source: https://docs.gitlab.com/user/project/web_ide/

## Important provenance rule

These projects are **references and source candidates**, not a declaration that their code has been incorporated into Ubuntu Determinant. Do not copy source files into the project without preserving the applicable license, copyright notices, and dependency provenance.

The White Edition should preferentially implement its own UI against the established `.asysma` Host/System Contract and use these projects as architectural references.

## Authority model

The desired White Edition interface combines the strongest relevant ideas:

```text
Cockpit
  → host/system administration

OpenBao
  → identity, policy, authorization, audit

GitLab
  → source/change control

White Edition
  → unified authority + source provenance + execution evidence
```

The resulting UI should be user-centered without making the user's identity itself a source of unlimited authority. Every consequential action remains bounded by capability, authorization, policy, provenance, and verification.

## Acquisition policy

Because these projects are large and multi-license, this repository records source locations and licensing before any vendoring. A future vendoring step should use Git submodules, pinned upstream commits, or a license-preserving source import rather than an untracked copy.
