# rvc-local-cli — Setup & Usage Guide

**What this tool does**: Controls your Samsung Jet Bot robot vacuum directly,
with no internet connection required. It uses commands built into the vacuum's
own operating system — think of it as a direct remote control that lives on
the device itself.

**When to use this tool**: Use this when your vacuum is offline, you do not
have a SmartThings account, or you simply prefer a simpler setup with no cloud
service involved.

> **Want richer control?**
> This tool cannot check battery level, detailed status, suction settings, or
> cleaning mode — those require a SmartThings connection. If you are online and
> have a SmartThings account, see
> [robotic-vacuum-cli/INSTRUCTIONS.md](../robotic-vacuum-cli/INSTRUCTIONS.md)
> for the full-featured alternative.

---

## Before You Start — What You Will Need

- [ ] TizenClaw must be **installed directly on the Samsung Jet Bot itself**,
      not on a separate computer. The `rvc_command` program that this tool
      relies on only exists on the vacuum's own operating system.
- [ ] A USB debugging connection between the vacuum and your development
      computer (used for the one-time install)
- [ ] About 15 minutes for setup

> **Important**: This tool will NOT work if TizenClaw is running on your
> laptop or desktop PC. The commands only exist on the vacuum. If you are
> using TizenClaw on a Linux PC, use `robotic-vacuum-cli` instead —
> see [robotic-vacuum-cli/INSTRUCTIONS.md](../robotic-vacuum-cli/INSTRUCTIONS.md).

---

## How This Works (Plain English)

Your Samsung Jet Bot runs its own operating system (Tizen OS) which has a
built-in program called `rvc_command`. Running `rvc_command 1` starts room
mapping. Running `rvc_command 2` maps and cleans simultaneously. There are
15 such commands in total.

`rvc-local-cli` is a small helper program that:
1. Takes a friendly command name (like `mapping` or `clean-map`)
2. Translates it to the right `rvc_command` number
3. Runs it on the device
4. Reports back whether it succeeded

No internet, no account, no passwords required.

---

## One-Time Setup

### Step 1 — Connect the Vacuum to Your Computer

Connect your Samsung Jet Bot to your development computer using a USB debugging
cable. Then open a terminal (press **Ctrl + Alt + T** on Linux) and run:

```bash
sdb devices
```

This lists all connected Tizen devices. You should see your vacuum in the list
with an ID like `device-12345`. Note down that ID — you will need it in the
next step.

### Step 2 — Deploy TizenClaw to the Vacuum

From your project folder, run:

```bash
cd ~/strawhats/tizen_claw/tizenclaw
./deploy.sh -a x86_64 -d YOUR_DEVICE_ID
```

Replace `YOUR_DEVICE_ID` with the ID from Step 1.

The build and install takes a few minutes. When it finishes, `rvc-local-cli`
is installed on the vacuum at:

```
/opt/usr/share/tizenclaw/tools/cli/rvc-local-cli/rvc-local-cli
```

There is no configuration file to fill in. Setup is complete.

---

## Day-to-Day Use

Once TizenClaw is running on the vacuum, control it by typing natural language
commands through `tizenclaw-cli`:

```bash
tizenclaw-cli ask "Map the room"
tizenclaw-cli ask "Start mapping and cleaning"
tizenclaw-cli ask "What local vacuum commands are available?"
tizenclaw-cli ask "Run vacuum command number 5"
```

TizenClaw's AI understands plain English — you do not need to use exact phrases.

### Running Commands Directly

If you prefer to skip the AI and run commands directly:

| What you want to do | Command |
|---|---|
| Start mapping the room | `rvc-local-cli mapping` |
| Map the room and clean at the same time | `rvc-local-cli clean-map` |
| Run an unknown command by number | `rvc-local-cli run --number 5` |
| See a list of all available commands | `rvc-local-cli list` |

Commands 3 through 15 are currently labelled as "unknown" because their
function has not been officially documented. You can safely explore them —
see the section below.

### Reading the Command Output

Every command prints a short result in a standard format. You do not need to
understand the format in detail — just know that:

- `"status":"ok"` → the command was sent successfully
- `"status":"error"` → something went wrong

Example of a successful result:
```json
{"status":"ok","command":1,"action":"mapping"}
```

Example of an error:
```json
{"status":"error","command":5,"exit_code":1}
```

---

## Connecting TizenClaw on the Vacuum to Ollama on a PC

