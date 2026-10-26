# Wine oracle — live Picasa 3.9.141 setup (Fedora 44, KDE Wayland)

Operational recipe for running real Picasa 3.9.141 under Steam Proton as the
behavioral reference for open-picasa. Working state as of 2026-10-27.

## Why the app showed the "you need to reinstall" dialog

Picasa's startup calls `ytResMgr::LoadError` → error dialog *"This application
encountered errors starting up… you need to reinstall"* when the `runtime\`
resource tree is missing. Extracting only the loose installer files
(Picasa3.exe, Picasa3i18n.dll, MovieThumb.exe, qtsupport.dll, npPicasa3.dll)
is NOT enough — the NSIS payload contains a full install tree.

## Correct payload: the original installer

- Installer: `picasa-3-9-141-259.exe` (repo root), NSIS-3 Unicode.
- `7z x picasa-3-9-141-259.exe -o/tmp/picasa_install` → 633 files:
  `runtime/` (~5.5 MB: `constants.ui`, `respack.yt` 3.8 MB, `.fen` UI files,
  `filters.txt`, `bezier.txt`, `filterdesc.xml`, `defaults.ini`, `geotag/`,
  `missing.jpg`, `splashbk.jpg`, `xhairs.png`…), `i18n/`, `plugins/`, `web/`,
  `buttons/`, `update/`, `PicasaPhotoViewer.exe`, `GPhotos.scr` (in `$SYSDIR`).
- **`runtime/winedisable.txt`** is Google's kill-list of build numbers that
  refuse to run under wine (40225 40192 40019 40372 40350 40373 40368 40370
  40371 40284 40393 40276 40432). DELETE IT wherever it gets installed, or the
  app aborts.

## Install (silent) into the Proton prefix

```bash
WPFX=/home/aliyarkhan/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/compatdata/picasa_manual/pfx
# run the NSIS installer silently through Proton wine; installs to
# C:\Program Files (x86)\Google\Picasa3\ + registry (viewer Shell\Open)
rm -f "$WPFX/drive_c/Program Files (x86)/Google/Picasa3/runtime/winedisable.txt"
```

Launch script pattern (`/tmp/opencode/run13.sh`; timeout the tool call — the
detached child makes the shell hang otherwise):

```bash
C=/home/aliyarkhan/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/common
WPFX=/home/aliyarkhan/.var/app/com.valvesoftware.Steam/.local/share/Steam/steamapps/compatdata/picasa_manual/pfx
EXE="C:\\Program Files (x86)\\Google\\Picasa3\\Picasa3.exe"
cd "$C" && setsid nohup env -u STEAM_COMPAT_DATA_PATH DISPLAY=:0 \
  WINEPREFIX=$WPFX WINEDEBUG=+file \
  ./SteamLinuxRuntime_sniper/_v2-entry-point --verb=run -- \
  "./Proton - Experimental/files/bin/wine" "$EXE" \
  > log 2>&1 < /dev/null & exit 0
```

- `WINEDEBUG=+file` shows the scanner's CreateFileW/ReadFile activity (ini
  reads, image reads). `+seh` shows exceptions (RPC_S_SERVER_UNAVAILABLE
  0x6ba dispatch in an early thread is benign — app continues).
- Window id changes every launch; query dynamically.

## Killing wine safely

`pkill -f wine` self-kills the calling shell. Use:

```bash
ps -eo pid,comm | awk '$2=="Picasa3.exe"||$2=="wineserver"{print $1}' | xargs -r kill -9
```

## Making the library populate (scan)

Picasa scans watched folders; the watch list lives in the DB dir:

- `…/users/steamuser/AppData/Local/Google/Picasa2/db3/watchedfolders.txt`
  — one entry per line: `+C:\users\steamuser\Pictures\opd_samples\r\n`
  (written by Tools → Folder Manager; we write it directly).
