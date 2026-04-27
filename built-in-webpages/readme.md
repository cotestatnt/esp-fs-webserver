The canonical built-in page sources are no longer maintained in this folder.

Use the shared repository at `C:\Cloud\fs-webserver-shared-pages` as the single source of truth for both **AsyncFsWebServer** and **FSWebServer** built-in pages.

Current workflow:
* edit `/setup` sources in `C:\Cloud\fs-webserver-shared-pages\setup`
* edit `/edit` sources in `C:\Cloud\fs-webserver-shared-pages\edit`
* open a terminal in `C:\Cloud\fs-webserver-shared-pages`
* run `npm install` once
* run `npm run build` to regenerate `setup_htm.h`, `logo_svg.h`, and `edit_htm.h` in both library `src/assets` folders

Target resolution order in the shared repo:
* `--target <path>` CLI arguments
* `targets.json`
* autodiscovery of the Arduino sketchbook `libraries` folder

This folder is kept only as historical reference and should not be used as the primary build entrypoint anymore.