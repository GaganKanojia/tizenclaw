# rvc-local-cli
**Description**: Control Samsung robot vacuum directly using on-device rvc_command. No internet or SmartThings required. Works fully offline.
**Category**: Home Automation

## Subcommands
| Subcommand | Description |
|---|---|
| `mapping` | Start room mapping only (vc_command 1 via TIDL) |
| `clean-map` | Map and clean simultaneously (vc_command 2 via TIDL) |
| `cmd3` | Command 3 — update description when function is discovered |
| `cmd4` | Command 4 — update description when function is discovered |
| `cmd5` | Command 5 — update description when function is discovered |
| `cmd6` | Command 6 — update description when function is discovered |
| `cmd7` | Command 7 — update description when function is discovered |
| `cmd8` | Command 8 — update description when function is discovered |
| `cmd9` | Command 9 — update description when function is discovered |
| `cmd10` | Command 10 — update description when function is discovered |
| `cmd11` | Command 11 — update description when function is discovered |
| `cmd12` | Command 12 — update description when function is discovered |
| `cmd13` | Command 13 — update description when function is discovered |
| `cmd14` | Command 14 — update description when function is discovered |
| `cmd15` | Command 15 — update description when function is discovered |
| `run --number <N>` | Run rvc_command N directly by number (1–15) |
| `list` | Show all command numbers, names, and descriptions as JSON |

## LLM Agent Instructions
**CRITICAL**: Pass exactly ONE subcommand as the first positional argument.
This tool runs rvc_command directly on the device — no credentials or config needed.
Use `list` first to see all available commands before choosing one.
Use `run --number N` when you know the command number but not its name yet.
Example: `mapping`
Example: `clean-map`
Example: `run --number 5`
Example: `list`
