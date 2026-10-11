# Implementation status

See `docs/STATUS.md` for the authoritative, current list. Summary:

- Host side is complete enough to boot and run Microsoft BASIC-M6502 on the
  project's 6502 core, with automated end-to-end tests (`make basic-test`).
- FM-1 hardware integration (LCD, keys, JieLi firmware, SysEx install) is not
  done and has not been tested on a device.

Safety: do not flash this project as firmware. No firmware image is produced yet.
The virtual I/O registers at `$D000/$D001` are project-specific, **not Apple II I/O**.
