
# ReadMarkable

A native EPUB reader for the **reMarkable Paper Pro Move** (chiappa). Built with the Chiappa SDK.

Reading EPUBs on a reMarkable shouldn't mean waiting 20 seconds for a font size change. The stock reMarkable converts EPUBs to PDFs internally, making every layout change painfully slow. ReadMarkable renders EPUBs natively on the e-paper display, so font changes, margin adjustments, and page turns are instant.

## Features

- **Native EPUB rendering** via CREngine: reflowed text, table of contents, footnotes, hyperlinks, images
- **Instant font and layout changes**: font size, font family, margins, and line spacing apply immediately
- **Xochitl integration**: EPUBs synced via the reMarkable PC/Mac app appear in the library automatically
- **Text selection and highlights**: select text, highlight with colors, add notes
- **(Future?) Pen annotations**: draw margin notes with the stylus (tried this but no success. Feature not available right now.)
- **Offline dictionary**: tap a word to look it up
- **In-book search**: full-text search across the entire book
- **Highlights export**: export all highlights and annotations as Markdown
- **Bookmarks and reading progress**: automatic position saving, bookmarks, reading time estimates
- **8 curated reading fonts**: Literata, Source Serif 4, Bitter, Libre Caslon, Merriweather, Cormorant Garamond, Crimson Pro, Vollkorn
- **Native e-paper design**: flat, high-contrast UI designed for 1-bit e-ink with zero animation
- **Tap-to-launch after reboot**: no PC needed to bring ReadMarkable back after the tablet reboots

## Screenshots

Installer process

<img width="289" height="221" alt="installer1" src="https://github.com/user-attachments/assets/85abf395-9432-4dc9-9a6c-08f269c9cdcc" />
<img width="289" height="221" alt="installer2" src="https://github.com/user-attachments/assets/e37725a1-d274-4f77-aa20-393ebe3f7844" />
<img width="289" height="221" alt="installer3" src="https://github.com/user-attachments/assets/af82b4f8-5d8f-4832-bcd2-f1dacc5637cb" />
<img width="289" height="221" alt="installer4" src="https://github.com/user-attachments/assets/f1ab3268-694d-454c-834d-c921c9b95174" />
<img width="289" height="221" alt="installer5" src="https://github.com/user-attachments/assets/0e79b469-1212-4a46-bbc7-da64bb1231c9" />
<img width="289" height="221" alt="installer6" src="https://github.com/user-attachments/assets/88a035b6-12b5-4bf6-9205-22b03cc0c6f6" />

App on device

<img width="467" height="745" alt="72c7b5da-d397-4f17-afb0-a659589b4dfe" src="https://github.com/user-attachments/assets/31d9d8d6-6cd0-4620-a603-b634cd3fbb60" />
<img width="459" height="735" alt="ebcb63fc-487a-4ae9-a508-8fa7a44f2afb" src="https://github.com/user-attachments/assets/caadc61c-c8b5-42f0-812e-eef36eb03ace" />
<img width="443" height="728" alt="b4b22fab-da13-492f-a2f4-ca55cad8b9c4" src="https://github.com/user-attachments/assets/e84b8d2f-e3be-49d3-9c29-927a36a7aaac" />
<img width="438" height="720" alt="472054c0-34af-44cc-a421-4d3b03c8e48f" src="https://github.com/user-attachments/assets/687c5ad6-c12a-451a-9618-7ee4e3a5b584" />
<img width="398" height="654" alt="5566918a-42f8-43d9-85dc-2443872a2774" src="https://github.com/user-attachments/assets/e2fd12c9-93ef-4380-84e1-6dfc90592fac" />

## Supported device

| Device | Status |
|--------|--------|
| reMarkable Paper Pro Move (chiappa) | Supported on firmware 5.6.x |
| reMarkable Paper Pro (ferrari) | Not supported |
| reMarkable 2 | Not supported |
| reMarkable 1 | Not supported |

ReadMarkable is built with the **Chiappa SDK**, which targets the Paper Pro Move specifically. The Paper Pro and reMarkable 2 use different SDKs, so compatibility with those devices cannot be guaranteed.

## Install

### Desktop installer (recommended)

The easiest way to install ReadMarkable. No command line, no jailbreak, no developer mode needed. Just plug in your reMarkable over USB and run the installer.

