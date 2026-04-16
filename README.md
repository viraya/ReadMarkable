# ReadMarkable

A native EPUB reader for the **reMarkable Paper Pro Move** (chiappa). Built with the Chiappa SDK.

Reading EPUBs on a reMarkable shouldn't mean waiting 20 seconds for a font size change. The stock reMarkable converts EPUBs to PDFs internally, making every layout change painfully slow. ReadMarkable renders EPUBs natively on the e-paper display  -  font changes, margin adjustments, and page turns are instant.

## Features

- **Native EPUB rendering** via CREngine  -  reflowed text, table of contents, footnotes, hyperlinks, images
- **Instant font and layout changes**  -  font size, font family, margins, and line spacing apply immediately
- **Xochitl integration**  -  EPUBs synced via the reMarkable PC/Mac app appear in the library automatically
- **Text selection and highlights**  -  select text, highlight with colors, add notes
- **Pen annotations**  -  draw margin notes with the stylus
- **Offline dictionary**  -  tap a word to look it up
- **In-book search**  -  full-text search across the entire book
- **Highlights export**  -  export all highlights and annotations as Markdown
- **Bookmarks and reading progress**  -  automatic position saving, bookmarks, reading time estimates
- **8 curated reading fonts**  -  Literata, Source Serif 4, Bitter, Libre Caslon, Merriweather, Cormorant Garamond, Crimson Pro, Vollkorn
- **Native e-paper design**  -  flat, high-contrast UI designed for 1-bit e-ink with zero animation

## Screenshots

<!-- Add screenshots of the app running on the reMarkable here -->
<!-- Suggested screenshots: library view, reading view, font picker, highlights, table of contents -->

*Screenshots coming soon.*

## Supported device

| Device | Status |
|--------|--------|
| reMarkable Paper Pro Move (chiappa) | Supported  -  firmware 5.6.x |
| reMarkable Paper Pro (ferrari) | Not supported |
| reMarkable 2 | Not supported |
| reMarkable 1 | Not supported |

ReadMarkable is built with the **Chiappa SDK**, which targets the Paper Pro Move specifically. The Paper Pro and reMarkable 2 use different SDKs, so compatibility with those devices cannot be guaranteed.

## Install

### Desktop installer (recommended)

The easiest way to install ReadMarkable. No command line, no jailbreak, no developer mode needed. Just plug in your reMarkable over USB and run the installer.

