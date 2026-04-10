# robotic-vacuum-cli — Setup & Usage Guide

**What this tool does**: Controls your Samsung Jet Bot robot vacuum through the
internet, using Samsung's SmartThings service. You can start cleaning, stop,
pause, send it back to its dock, adjust suction power, and check its battery
and status — all from your computer or by talking to TizenClaw in plain English.

**When to use this tool**: Use this when your vacuum and your computer are both
connected to the internet. This tool gives you the richest control, including
battery level, cleaning modes, and suction settings.

> **No internet? Use rvc-local-cli instead.**
> If your vacuum is offline or you prefer a simpler setup with no account
> required, see [rvc-local-cli/INSTRUCTIONS.md](../rvc-local-cli/INSTRUCTIONS.md).

---

## Before You Start — What You Will Need

- [ ] Your Samsung Jet Bot is already set up and visible in the **SmartThings
      app** on your phone
- [ ] A stable internet connection on the computer running TizenClaw
- [ ] TizenClaw is installed (see `RVC_Plan.md` in the project root)
- [ ] A **Samsung account** — if you can log in to the SmartThings app, you
      already have one
- [ ] About 30 minutes for the one-time setup

---

## What is SmartThings? (Plain English)

Samsung controls the Jet Bot through a service called **SmartThings** — think
of it as Samsung's "remote control centre in the cloud". When you tell TizenClaw
to start cleaning, TizenClaw sends a message to SmartThings, which tells your
vacuum what to do.

To let TizenClaw talk to SmartThings on your behalf, you need to give it a
**secret key** (called an access token). This section walks you through getting
that key — it is a one-time process.

---

## One-Time Setup

### Step 1 — Create a SmartThings Developer App

This step creates an "identity" for TizenClaw so SmartThings knows to trust it.

1. Open a web browser and go to: **https://developer.smartthings.com**
2. Click **Sign In** in the top-right corner and log in with your Samsung
   account. If you do not have one, click **Create Account** — it is free.
3. Once signed in, look for **My Apps** in the top menu and click it. Then
   click **New App** (or **Register App**).
4. Fill in the form:
   - **App Name**: `TizenClaw` (or any name you will remember)
   - **Description**: `TizenClaw robot vacuum controller`
   - **Redirect URI**: type `https://localhost` exactly as shown here
   - **Scopes**: tick the boxes for `r:devices:*` and `x:devices:*`
     (These mean "read my devices" and "control my devices")
5. Click **Save**. You will now see a screen showing two codes:
   - **Client ID** — a long string like `abc123-def456-...`
   - **Client Secret** — another long string, shorter
6. **Copy both codes and paste them somewhere safe** (a text file, notepad,
   etc.) — you will need them shortly.

> Do not share these codes with anyone. They are like a password to your
> Samsung account.

---

### Step 2 — Get Your Security Tokens

This step creates the actual password (called an "access token") that TizenClaw
uses every time it talks to SmartThings.

Open a terminal window. On Linux, press **Ctrl + Alt + T**.

#### Step 2a — Open the authorization page in your browser

Build the following URL by replacing `YOUR_CLIENT_ID` with the Client ID you
copied in Step 1. Then paste the whole URL into your browser address bar and
press **Enter**.

```
https://api.smartthings.com/oauth/authorize?response_type=code&client_id=YOUR_CLIENT_ID&redirect_uri=https://localhost&scope=r:devices:*%20x:devices:*
```

#### Step 2b — Authorize TizenClaw

Your browser will show a Samsung authorization page asking if you want to allow
TizenClaw to access your devices. Click **Authorize** or **Allow**.

After you click, your browser will try to open `https://localhost` and show an
error page saying "This site can't be reached" — **this is completely normal**.

Look at the address bar. The URL will look something like:

```
https://localhost?code=AbCdEfGhIjKlMnOpQrSt
```

Copy the part that comes after `code=` — in the example above that is
`AbCdEfGhIjKlMnOpQrSt`. This is your **authorization code**.

> The authorization code expires in a few minutes. Complete Step 2c immediately.

#### Step 2c — Exchange the code for tokens

In your terminal, run the command below. Replace the three placeholders with
your own values:

- `YOUR_CLIENT_ID` → the Client ID from Step 1
- `YOUR_CLIENT_SECRET` → the Client Secret from Step 1
- `YOUR_CODE` → the authorization code you just copied

```bash
curl -X POST https://api.smartthings.com/oauth/token \
  -u "YOUR_CLIENT_ID:YOUR_CLIENT_SECRET" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "grant_type=authorization_code&code=YOUR_CODE&redirect_uri=https://localhost&client_id=YOUR_CLIENT_ID"
```

The terminal will print a response that looks like this:

```json
{
  "access_token": "aaaabbbb-cccc-dddd-eeee-ffffgggg",
  "refresh_token": "1111-2222-3333-4444-5555",
  "token_type": "Bearer",
  "expires_in": 86400
}
```

**Copy both the `access_token` value and the `refresh_token` value** into your
safe notes from Step 1.

> The access token expires every 24 hours, but TizenClaw renews it
> automatically using the refresh token. You only need to do this step once.

---

### Step 3 — Find Your Vacuum's Device ID

Every device in SmartThings has a unique identifier. Run this command in your
terminal. Replace `YOUR_ACCESS_TOKEN` with the `access_token` value from Step 2:

```bash
curl -H "Authorization: Bearer YOUR_ACCESS_TOKEN" \
  https://api.smartthings.com/v1/devices
```

The output will be a long list of your SmartThings devices. Scan through it
for a section mentioning "Jet Bot", "robot", or whatever name your vacuum has
in the SmartThings app. Inside that section, find the line that starts with
`"deviceId"` — it will look like:

```json
"deviceId": "12345678-abcd-1234-abcd-123456789abc"
```

Copy that value into your notes. This is your vacuum's **device ID**.

> **Tip**: If the output is hard to read, add `| python3 -m json.tool` to the
> end of the command to format it more clearly.

---

### Step 4 — Fill In the Config File

Now put everything together in the configuration file that TizenClaw reads.

Open the file in a text editor by running this command in your terminal:

**On host Linux** (TizenClaw installed via `deploy_host.sh`):
```bash
nano ~/.tizenclaw/data/config/robotic_vacuum_config.json
```

**On Tizen emulator or device** (TizenClaw deployed via `deploy.sh`):
```bash
sdb shell
nano /opt/usr/share/tizenclaw/data/config/robotic_vacuum_config.json
```

The file will look like this:

```json
{
  "client_id": "<SMARTTHINGS_CLIENT_ID>",
  "client_secret": "<SMARTTHINGS_CLIENT_SECRET>",
  "access_token": "<INITIAL_ACCESS_TOKEN>",
  "refresh_token": "<REFRESH_TOKEN>",
  "device_id": "<JET_BOT_DEVICE_ID>"
}
```

Replace each placeholder (including the `< >` angle brackets) with your values:

| Placeholder | Replace with |
|---|---|
| `<SMARTTHINGS_CLIENT_ID>` | Client ID from Step 1 |
| `<SMARTTHINGS_CLIENT_SECRET>` | Client Secret from Step 1 |
| `<INITIAL_ACCESS_TOKEN>` | The `access_token` value from Step 2 |
| `<REFRESH_TOKEN>` | The `refresh_token` value from Step 2 |
| `<JET_BOT_DEVICE_ID>` | The `deviceId` value from Step 3 |

When you are done, press **Ctrl + O** then **Enter** to save, then **Ctrl + X**
to exit.

The finished file should look something like this (your values will be different):

```json
{
  "client_id": "abc123-def456-ghi789",
  "client_secret": "mysecretkey12345",
  "access_token": "aaaabbbb-cccc-dddd-eeee-ffffgggg",
  "refresh_token": "1111-2222-3333-4444-5555",
  "device_id": "12345678-abcd-1234-abcd-123456789abc"
}
```

---

### Step 5 — Build and Deploy

Now rebuild TizenClaw so the tool is installed with your configuration.

**On host Linux:**
```bash
cd ~/strawhats/tizen_claw/tizenclaw
./deploy_host.sh
```

**On Tizen emulator:**
```bash
cd ~/strawhats/tizen_claw/tizenclaw
./deploy.sh -a x86_64 -d emulator-26101
```