- `db3/scanlist.txt` — drive watcher list (`+C:\r\n+Z:\r\n` default). The
  `+Z:\` entry makes it watch the ENTIRE Linux FS through dosdevices/z: —
  first scan crawls forever. Keep only `+C:\`.
- Wine user dirs are plain `Pictures` (no `My Pictures`); Picasa's
  "Import to: My Pictures" still maps there.
- Deleting the whole `AppData/Local/Google/Picasa2` dir = fresh DB.
- DB commits land late (~5 min) via `ioqueue/*.ioq`; starlist/tags stay empty
  until used.

## Input injection (XTest) — KWin Remote Control permission

KWin on Wayland prompts per app: *"Remote Control — <app> is asking to
control input devices"* + checkbox "**Always allow apps claiming to be
<name>**" + Allow/Deny.

- **Allow without the checkbox = per-process grant.** Every new process
  (each script invocation) is silently blocked until re-prompted. This made
  earlier experiments behave randomly.
- Check the box once per tool name, then everything works: synthetic
  motion, buttons and keys.
- Keyboard focus quirk: in the library view, keys go to the search box;
  click a thumbnail first (or clear search with its ✕ at ~(1306,66)).

Tools (source kept in /tmp/opencode, rebuild on demand):

- `xctl` (`xctl.c`, gcc -lX11 -lXtst): `named <substr>` → find window by
  _NET_WM_NAME; `activate <wid>` (EWMH client message, prints active-ok);
  `click x y` (move → 120 ms → press → 80 ms → release);
  `dbl x y` (real double-click, 140 ms gap — two separate processes are
  too slow and read as two single clicks); `key <keysym>`;
  `combo <mod> <key>`; `active`.
- `probe` (`probe.c`): move pointer to x,y, print real cursor pos +
  window-under-cursor — use to verify motion permission is live.

Before every injection: `xprop -root _NET_ACTIVE_WINDOW` must equal the
Picasa window id, else abort.

## UI map (library & edit views, maximized 1366x768)

Menubar: File Edit View Folder Picture Create Tools Help (y≈39).

Library view:

- Folder toolbar (y≈124): Upload, etc. Filters toolbar (y≈68): star filter
  at (559,68) filters starred-only.
- Left tree: Albums (N) / Recently Updated (N) / Folders (N) / folders.
- Thumb grid starts (332,255), spacing ≈188 px (thumb2 ≈ 520,255).
- Bottom action row (selection present): star (513,620), rotate CCW
  (555,620), rotate CW (589,620); Email (668,667), Print (720,667),
  Export (777,667); big green "Upload to Google Photos" (~496-635, 650-685).
- Status strip above: "filename | mtime | WxH pixels | KB".

Picture menu (library or edit view): View and Edit (Ctrl+3), Batch Edit ▸,
Undo All Edits, Hide, Unhide, Reset Faces, Properties (Alt+Enter).
**Rotate lives in Batch Edit ▸**: Rename (F2), Rotate Clockwise (Ctrl+R),
Rotate Counterclockwise (Ctrl+Shift+R), Auto Contrast, Auto Color, I'm
Feeling Lucky, Sepia, Sharpen, Warmify, Film Grain, Black and White, Auto
Red Eye Correction, Show Text, Hide Text.

Edit view (double-click a photo): "Back to Library" (60,67); left panel
Basic Fixes: Crop (54,152), Straighten, Redeye (214,152), I'm Feeling Lucky,
Auto Contrast, Auto Color, Retouch, Text, Fill Light slider; Undo/Redo;
"Histogram & Camera Information" ("No EXIF data available." for our
samples); caption bar "Make a caption!" under the image. Bottom action row
same buttons at y≈648.

## Experiment log (ini round-trip, 2026-10-27)

- Our staged `picasa.ini` (bare `rotate=1`, crop, redeye, star variants; LF
  then CRLF; with/without `[Picasa2] encoding=UTF-8`; with/without `size=`)
  was opened and fully read by the scanner (6×) but NEVER applied — thumbs
  stayed landscape, no star badge, starlist.txt empty.
- UI-driven edits (Batch Edit → Rotate Clockwise; star button) wrote the ini
  immediately: `rotate=rotate(1)`, `backuphash=60931`, `star=yes`; rotated
  thumb went portrait in grid + tray; selection tray/status updated.
- Picasa's writer preserved our staged foreign section verbatim but its
  edits remain unapplied → section-trust gate (backuphash) hypothesis.
- Raw byte captures + screenshots: /tmp/opencode/rt_s*.png (session data,
  re-create as needed); trace logs under `re/picasa_run*.log`.

## Current in-flight state (stop point)

- Picasa running (window id queried dynamically), Import view accidentally
  opened (Back to Library returns).
- picasa.ini in `…/opd_samples/` currently has sample_0: rotate(1)+
  backuphash 60931+star; sample_1: star=yes (staged, untrusted).
- Next steps live in `docs/formats/picasa-ini.md` → "Open items".