1. Download `ReadMarkableInstaller-v1.0.0-win64.zip` from the [latest release](https://github.com/viraya/ReadMarkable/releases/latest).
2. Extract the zip and run `readmarkable-installer.exe`.
3. Windows will show a "Windows protected your PC" SmartScreen warning because the app is not code-signed. Click **"More info"**, then click **"Run anyway"**. If you don't see that option, right-click `readmarkable-installer.exe`, go to **Properties > General**, and check the **Unblock** checkbox under Security, then click OK and try again. This is normal for unsigned open source software.
4. Connect your reMarkable to your computer via USB.
5. Make sure **USB web interface** is enabled on your reMarkable (Settings > General > Storage > USB web interface).
6. Follow the installer steps.
7. Done. A **ReadMarkable** tile appears on your home screen.

Currently available for **Windows** only. macOS and Linux users can use the SSH install method below.

The installer handles everything automatically:
- Sets up SSH connectivity to your device
- Installs [XOVI](https://github.com/asivery/rm-xovi-extensions) (extension loader) and [AppLoad](https://github.com/asivery/rm-appload) (app sidebar)
- Deploys the ReadMarkable binary, fonts, and launcher icon
- Configures boot persistence so everything survives reboots

No need to install XOVI, AppLoad, or any other tools separately. The installer does it all.

Your device continues to work normally  -  reMarkable Cloud sync, OTA firmware updates, notebooks and handwriting are all untouched.

See [`installer/README.md`](installer/README.md) for more details, Windows SmartScreen instructions, and macOS Gatekeeper notes.

### Manual install via SSH

For Linux users or anyone who prefers the command line.

#### Prerequisites

- SSH access to your reMarkable (developer mode enabled, root password or SSH key configured)
- `scp` and `ssh` on your host
- A Paper Pro Move (chiappa) on firmware 5.6.x

#### Steps

```bash
VER=v1.0.0
curl -LO https://github.com/viraya/ReadMarkable/releases/download/${VER}/readmarkable-${VER}-chiappa.tar.gz
curl -LO https://github.com/viraya/ReadMarkable/releases/download/${VER}/readmarkable-${VER}-chiappa.tar.gz.sha256
sha256sum -c readmarkable-${VER}-chiappa.tar.gz.sha256

scp readmarkable-${VER}-chiappa.tar.gz root@10.11.99.1:/home/root/
ssh root@10.11.99.1 "cd /home/root && tar -xzf readmarkable-${VER}-chiappa.tar.gz && cd readmarkable-${VER}-chiappa && ./install.sh"
```

The default USB IP is `10.11.99.1`. The installer is non-interactive  -  it logs what it does and exits.

#### Installer flags

- `--force`  -  skip the chiappa device check (experimental, not supported)
- `--no-launcher`  -  skip AppLoad + Draft file placement
- `--verbose`  -  log every file operation

## Adding books

Two ways to get EPUBs into ReadMarkable:

1. **Via the reMarkable PC/Mac app**  -  import EPUBs through the official app. They sync to the device and appear in ReadMarkable's library automatically.
2. **Via SCP**  -  copy `.epub` files directly to `/home/root/.local/share/readmarkable/books/` on the device.

## Quit ReadMarkable

Two ways to return to the stock reMarkable interface:

- **Library toolbar**  -  tap the quit icon (top-right of the library screen).
- **Global Settings**  -  scroll to the bottom and tap "Quit ReadMarkable".

Both show a "Returning to reMarkable..." splash for 2–4 seconds while xochitl restarts. This is normal.

If the app hangs, hold the power button for ~10 seconds to force shutdown. xochitl returns on reboot.

## Uninstall

### Via the desktop installer

Re-run the installer. It detects the existing installation and offers an uninstall option.

### Via SSH

```bash
ssh root@10.11.99.1
cd /home/root/.local/share/readmarkable
./uninstall.sh
```

Flags:
- `--purge`  -  also remove reading positions, highlights, and preferences
- `--verbose`  -  log every file removal

## Updating

Download a newer release and run the installer again (desktop or SSH). Reading positions and highlights are preserved automatically.

### After a firmware OTA

- The AppLoad tile survives automatically (files live under `/home`).
- The Draft tile is auto-restored on the next ReadMarkable launch.
- Firmware OTAs reset the root password and SSH host key  -  you'll need to re-enable developer mode and set a new password.

## Troubleshooting

**"Unsupported device" error during install**
ReadMarkable v1.0 targets Paper Pro Move (chiappa) only. Check `cat /proc/device-tree/model` on your device.

**Nothing appears on the home screen after install**
Open the AppLoad sidebar in xochitl and tap Reload.

**Xochitl is still running after launch**
Check `systemctl status xochitl` over SSH. Reboot the device if it's in an inconsistent state.

**"Returning to reMarkable..." splash won't go away**
Wait ~15 seconds. If nothing changes, hold the power button to force shutdown.

**App disappeared after a firmware update**
Launch the app once via AppLoad or SSH  -  it recreates the Draft entry automatically.

## Building from source

ReadMarkable is cross-compiled using a Docker toolchain targeting the Chiappa SDK.

```bash
docker build -t readmarkable-toolchain .
./deploy.sh build
```

See the `Dockerfile` and `deploy.sh` for the full build setup. The installer has its own build system  -  see [`installer/README.md`](installer/README.md).

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

## Developer

Created by Niels Peeren.
