# ATOMiK Schema Tools for VS Code

Schema validation, intellisense, and SDK generation for ATOMiK delta-state computing modules.

## Features

### JSON Schema Intellisense

Files matching `*.atomik.json` or located in `**/schemas/**/*.json` automatically receive:

- Field autocompletion
- Enum value suggestions
- Real-time validation error squiggles
- Hover documentation for all fields

### Snippets

| Prefix | Description |
|--------|-------------|
| `atomik-schema` | Complete ATOMiK schema skeleton |
| `atomik-field` | Delta field block |
| `atomik-hardware` | Hardware mapping section |

### Command Palette

Open the command palette (`Ctrl+Shift+P` / `Cmd+Shift+P`) and type "ATOMiK":

- **ATOMiK: Generate SDK from Current Schema** -- generate multi-language SDK code
- **ATOMiK: Validate Current Schema** -- validate without generating
- **ATOMiK: Batch Generate from Directory** -- process all schemas in a folder
- **ATOMiK: Show Schema Info** -- display namespace, fields, and operations

## Prerequisites

The command palette commands require the `atomik-gen` CLI tool:

```bash
cd software
pip install -e .
```

Verify installation:

```bash
python -m atomik_sdk.cli --version
python -m atomik_sdk.cli list
```

> **Note:** The extension automatically discovers `atomik-gen` via PATH, `python -m`, or `python3 -m` — no manual PATH configuration needed.

## Development

```bash
cd vscode-extension/atomik-vscode
npm install
npm run compile
```

### Install locally

```bash
code --install-extension .
```

### Package as VSIX

```bash
npx vsce package
```
