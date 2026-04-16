# ReadMarkable Installer

A cross-platform desktop wizard that installs ReadMarkable on a reMarkable Paper Pro Move (chiappa) over USB. Wraps the install of XOVI + AppLoad + ReadMarkable into a single double-click flow with persistence across reMarkable reboots.

## End users

### Download

Grab the installer for your platform from the latest [**installer release**](https://github.com/viraya/ReadMarkable/releases?q=installer-v):

- **Windows:** `ReadMarkableInstaller-setup.exe` + `.sha256`
- **macOS (Apple Silicon + Intel):** `ReadMarkableInstaller-<version>.dmg` + `.sha256`

### System requirements

- reMarkable **Paper Pro Move** (chiappa), firmware 5.6.x.
- On your reMarkable: **Settings → General → Storage → USB web interface** turned ON.
- USB cable (or LAN  -  the wizard has a "Connect by IP" button).
- Host: Windows 10/11 (64-bit) or macOS 12+ (Apple Silicon or Intel).

### Windows  -  why does it warn me?

The installer is **unsigned**. Windows SmartScreen will say *"Microsoft Defender SmartScreen prevented an unrecognized app from starting"* on first run. It does not mean the file is malicious  -  it means Microsoft hasn't seen it enough times yet to build reputation.

**To run it anyway:**

1. When SmartScreen appears, click **More info** (text under the warning).
2. Click **Run anyway** (the button that appears below the publisher line).

**To verify it's the file we published:**

```powershell
Get-FileHash -Algorithm SHA256 ReadMarkableInstaller-setup.exe
```

Compare against the `.sha256` file published on the GitHub Release page. If they match, the file is the one we built.

### macOS  -  first-run walkthrough

The `.dmg` is signed + notarized with our Developer ID, so Gatekeeper should accept it without the right-click-Open dance. If you ever see *"ReadMarkable Installer cannot be opened because the developer cannot be verified"*:

1. Right-click the app → **Open**.
2. Click **Open** in the dialog that appears.

This only needs to be done once.

### What the installer does under the hood

1. Connects to your reMarkable over SSH (via USB or LAN IP).
2. Uploads and runs [**XOVI**](https://github.com/asivery/rm-xovi-extensions) (a community-maintained extension loader for xochitl) + [**AppLoad**](https://github.com/asivery/rm-appload) (XOVI extension that adds a third-party-app sidebar), plus the ReadMarkable binary + fonts + launcher icon.
3. Writes a small systemd service (`xovi-activate.service`) to `/usr/lib/systemd/system/` so XOVI stays active across reboots.
4. Restarts `xochitl`. A **ReadMarkable** tile appears on your home screen.

Your device continues to work normally  -  reMarkable Cloud sync, OTA firmware updates, notebooks and handwriting are all untouched.

### Uninstall

Re-run the installer. When it detects ReadMarkable is already installed, it offers an **Uninstall** path that removes all installed files, persistence units, and the installer's SSH key, then restarts xochitl. Your reading data is preserved by default.

### Diagnostics

If install fails, the **Save diagnostics…** button on the final page produces a `.tar.gz` bundle (session log, pinned versions, OS info, device-state JSON) suitable for attaching to a [GitHub Issue](https://github.com/viraya/ReadMarkable/issues). No telemetry is collected and nothing is uploaded automatically.

## Developers

### Prerequisites

| Tool | Min version | Windows | macOS |
|---|---|---|---|
| CMake | 3.20 | `winget install Kitware.CMake` | `brew install cmake` |
| Ninja | latest | `winget install Ninja-build.Ninja` | `brew install ninja` |
| Qt 6 | 6.7.x | `aqtinstall` (see below) | `aqtinstall` or `brew install qt@6` |
| libssh2 | 1.11 | `vcpkg install libssh2` | `brew install libssh2` |

#### Installing Qt via `aqtinstall` (cross-platform unattended)

```bash
python -m pip install aqtinstall
# Windows MSVC
python -m aqt install-qt windows desktop 6.7.3 win64_msvc2019_64 -O C:\Qt
# macOS clang
python -m aqt install-qt mac desktop 6.7.3 clang_64 -O ~/Qt
```

Set `QTDIR` to the resulting `Qt/<version>/<arch>/` directory before invoking CMake.

### Build

```bash
# Windows
set QTDIR=C:\Qt\6.7.3\msvc2019_64
cmake --preset windows-msvc-debug
cmake --build build/windows-msvc-debug

# macOS
export QTDIR=~/Qt/6.7.3/clang_64
cmake --preset macos-clang-debug
cmake --build build/macos-clang-debug
```

Run:
- Windows: `build/windows-msvc-debug/readmarkable-installer.exe`
- macOS:   `open build/macos-clang-debug/readmarkable-installer.app`

### Test

```bash
cd build/<preset>
ctest --output-on-failure
```

### Bumping pinned XOVI / AppLoad versions

```bash
bash installer/scripts/refresh-pins.sh
git add installer/config/pinned-versions.toml
git commit -m "chore(installer): bump pinned XOVI to vNN-DDMMYYYY"
```
