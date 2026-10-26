# Picasa 3.9 — Data & Storage formats (RE notes)

Derived from static analysis of `picasa-3-9-141-259.exe` (NSIS payload: Picasa3.exe
10.1 MB, 32-bit; Picasa3i18n.dll; MovieThumb.exe; qtsupport.dll; npPicasa3.dll).
Strings mining verified formats; wine behavior-diff confirms semantics (see
"Verification" section for each key as we test it).

## Installer payload layout

| File | Size | Role |
|---|---|---|
| Picasa3.exe | 10.1 MB | entire application, Win32, links ADVAPI32 AVIFIL32 COMCTL32 COMDLG32 DDRAW GDI32 IMM32 IPHLPAPI KERNEL32 MPR mscms MSVFW32 ole32 OLEAUT32 SHELL32 SHLWAPI urlmon USER32 VERSION WININET WINMM WINSPOOL WINTRUST WS2_32 |
| Picasa3i18n.dll | 26.9 MB | resource-only DLL: all localization/UI strings |
| MovieThumb.exe | 715 KB | movie thumbnailer helper |
| qtsupport.dll | 100 KB | QuickTime support for movies |
| npPicasa3.dll | 59 KB | NPAPI browser plugin |

Internal frameworks observed in RTTI/strings: `glimmer` (render/UI), `yt*` prefix
(internal namespace), `ILocalDatabase` (photo DB), `CFaceDetector` (face engine),
collage/slideshow/retouch/crop engines, `JSON_parser_*` exports (bundled json).

## picasa.ini (per-folder metadata)

Stored in each scanned folder. Sections listed per image filename (e.g.
`[IMG_0001.jpg]`), plus system sections. Observed per-image keys in binary
strings (Picasa 3.9.141):

- `star=yes`
- `keywords=%s` (comma/space separated terms)
- `crop=rect64(%s)` where `%I64x` — 64-bit rect encoding:
  16 nibbles packed as `y0 x0 y1 x1` with each value scaled 0..0xFFFF over the
  image axis. (OBSOLETE/unified elsewhere: Picasa2 used CropRectangleN vars.)
- `redeye=1;` / `retouch=1;` — tool-used markers; parameters are stored in the
  local DB (`%d,%d,%d,%d,%d;` fragments, `deye`/`rtch` mode tags).
