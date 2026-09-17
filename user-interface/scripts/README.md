# UI source acquisition

These scripts download the upstream UI projects registered in `user-interface/sources/` into a local `vendor/` directory.

## Supported hosts

- Linux
- macOS
- Windows PowerShell

## Linux / macOS

From the repository root:

```bash
bash user-interface/scripts/clone-ui-sources.sh
```

Optional destination:

```bash
bash user-interface/scripts/clone-ui-sources.sh /path/to/ui-sources
```

## Windows

From PowerShell at the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\user-interface\scripts\clone-ui-sources.ps1
```

Optional destination:

```powershell
.\user-interface\scripts\clone-ui-sources.ps1 -Root 'C:\src\ui-sources'
```

## Sources

The scripts currently acquire:

- Cockpit: `https://github.com/cockpit-project/cockpit.git`
- OpenBao: `https://github.com/openbao/openbao.git`
- GitLab FOSS: `https://gitlab.com/gitlab-org/gitlab-foss.git`

The checkout is intentionally shallow (`--depth 1`) to keep the local acquisition lightweight. The scripts update an existing checkout with `fetch` and fast-forward-only `pull`.

## Provenance and licensing

These are **reference sources**, not automatically incorporated into the White Edition product. Keep each upstream repository's license, copyright notices, and provenance intact. Review the relevant license before copying or adapting any source into `user-interface/`.

For reproducible production builds, replace the moving shallow checkout with a pinned commit/tag manifest and verify the downloaded commit before use.