Setup is complete. The tool is now ready to use.

---

## Day-to-Day Use

Once TizenClaw is running, control your vacuum by typing in plain English:

```bash
tizenclaw-cli ask "Start cleaning in auto mode"
tizenclaw-cli ask "Send the vacuum back to its dock"
tizenclaw-cli ask "What is the vacuum's battery level?"
tizenclaw-cli ask "Set suction to maximum"
tizenclaw-cli ask "Pause the vacuum"
tizenclaw-cli ask "Start cleaning quietly"
```

TizenClaw's AI understands natural language — you do not need to use exact
phrases.

### Running Commands Directly

If you prefer to skip the AI and run commands directly:

| What you want to do | Command |
|---|---|
| Start cleaning (automatic) | `robotic-vacuum-cli start --mode auto` |
| Start cleaning a specific area | `robotic-vacuum-cli start --mode part` |
| Repeat cleaning the same area | `robotic-vacuum-cli start --mode repeat` |
| Map the room then clean | `robotic-vacuum-cli start --mode map` |
| Stop the vacuum | `robotic-vacuum-cli stop` |
| Pause cleaning | `robotic-vacuum-cli pause` |
| Send back to dock | `robotic-vacuum-cli dock` |
| Check battery and status | `robotic-vacuum-cli status` |
| Set suction to high | `robotic-vacuum-cli turbo --level on` |
| Set suction to quiet mode | `robotic-vacuum-cli turbo --level silence` |
| Turn off extra suction | `robotic-vacuum-cli turbo --level off` |

### What the Cleaning Modes Mean

| Mode | What it does |
|---|---|
| `auto` | Vacuum cleans the whole reachable area automatically |
| `part` | Cleans a specific spot or limited section |
| `repeat` | Cleans the same area multiple times for a deeper clean |
| `manual` | You direct the vacuum manually |
| `map` | Maps the room layout first, then cleans using the map |

### Reading the Status Output

The `status` command prints a result like this:

```json
{
  "status": "ok",
  "battery_pct": 85,
  "movement": "charging",
  "cleaning_mode": "stop",
  "turbo_mode": "off"
}
```

| Field | Meaning |
|---|---|
| `battery_pct` | Battery percentage (0–100) |
| `movement` | What the vacuum is currently doing (`cleaning`, `homing`, `charging`, `idle`, `pause`) |
| `cleaning_mode` | The cleaning mode that is active or was last used |
| `turbo_mode` | Current suction level (`on`, `off`, or `silence`) |

---

## Troubleshooting

**"401 Unauthorized" error**

Your access token has expired or is invalid. TizenClaw should renew it
automatically, but if it fails, repeat Step 2 to get a fresh authorization code
and new tokens. Update the `access_token` and `refresh_token` fields in the
config file.

**The vacuum does not respond**

1. Open the SmartThings app on your phone — can you see and control the vacuum
   there? If not, the vacuum may be offline or out of Wi-Fi range.
2. Check that your computer has internet access.
3. Run `robotic-vacuum-cli status` and check the error message.

**"No such file or directory" when opening the config file**

The config directory may not exist yet. Create it and the file:

```bash
mkdir -p ~/.tizenclaw/data/config
cp ~/strawhats/tizen_claw/tizenclaw/data/config/robotic_vacuum_config.json \
   ~/.tizenclaw/data/config/
```

Then edit the copied file (Step 4).

**The tool works but the vacuum ignores the command**

The vacuum hardware may be refusing the command because it is in an error
state, its battery is critically low, or it is already performing another task.
Check the vacuum's indicator lights and try again after it is idle.

**The device ID is wrong**

Re-run the Step 3 command and double-check that you copied the `deviceId` from
the correct device (the Jet Bot, not a different SmartThings device like a TV
or light).

---

## Remote Access via Web Dashboard

TizenClaw has a built-in HTTP server that lets you send commands from any
device on your network — another computer, a phone, a tablet — without needing
the `tizenclaw-cli` program installed there. You can use it from a browser or
from the terminal with `curl`.

The web dashboard is already enabled by default. No extra configuration is
needed.