- `picnik=1;` — online-editor marker (Picnik was Google's web editor).
- `flipped(0)`, `rotate(...)` — rotation state variants.
- `geotag` as `geotag=%lf,%lf` (lat,long).
- `.picasaoriginals` / `Originals` subfolder holds pre-edit backups.

Folder-level & album metadata (`*.pal`-style `albums` files):

```
[Picasa]
name=%s
description=%s
location=%s
category=%s
date=%f
```

Local database (custom, not sql): records tagged `]album`, `]star`,
`]screensaver`, `]geo`, `]hidden`, `]edited`, `]revertable`,
`]history:email/output/upload/export`; `]contacts\` for faces; compact lock
files `.lck`/`_lock`.

## Our reimplementation contract

We use the *same file names and formats* where behavior-parity matters:

1. `picasa.ini` — generic order-preserving INI (`src/data/picasa_ini.*`),
   sections per filename; write only keys we touched, keep all else verbatim.
2. Thumbnails: Picasa stored its own `db3` cache; ours uses
   `~/.cache/open-picasa/*.jpg` keyed on path-hash (internal only).
3. Albums/contacts: parsed read-only in MVP.

Key mapping used by open-picasa (see `src/data/picasa_ini.h` docs):

| Edit op | picasa.ini key(s) | Notes |
|---|---|---|
| star | `star=yes` / absent | **live-verified** (written by real Picasa 3.9.141) |
| rotation | `rotate=rotate(%d)` (0..3) | **live-verified** — value is the *string* `rotate(N)`, not a bare integer. Our emitter's `rotate=%d` is ignored by real Picasa; emitter fix pending |
| crop | `crop=rect64(%I64x)` | nibble order per above; live ground truth still needed |
| redeye | `redeye=1;` + DB list | our tool stores circles list in ini (2-entry list) until DB format mapped |
| retouch | `retouch=1;` + DB list | same treatment |
| tuning | `enhancement=...` / sliders | P1 pending key verification |
| (any edit) | `backuphash=%d` | added by Picasa with the first edit in a section (sample_0 → `backuphash=60931`). Not crc32/adler32/md5-low16/sha1-low16/byte-sum of the file (all computed, none match). Likely hash of name/size/mtime or internal token — gates section trust, see below |

## Verification status

**Wine oracle is LIVE** (2026-10-27): real Picasa 3.9.141 runs under Steam Proton
and is used as the behavioral reference. Setup recipe and UI map live in
`docs/oracle-wine-setup.md`; raw traces in `re/picasa_run*.log`.

### Live-verified writer format (captured byte-for-byte)

Real Picasa 3.9.141 was driven via UI injection (select photo → Batch Edit →
Rotate Clockwise, then star button). The ini it wrote (`cat -A`, `^M$` = CRLF):

```
[Picasa2]^M$
encoding=UTF-8^M$
^M$
[sample_1.jpg]^M$      ← foreign section we staged earlier, preserved verbatim
star=yes^M$
^M$
[sample_0.jpg]^M$      ← section created by Picasa on first edit
rotate=rotate(1)^M$
backuphash=60931^M$
star=yes^M$            ← appended later by the star button (key order kept)
```

Facts established:

- CRLF endings; `[Picasa2]` + `encoding=UTF-8` header; sections named with the
  exact filename; new sections appended in edit order; writer merges/keeps
  unknown keys and foreign sections.
- **Rotation value syntax is `rotate=rotate(1)`** — the parsed value is the
  string `rotate(N)` (matches mined `rotate(%d)` format string). A bare
  `rotate=1` is silently ignored by the reader.
- `backuphash=<int>` is written with the first edit in a section. Value for
  sample_0.jpg (70144 B) is 60931 — no match against crc32/adler32 low/high16,
  md5/sha1 low16, or byte-sum%65536 of the file content. TBD.
- **Section trust gate**: our staged `[sample_1.jpg] star=yes` (no backuphash)
  was *preserved verbatim* by Picasa's writer, but was *never applied* —
  thumbnail showed no star badge and no rotation across multiple fresh-DB
  restarts, while Picasa-edited sections (with backuphash) applied instantly.
  Strong hypothesis: the reader requires a valid per-section hash (backuphash
  or equivalent) before trusting any edit state in that section. TO VERIFY:
  stage `rotate=rotate(1)` + correct `backuphash` and restart with fresh DB.

### Scan-time ini behavior (WINEDEBUG=+file trace)

- Scanner thread enumerates the folder, lists `picasa.ini` as a dir entry.
- Probes `.picasa.ini` (dot variant) with GetFileAttributesW (we didn't create
  one — unknown whether it takes precedence).
- Opens `picasa.ini` GENERIC_READ and reads the **full file** repeatedly
  (6× during a 4-image folder scan — looks like per-image re-parse).
- With an untrusted ini present, the scan still completes and commits the DB
  (db3/*.pmp written ~5 min after launch; ioqueue holds earlier writes) — but
  no edit state is applied.

### db3 PMP artifacts (live-dumped)

- PMP header starts with float32 `1.0` (`cd cc cc 3f`) + u16 count + tag
  `32 13` (`0x1332`), then per-record blocks; see `docs/reference`.
- `imagedata_height.pmp` / `imagedata_width.pmp`: per-image u32 (900/1200).
- `imagedata_edit_height.pmp` / `imagedata_edit_width.pmp`: same layout,
  zeros (unedited images); presumably updates on edit.
- `imagedata_originfast.pmp`: per-image u32 content fingerprint
  (sample_3 → 0x0cf6ea35); zeros until first processed. Candidate source for
  backuphash/token comparisons.
- `starlist.txt`, `tags.txt`, `saverlist.txt`: empty until starred/tagged.

### Static verification (unit tests, doctest — 47 assertions)

- `IniFile::load/save/set/remove_key` preserves unknown keys verbatim.
- `rect64_encode` == `00000000ffffffff` for full-frame; clamps; 16 hex chars.
- `set_crop/set_redeye/set_star` emit the mined key formats with unknown keys
  retained in same section. `set_rotate` emits the WRONG form (`rotate=%d`);
  must be changed to `rotate=rotate(%d)` — #1 next step.

### Open items for the ini round-trip

1. Fix `set_rotate` to `rotate=rotate(N)`; re-run smoke/unit.
2. Crack `backuphash` (collect ≥2 data points: edit a second file, re-edit
   the same file, check invariance vs file bytes vs path/mtime).
3. Verify staged sections WITH backuphash are applied on fresh-DB restart.
4. Capture crop + redeye ground truth via the edit-view tools (drag needs
   press/rel subcommands in xctl).
5. Check `.picasa.ini` dot-variant precedence.
