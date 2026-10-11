# Third-party sources

Both components are MIT licensed (Copyright (c) Microsoft Corporation). The
license text is kept next to each upstream file and must be preserved.

| Directory | Upstream | Commit | Files |
| --- | --- | --- | --- |
| `BASIC-M6502/` | https://github.com/microsoft/BASIC-M6502 | 7460af2c03ae19c0e60ff327489229d2005b9357 | `m6502.asm` (original MACRO-10 source, reference only), `LICENSE` |
| `BASIC-M6502-CC65/` | https://github.com/rene6502/BASIC-M6502-CC65 | fa31daddafa6f1280efce9d66f60a4d2bf4ea474 | `m6502-min.s` (ca65 conversion, minimal hardware dependencies), `LICENSE` |

The files are stored **unmodified**. FM-1 specific changes are applied at build
time as a patch (`rom/m6502-fm1.patch`), so the upstream text can be diffed and
refreshed. Upstream's `m6502-min.s` assembles (with ca65 V2.18, `--feature
force_range`) to a binary with MD5 `b2b05e5bcb6dedab4279181f0c4b9e6d`, which
matches the value published by the cc65 port.