TizenClaw needs an AI model to understand your plain English commands. By
default it looks for Ollama on the same device (`localhost`). Since
`rvc-local-cli` runs on the vacuum itself — a lightweight device — you will
almost certainly want to run the Ollama AI on a more powerful PC on the same
Wi-Fi network and have the vacuum connect to it remotely.

This section walks through the full setup.

### What You Will Need

- A PC on the same Wi-Fi as the vacuum with at least **8 GB RAM** (16 GB or
  more recommended; 32 GB for large models)
- Ollama installed on that PC (free, open-source)
- The vacuum and the PC on the **same Wi-Fi network**

---

### Step 1 — Install Ollama on the PC (if not already installed)

On the PC that will run the AI, open a terminal and run:

```bash
curl -fsSL https://ollama.com/install.sh | sh
```

This installs Ollama as a background service. It starts automatically after
installation.

---

### Step 2 — Download an AI Model

Still on the PC, download a model. Choose based on how much RAM your PC has:

| PC RAM | Recommended model | Command |
|---|---|---|
| 8 GB | `llama3` (smaller, faster) | `ollama pull llama3` |
| 16 GB | `mistral` | `ollama pull mistral` |
| 32 GB or more | `qwen2.5:32b` (best quality) | `ollama pull qwen2.5:32b` |

The download can be several gigabytes — let it complete before continuing.

---

### Step 3 — Allow the PC to Accept Connections from Other Devices

By default, Ollama only listens for connections coming from itself. You need
to open it up so the vacuum can reach it.

**Make the change permanent** (survives reboots):

```bash
sudo systemctl edit ollama
```

