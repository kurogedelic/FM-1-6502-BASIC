#!/usr/bin/env python3
"""check_package.py PKG.fwsc APP.bin PRODUCT : structural checks of a built package, using
felucca-bsp's own installer code (tools/fm1_install.py) to parse it. Not a substitute for a
device test."""
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "third_party/felucca-bsp/tools"))
import fm1_install as I  # noqa: E402

pkg, app, product = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
raw = pkg.read_bytes()
ident = I.product_of(raw)
assert ident == product, f"identity {ident!r} != {product!r}"
assert I.re.fullmatch(r"[^_]+_\d+", ident)
image = I.logical_image(raw)
assert len(image) == len(raw) - I.BLOCKS, (len(image), len(raw))
assert app.stat().st_size <= 0x8DFBC
print(f"package ok: {pkg.name} {len(raw)} B, identity {ident}, logical image {len(image)} B, app {app.stat().st_size} B")