### Step 1 — Check the Port

The dashboard listens on a different port depending on where TizenClaw is running:

| Where TizenClaw is installed | Dashboard port |
|---|---|
| Tizen device or emulator (`deploy.sh`) | **9090** |
| Host Linux PC (`deploy_host.sh`) | **9091** |

You can verify this by looking at
`tizenclaw/data/config/channel_config.json` in the project folder.

### Step 2 — Find the IP Address of the Device Running TizenClaw

You need the IP address of the machine where TizenClaw is running so you can
connect to it from another device.

**If TizenClaw is running on your Linux PC:**

```bash
ip route get 1.1.1.1 | awk '{print $7; exit}'
```

This prints one line — that is the IP address to use (e.g. `192.168.1.42`).

**If TizenClaw is running on the Tizen emulator or device:**

```bash
sdb shell ip addr show
```

Look through the output for a line starting with `inet` that is NOT
`127.0.0.1`. The number before the `/` is the IP address (e.g. `10.0.2.15`).

### Step 3 — Test That the Dashboard Is Reachable

From any machine on the same network, open a terminal and run (replace
`192.168.1.42` and the port with your values from Steps 1 and 2):

```bash
curl http://192.168.1.42:9090/api/status
```

If you see a JSON response, the dashboard is reachable. If you see "connection
refused", check that TizenClaw is running and that both devices are on the
same Wi-Fi network.

### Step 4 — Send Vacuum Commands Remotely

The chat endpoint accepts plain English prompts and does not require a
password. Replace `192.168.1.42:9090` with your address and port:

**Start cleaning:**
```bash
curl -X POST http://192.168.1.42:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Start the robot vacuum in auto mode"}'
```

**Send back to dock:**
```bash
curl -X POST http://192.168.1.42:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Send the vacuum back to its dock"}'
```

**Check battery and status:**
```bash
curl -X POST http://192.168.1.42:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "What is the vacuum battery level and current status?"}'
```

**Set suction to quiet mode:**
```bash
curl -X POST http://192.168.1.42:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Set the vacuum suction to quiet mode"}'
```

Each response looks like:

```json
{
  "status": "ok",
  "session_id": "web-1",
  "response": "The vacuum has started cleaning in auto mode."
}
```

The `session_id` field is optional — if you include the same value in
follow-up requests, the AI remembers the context of your conversation:

```bash
curl -X POST http://192.168.1.42:9090/api/chat \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Now pause it", "session_id": "web-1"}'
```

### Step 5 — (Optional) Log In for Secured Endpoints

The chat endpoint is open, but some endpoints (like system status and config
changes) require you to log in first. The default password is `admin`.

**Log in and get a token:**

```bash
curl -X POST http://192.168.1.42:9090/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"password": "admin"}'
```

Response:

```json
{"status": "ok", "token": "abc123def456..."}
```

**Use the token for secured endpoints:**

```bash
curl http://192.168.1.42:9090/api/status \
  -H "Authorization: Bearer abc123def456..."
```

### Step 6 — Change the Default Password (Recommended)

The default password `admin` is well-known. Change it after your first login,
especially if your device is on a shared network.

```bash
curl -X POST http://192.168.1.42:9090/api/auth/change_password \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer abc123def456..." \
  -d '{"old_password": "admin", "new_password": "your-new-password"}'
```

### Web Dashboard Troubleshooting

**"Connection refused" when running curl**

- Make sure TizenClaw is actually running (check with `./deploy_host.sh --status` on host Linux)
- Confirm you are using the right port (9090 for Tizen, 9091 for host Linux)
- Check both devices are on the same Wi-Fi network

**"Invalid password" when logging in**

The password has been changed from the default. If you have forgotten it,
delete `~/.tizenclaw/admin_password.json` (host Linux) or
`/opt/usr/share/tizenclaw/admin_password.json` (Tizen) and restart
TizenClaw — it will reset to `admin`.

**The chat response says it cannot find or use the tool**

Make sure TizenClaw was rebuilt and deployed after the `robotic_vacuum_config.json`
was filled in. The config file must be present on the same machine where TizenClaw
is running, not just on the machine sending the curl commands.