1. Download `ReadMarkableInstaller-v1.0.0-win64.zip` from the [latest release](https://github.com/viraya/ReadMarkable/releases/latest), or grab it directly from [`dist/`](dist/) in this repo.
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
- Configures tap-to-launch so you can bring ReadMarkable back after a reboot without a PC

No need to install XOVI, AppLoad, or any other tools separately. The installer does it all.

Your device continues to work normally. reMarkable Cloud sync, OTA firmware updates, notebooks, and handwriting are all untouched.

See [`installer/README.md`](installer/README.md) for more details, Windows SmartScreen instructions, and macOS Gatekeeper notes.

### Manual install via SSH

For Linux users or anyone who prefers the command line.

#### Prerequisites

- SSH access to your reMarkable (developer mode enabled, root password or SSH key configured)
- `scp` and `ssh` on your host
- A Paper Pro Move (chiappa) on firmware 5.6.x

#### Steps

```bash
VER=v1.0.1
curl -LO https://github.com/viraya/ReadMarkable/releases/download/${VER}/readmarkable-${VER}-chiappa.tar.gz
curl -LO https://github.com/viraya/ReadMarkable/releases/download/${VER}/readmarkable-${VER}-chiappa.tar.gz.sha256
sha256sum -c readmarkable-${VER}-chiappa.tar.gz.sha256

scp readmarkable-${VER}-chiappa.tar.gz root@10.11.99.1:/home/root/
ssh root@10.11.99.1 "cd /home/root && tar -xzf readmarkable-${VER}-chiappa.tar.gz && cd readmarkable-${VER}-chiappa && ./install.sh"
```

The default USB IP is `10.11.99.1`. The installer is non-interactive. It logs what it does and exits.

#### Installer flags

- `--force`: skip the chiappa device check (experimental, not supported)
- `--no-launcher`: skip AppLoad + Draft file placement
- `--verbose`: log every file operation

## Adding books

Two ways to get EPUBs into ReadMarkable:

1. **Via the reMarkable PC/Mac app**: import EPUBs through the official app. They sync to the device and appear in ReadMarkable's library automatically.
2. **Via SCP**: copy `.epub` files directly to `/home/root/.local/share/readmarkable/books/` on the device.

## After a reboot

The tablet boots to stock software first. To bring back ReadMarkable, triple-tap the top of the screen within two seconds, or triple-press the power button. The ReadMarkable tile reappears a few seconds later.

The installer enables this by default. It does not auto-start XOVI on boot, because that pattern can bootloop the tablet when an extension fails to load. Stock software always boots first, so if anything ever goes wrong you can recover on the device without a PC.

Reboots on a reMarkable are rare in normal use, so most users only need to do this every few weeks.

## Quit ReadMarkable

Two ways to return to the stock reMarkable interface:

- **Library toolbar**: tap the quit icon (top-right of the library screen).
- **Global Settings**: scroll to the bottom and tap "Quit ReadMarkable".

Both show a "Returning to reMarkable..." splash for 2 to 4 seconds while xochitl restarts. This is normal.

If the app hangs, hold the power button for ~10 seconds to force shutdown. xochitl returns on reboot.

## Uninstall

### Via the desktop installer

Re-run the installer. When it detects ReadMarkable is already installed, the Action Summary page shows an "Uninstall instead" button. Click it, confirm, and the installer will remove the ReadMarkable app, the tap-to-launch listener, and AppLoad (if it was installed by this installer). XOVI itself is kept on disk by default. Check "Also remove XOVI" to remove it too. Reading data is preserved unless you purge.

### Via SSH

```bash
ssh root@10.11.99.1
cd /home/root/.local/share/readmarkable
./uninstall.sh
```

Flags:
- `--purge`: also remove reading positions, highlights, and preferences
- `--verbose`: log every file removal

## Updating

Download a newer release and run the installer again (desktop or SSH). Reading positions and highlights are preserved automatically.

### After a firmware OTA

- The AppLoad tile survives automatically (files live under `/home`).
- The Draft tile is auto-restored on the next ReadMarkable launch.
- Firmware OTAs reset the root password and SSH host key. You will need to re-enable developer mode and set a new password.

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
Launch the app once via AppLoad or SSH. It recreates the Draft entry automatically.

**Triple-tap does not bring the tile back after reboot**
Tap within the top 15% of the screen (the strip under the reMarkable bezel logo). All three taps must land within two seconds of each other. As a fallback, triple-press the power button instead.

## Building from source

ReadMarkable is cross-compiled using a Docker toolchain targeting the Chiappa SDK.

```bash
docker build -t readmarkable-toolchain .
./deploy.sh build
```

See the `Dockerfile` and `deploy.sh` for the full build setup. The installer has its own build system. See [`installer/README.md`](installer/README.md).

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

## Developer

Created by Niels Peeren.
