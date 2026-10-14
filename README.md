<picture>
  <source media="(prefers-color-scheme: dark)" srcset=".github/logo-dark.png">
  <img src=".github/logo-light.png" alt="OpenKogama Launcher" width="400">
</picture>

The Worlds and Avatars tabs manage your local game data.

## Install

Download `OpenKogamaLauncher-win-Setup.exe` from the latest release. The launcher updates itself from GitHub releases.

Program files live in `%LocalAppData%\OpenKogamaLauncher` and are replaced on every update. Your data stays in `%LocalAppData%\OpenKogama\OpenKogama Launcher`: installed game versions in `instances`, and worlds, avatars, logs and downloaded assets in `server`. It is kept when the launcher updates or is uninstalled.

To publish a release, run the release workflow with a tag such as `v1.0.0`. A tag with a dash, like `v1.0.0-rc.2`, becomes a pre-release that only pre-release installs update to.

Build with `-DBUILD_TESTING=ON` and run `ctest --output-on-failure` to check server metadata, direct join requests, size limits and offline caching.

## Credits

Made by nightus.

- **Becko**: huge thanks, the main inspiration for this project, and the packet captures
- **LazyLemon**: info on how things looked in old versions, and help with the launcher
- **bustersky**: archiving the standalone builds
- **kamilslimak**: finding the 2012 build
