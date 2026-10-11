; FM-1 6502 BASIC: wrapper around Microsoft BASIC-M6502 (via the cc65 port).
;
; The BASIC interpreter itself is the unmodified upstream conversion
; third_party/BASIC-M6502-CC65/m6502-min.s (MIT, Copyright (c) Microsoft
; Corporation) plus the small banner patch rom/m6502-fm1.patch, which the
; Makefile applies into build/m6502-fm1.s.
;
; Entry points used by the FM-1 platform layer (rom/kernal.s):
.export STOP, INIT, FCERR
.include "m6502-fm1.s"