A text editor will open. Type these lines exactly, then save and exit
(**Ctrl + O**, **Enter**, **Ctrl + X** if it's nano):

```
[Service]
Environment="OLLAMA_HOST=0.0.0.0"
```

Then restart Ollama:

```bash
sudo systemctl restart ollama
```

**Optional — open the firewall port** (only needed if your PC has a firewall
active):

```bash
sudo ufw allow 11434/tcp
```

---

### Step 4 — Find the PC's IP Address

On the **PC running Ollama**, run:

```bash
ip route get 1.1.1.1 | awk '{print $7; exit}'
```

This prints a single line like `192.168.1.42`. Write this down — you will
need it in the next step.

---

### Step 5 — Verify the Vacuum Can Reach Ollama

From a terminal connected to the vacuum via `sdb shell`, run (replace
`192.168.1.42` with your PC's IP from Step 4):

```bash
curl http://192.168.1.42:11434/api/tags
```

If you see a JSON response listing your downloaded models, the connection is
working. If you get "connection refused", recheck Steps 3 and 4.

---

### Step 6 — Update the LLM Config on the Vacuum

The config file that tells TizenClaw where to find Ollama lives on the
vacuum at:

```
/opt/usr/share/tizenclaw/config/llm_config.json
```

Open it via `sdb shell`:

```bash
sdb shell
nano /opt/usr/share/tizenclaw/config/llm_config.json
```

Find the `"ollama"` section and the `"active_backend"` line. Change them to
match your setup. Replace `192.168.1.42` with your PC's IP, and
`qwen2.5:32b` with whichever model you downloaded in Step 2:

```json
{
  "active_backend": "ollama",
  "backends": {
    "ollama": {
      "model": "qwen2.5:32b",
      "endpoint": "http://192.168.1.42:11434"
    }
  }
}
```

> Leave all other fields in the file unchanged — only edit `active_backend`,
> `ollama.model`, and `ollama.endpoint`.

Save the file: **Ctrl + O**, **Enter**, **Ctrl + X**.

---

### Step 7 — Restart TizenClaw on the Vacuum

For the config change to take effect, restart the daemon:

```bash
sdb shell systemctl restart tizenclaw
```

---

### Step 8 — Test the Connection

Send a test command through TizenClaw:

```bash
tizenclaw-cli ask "What local vacuum commands are available?"
```

If TizenClaw responds correctly, it is now using your PC's Ollama model to
understand commands and control the vacuum.

---

### Alternative: Update the Config via the Web Dashboard

If you have already set up the web dashboard (see the section below), you can
update the LLM config remotely without using `sdb shell` at all.

**Step 1** — Log in and get a token (replace IP and port as before):

```bash
curl -X POST http://192.168.1.55:9090/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"password": "admin"}'
```

**Step 2** — Write the updated config (replace the model and endpoint with
your values, and `TOKEN` with the token from Step 1):

```bash
curl -X POST http://192.168.1.55:9090/api/config/llm_config.json \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer TOKEN" \
  -d '{
    "content": "{\"active_backend\":\"ollama\",\"fallback_backends\":[],\"backends\":{\"ollama\":{\"model\":\"qwen2.5:32b\",\"endpoint\":\"http://192.168.1.42:11434\"}}}"
  }'
```

**Step 3** — Restart TizenClaw:

```bash
sdb shell systemctl restart tizenclaw
```

---

### Ollama Connection Troubleshooting

**"connection refused" when curling the Ollama PC from the vacuum**

- Check that Ollama is running on the PC: `systemctl status ollama`
- Confirm `OLLAMA_HOST=0.0.0.0` is set: `systemctl show ollama | grep OLLAMA_HOST`
- Confirm the firewall allows port 11434: `sudo ufw status`
- Make sure both devices are on the same Wi-Fi network (not guest vs main)

**TizenClaw responds but gives wrong or poor answers**

The model may be too small for the task. Try a larger model (see the table in
Step 2) and update `ollama.model` in `llm_config.json`.

**"model not found" error from Ollama**

The model name in `llm_config.json` does not match what Ollama has downloaded.
On the PC, run `ollama list` to see the exact model names available, then
update `llm_config.json` to match.

---

## Discovering What the Unknown Commands Do

Commands 3–15 are available but not yet labelled. Here is a safe way to
explore them and find out what each one does.

**Before you start**: Make sure the vacuum is in a clear, open area with
nothing it could bump into.

1. Run `rvc-local-cli list` to see the full list of commands.
2. Try a command: `rvc-local-cli run --number 3`
3. Watch what the vacuum does. Note it down.
4. Repeat for other numbers you want to explore.

**Once you know what a command does**, update the tool so the friendly name
reflects it:

1. Open [main.cc](main.cc) in a text editor.
2. Find the line for that command number, for example:
   ```
   {3,  "cmd3",  "Unknown — update when function is discovered"},
   ```
3. Change the name and description to match what you discovered, for example:
   ```
   {3,  "dock",  "Return to charging dock"},
   ```
4. Also update the matching row in [tool.md](tool.md).
5. Rebuild and redeploy: `./deploy.sh -a x86_64 -d YOUR_DEVICE_ID`

After rebuilding, you can call it by name: `rvc-local-cli dock`

---

## Comparison: rvc-local-cli vs robotic-vacuum-cli

| Feature | rvc-local-cli (this tool) | robotic-vacuum-cli |
|---|---|---|
| Requires internet | No | Yes |
| Requires SmartThings account | No | Yes |
| Works offline | Yes | No |
| Check battery level | No | Yes |
| Check cleaning status | No | Yes |
| Adjust suction/turbo | No | Yes |
| Choose cleaning mode | No (limited) | Yes (auto, part, map…) |
| Setup complexity | Very simple | Moderate (one-time token setup) |

Use `rvc-local-cli` for quick offline control. Use `robotic-vacuum-cli` when
you need status information or fine-grained control over cleaning behaviour.

---

## Troubleshooting

**"command not found: rvc_command"**

This means the tool is not running on the vacuum — it is running on a
different machine that does not have `rvc_command`. Either:
- Ensure TizenClaw is deployed to the vacuum (not your laptop), or
- Switch to `robotic-vacuum-cli` for internet-based control:
  [robotic-vacuum-cli/INSTRUCTIONS.md](../robotic-vacuum-cli/INSTRUCTIONS.md)

**`rvc-local-cli` binary is not found on the device**

The build may not have completed successfully. Re-run the deploy command:

```bash
./deploy.sh -a x86_64 -d YOUR_DEVICE_ID
```

Check the terminal output for any red error messages during the build.

**A command returns an error exit code**

The vacuum may be in a state where that command is not allowed — for example,
it is currently charging, in an error state, or already executing another
command. Wait for the current task to finish and try again.

**The vacuum does nothing when the command reports success**

Some `rvc_command` numbers may not trigger visible movement depending on the
vacuum's current state. Try running `rvc-local-cli clean-map` or
`rvc-local-cli mapping` first (the two confirmed commands), then explore
others.

**`sdb devices` shows nothing**

- Check that the USB cable is securely connected
- Make sure USB debugging is enabled on the vacuum (usually enabled by default
  on developer builds)
- Try a different USB cable or port

---

## Remote Access via Web Dashboard

TizenClaw has a built-in HTTP server that lets you send commands from any
device on your network — another computer, a phone, a tablet — without needing
the `tizenclaw-cli` program installed there. You can use it from a browser or
from the terminal with `curl`.

The web dashboard is already enabled by default. No extra configuration is
needed.

> **Note**: Because `rvc-local-cli` runs commands directly on the vacuum's
> operating system, TizenClaw must still be installed on the vacuum itself.
> The web dashboard simply gives you a remote way to send instructions to it.

### Step 1 — Check the Port

The dashboard listens on a different port depending on where TizenClaw is running:

| Where TizenClaw is installed | Dashboard port |
|---|---|
| Tizen device or emulator (`deploy.sh`) | **9090** |
| Host Linux PC (`deploy_host.sh`) | **9091** |

Since `rvc-local-cli` must run on the vacuum (Tizen device), the port will
almost always be **9090**.

### Step 2 — Find the IP Address of the Vacuum

You need the IP address of the vacuum (the device running TizenClaw) so you
can connect to it from another device on the same Wi-Fi network.

Connect via USB and run:

```bash
sdb shell ip addr show
```

Look through the output for a line starting with `inet` that is NOT
`127.0.0.1`. The number before the `/` is the IP address (e.g. `10.0.2.15`
or `192.168.1.55`).

### Step 3 — Test That the Dashboard Is Reachable

From any machine on the same network, open a terminal and run (replace
`192.168.1.55` with your vacuum's IP address):

```bash
curl http://192.168.1.55:9090/api/status
```

If you see a JSON response, the dashboard is reachable. If you see "connection
refused", check that TizenClaw is running on the vacuum and that both devices
are on the same Wi-Fi network.

### Step 4 — Send Vacuum Commands Remotely

The chat endpoint accepts plain English prompts and does not require a
password. Replace `192.168.1.55:9090` with your vacuum's address:

**Start room mapping:**
```bash
curl -X POST http://192.168.1.55:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Start mapping the room"}'
```

**Map and clean at the same time:**
```bash
curl -X POST http://192.168.1.55:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Map the room and start cleaning"}'
```

**List all available local commands:**
```bash
curl -X POST http://192.168.1.55:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "What local vacuum commands are available?"}'
```

**Run a specific command by number:**
```bash
curl -X POST http://192.168.1.55:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Run vacuum command number 5"}'
```

Each response looks like:

```json
{
  "status": "ok",
  "session_id": "web-1",
  "response": "The mapping command has been started."
}
```

The `session_id` field is optional — if you include the same value in
follow-up requests, the AI remembers the context of your conversation:

```bash
curl -X POST http://192.168.1.55:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Now run command 3 as well", "session_id": "web-1"}'
```

### Step 5 — (Optional) Log In for Secured Endpoints

The chat endpoint is open, but some endpoints (like system status and config
changes) require you to log in first. The default password is `admin`.

**Log in and get a token:**

```bash
curl -X POST http://192.168.1.55:9090/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"password": "admin"}'
```

Response:

```json
{"status": "ok", "token": "abc123def456..."}
```

**Use the token for secured endpoints:**

```bash
curl http://192.168.1.55:9090/api/status \
  -H "Authorization: Bearer abc123def456..."
```

### Step 6 — Change the Default Password (Recommended)

The default password `admin` is well-known. Change it after your first login,
especially if the vacuum is on a shared network.

```bash
curl -X POST http://192.168.1.55:9090/api/auth/change_password \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer abc123def456..." \
  -d '{"old_password": "admin", "new_password": "your-new-password"}'
```

### Web Dashboard Troubleshooting

**"Connection refused" when running curl**

- Make sure TizenClaw is running on the vacuum (not just deployed — it must be actively running)
- Confirm you are using port 9090 (Tizen device)
- Check both your computer and the vacuum are on the same Wi-Fi network

**"Invalid password" when logging in**

The password has been changed from the default. If you have forgotten it,
delete `/opt/usr/share/tizenclaw/admin_password.json` on the vacuum and
restart TizenClaw — it will reset to `admin`.

**The chat response says it cannot find or use the tool**

`rvc-local-cli` only works when TizenClaw is running on the vacuum itself —
the `rvc_command` binary does not exist on other machines. If TizenClaw is
running on a separate PC, use `robotic-vacuum-cli` instead:
[robotic-vacuum-cli/INSTRUCTIONS.md](../robotic-vacuum-cli/INSTRUCTIONS.md)
