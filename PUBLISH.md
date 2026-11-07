# Application Store Publish Record

Keep this file in the application project root. Update it when package
metadata, listing copy, assets, privacy choices, or submission status changes.

Portal: <https://dev.cardputer.cc/#/upload>

## Package

- Debian package path: `dist/zero-music_0.2.1_rev1_arm64.deb`
- Package: `zero-music`
- Version: `0.2.1-rev1`
- Architecture: `arm64`
- Maintainer: `H.B. Yu <mail@yuhaibo.com>`
- APPLaunch desktop path: `usr/share/APPLaunch/applications/zero-music.desktop`
- Executable path: `/usr/bin/zero-music`

## Source and Ownership

- Public source repository: TODO
- Expected GitHub uploader/owner: TODO
- Hide uploader email: `true`

## Listing

- Application name: `Zero Music`
- One-line summary (maximum 80 characters): `Local music and streaming services, plugin-powered.`
- Description: `Zero Music plays local files and, with third-party MyGo Music-compatible plugins, streaming sources on Cardputer Zero. It ships no music catalogue or service credentials. The keyboard-first LVGL interface is built for the 320x170 screen.`
- Categories (maximum 6): `Media`

## Assets

- 256 x 256 source icon: `assets/images/zero-music.png` (512 x 512 source, downscaled by the portal)
- Icon packaged in `.deb`: `usr/share/APPLaunch/share/images/zero-music.png`
- Screenshot 1 (320 x 170): `screenshot/zero-music-home.png`
- Screenshot 2 (320 x 170): `screenshot/zero-music-now-playing.png`
- Screenshot 3 (320 x 170): `screenshot/zero-music-plugins.png`
- Screenshot 4 (320 x 170): `screenshot/zero-music-install.png`
- Optional screenshot 5 (portal supports up to 6): not used
- Optional screenshot 6 (portal supports up to 6): not used

## Preflight

- [x] `.deb` exists and can be parsed.
- [x] Control fields contain Package, Version, Architecture, and Maintainer.
- [x] Architecture is appropriate for the package (`arm64` for native binaries).
- [x] APPLaunch `.desktop`, executable, and PNG icon exist inside the package.
- [ ] Package name ownership and update version are valid.
- [ ] Listing copy and categories have been reviewed.
- [x] Four clean 320 x 170 baseline screenshots exist.
- [x] No credentials, tokens, or private server details are embedded.
- [ ] The user explicitly approved external submission.

## Submission Result

- Status: `not submitted`
- Submitted at: TODO
- Portal message: TODO
- Actions URL: TODO
- Tracking or pull-request URL: TODO
