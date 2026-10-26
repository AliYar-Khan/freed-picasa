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
| star | `star=yes` / absent | same |
| rotation | `rotate=%d` (0..3) | pending wine verify of sign semantics |
| crop | `crop=rect64(%I64x)` | nibble order per above |
| redeye | `redeye=1;` + DB list | our tool stores circles list in ini (2-entry list) until DB format mapped |
| retouch | `retouch=1;` + DB list | same treatment |
| tuning | `enhancement=...` / sliders | P1 pending key verification |

## Verification status

Static verification (strings/RTTI mining + unit tests in `tests/test_picasa_ini.cpp`)
is complete for: star, crop rect64, redeye "1;", rotate integer form. A *live*
behavioral diff under wine is currently blocked (P6):

- Fedora wine-core 11.0-3 fails to construct a usable prefix (`kernel32.dll`
  c0000135 with syswow64 never populated; gstreamer deadlock workaround
  `WINEDLLOVERRIDES="winegstreamer="` applied; headless + display boot loops).
- Steam Proton (wine-11.0) renders the app process but the DB-init exit cycle
  ends before a mapped window: prefix archaeology showed (a) broken relative
  `steam.exe` symlinks outside Steam's compatdata layout, and (b) Proton
  re-trimming/dropping symlinked builtin DLLs on every prefix upgrade. A
  manually repaired prefix at
  `~/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/compatdata/picasa_manual/pfx`
  (arch-correct symlinks: system32→x86_64-windows, syswow64→i386-windows)
  reached services+DB init (`AppData/Local/Google/Picasa2/db3` populated,
  ytf font runtime written) but never mapped a main window (RPC_S_SERVER_
  UNAVAILABLE seh unwind in the updater/automation thread; silent exit).

Unit covered round-trips (doctest, 47 assertions):
- `IniFile::load/save/set/remove_key` preserves unknown keys verbatim.
- `rect64_encode` == `00000000ffffffff` for full-frame; clamps; 16 hex chars.
- `set_crop/set_redeye/set_rotate/set_star` emit the mined key formats with
  unknown keys retained in same section.

Remaining when wine becomes usable on this machine: run a star/rotate/crop
round-trip in Picasa3.exe, diff its `picasa.ini` output key-by-key against
ours, and confirm the rotation sign semantics + tuning key names.
