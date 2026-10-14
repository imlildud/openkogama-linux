<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/openkogama/openkogama-site/main/.github/logo/launcher-dark.png">
  <img src="https://raw.githubusercontent.com/openkogama/openkogama-site/main/.github/logo/launcher-light.png" alt="OpenKogama Launcher" width="400">
</picture>

The Worlds and Avatars tabs manage your local game data. Add community servers in the Servers tab and click Join to launch the selected KoGaMa version directly. The server chooses its entry world inside the game.

The launcher reads only `/api/server` (`title` and the optional fixed `/api/server/thumbnail` path) for server tiles. Titles are limited to 40 characters, thumbnails to 256 KB and 4096 pixels per side, then cropped to 400 × 160. The last valid title and thumbnail remain cached when the server is offline. Older servers can answer `/ping` without providing tile metadata.

Build with `-DBUILD_TESTING=ON` and run `ctest --output-on-failure` to check server metadata, direct join requests, size limits and offline caching.

## Credits

Made by nightus.

- **Becko**: huge thanks, the main inspiration for this project, and the packet captures
- **LazyLemon**: info on how things looked in old versions, and help with the launcher
