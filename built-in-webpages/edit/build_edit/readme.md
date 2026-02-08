# Edit Page Build Script

This script processes the edit page HTML file by compressing it and converting it to a C header file for embedding in the firmware.

## Prerequisites

Install Node.js dependencies:

```bash
npm install
```

## Usage

Run the build script:

```bash
npm run build
```

## What it does

1. Reads `edit.htm` from the parent directory
2. (Optional) Minifies HTML, CSS, and JavaScript
3. Compresses the result with gzip (maximum compression)
4. Converts to a C byte array
5. Saves as `edit_htm.h` in `src/assets/` directory

## Output

- **Intermediate files**: `build_edit/min/edit.htm`, `build_edit/min/edit.htm.gz`
- **Final output**: `src/assets/edit_htm.h` (PROGMEM array: `_acedit_htm`)

## Notes

- HTML minification is disabled by default to avoid breaking functionality
- The edit page already has inline CSS and JavaScript, so no separate processing is needed
- External ACE editor is loaded from CDN and not bundled
