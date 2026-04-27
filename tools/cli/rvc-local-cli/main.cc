/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// rvc-local-cli — Direct on-device control of Samsung robot vacuum via
// the Tizen TIDL RpcPort interface (RVCTizenClawService).
// Sends "vc_command <N>" to com.samsung.vr.robot-main which executes it.
// No internet or SmartThings required; works offline.
//
// To add a newly discovered command:
//   1. Update the kCommands table entry (change "cmdN" name and description).
//   2. Update tool.md with the real subcommand name and description.
//   3. Rebuild: ./deploy.sh -a aarch64 -d <device>

#include <csignal>
#include <iostream>
#include <string>
#include <unistd.h>

#include "tidl/rvc_local.h"

namespace {

constexpr const char kUsage[] = R"(Usage:
  rvc-local-cli <subcommand> [options]

Named subcommands (known):
  mapping     Start room mapping (vc_command 1 via TIDL)
  clean-map   Map and clean simultaneously (vc_command 2 via TIDL)

Placeholder subcommands (update when function is discovered):
  cmd3 .. cmd15   Run vc_command 3..15 by name

Run by number:
  run --number <N>   Run vc_command N directly (N: 1-15)

Utility:
  list   Show all command numbers, names, and descriptions as JSON

All output is JSON.
)";

// Table of all 15 known/placeholder vc_command mappings.
// Update name and description here (and in tool.md) when a command is
// identified. The name must match the subcommand string passed on the CLI.
struct RvcEntry {
  int         number;
  const char* name;
  const char* description;
};

static const RvcEntry kCommands[] = {
  {1,  "mapping",   "Start room mapping only"},
  {2,  "clean-map", "Map and clean simultaneously"},
  {3,  "cmd3",      "Unknown — update when function is discovered"},
  {4,  "cmd4",      "Unknown — update when function is discovered"},
  {5,  "cmd5",      "Unknown — update when function is discovered"},
  {6,  "cmd6",      "Unknown — update when function is discovered"},
  {7,  "cmd7",      "Unknown — update when function is discovered"},
  {8,  "cmd8",      "Unknown — update when function is discovered"},
  {9,  "cmd9",      "Unknown — update when function is discovered"},
  {10, "cmd10",     "Unknown — update when function is discovered"},
  {11, "cmd11",     "Unknown — update when function is discovered"},
  {12, "cmd12",     "Unknown — update when function is discovered"},
  {13, "cmd13",     "Unknown — update when function is discovered"},
  {14, "cmd14",     "Unknown — update when function is discovered"},
  {15, "cmd15",     "Unknown — update when function is discovered"},
};

static constexpr int kCommandCount =
    static_cast<int>(sizeof(kCommands) / sizeof(kCommands[0]));

// ─── SIGALRM-based timeout for blocking SendCommand ──────────────────────────

static volatile sig_atomic_t g_tidl_timed_out = 0;

static void OnAlarm(int) { g_tidl_timed_out = 1; }

// Arms a SIGALRM that interrupts the blocking rpc_port read after |seconds|.
// rpc_port_parcel_create_from_port returns EINTR → ConsumeCommand exits →
// SendCommand throws InvalidProtocolException, which we re-map to a timeout.
static void ArmTimeout(unsigned int seconds) {
  g_tidl_timed_out = 0;
  struct sigaction sa {};
  sa.sa_handler = OnAlarm;
  // SA_RESTART intentionally omitted so blocking reads return EINTR.
  sigaction(SIGALRM, &sa, nullptr);
  alarm(seconds);
}

static void CancelTimeout() { alarm(0); }

// ─── TIDL listener (minimal — CLI only needs connected/rejected) ──────────────

class CliListener
    : public rpc_port::rvc_local::proxy::RVCTizenClawService::IEventListener {
 public:
  void OnConnected() override {}
  void OnDisconnected() override {}
  void OnRejected() override {}
};

// ─── helpers ─────────────────────────────────────────────────────────────────

const RvcEntry* FindByNumber(int number) {
  for (int i = 0; i < kCommandCount; ++i)
    if (kCommands[i].number == number) return &kCommands[i];
  return nullptr;
}

const RvcEntry* FindByName(const std::string& name) {
  for (int i = 0; i < kCommandCount; ++i)
    if (name == kCommands[i].name) return &kCommands[i];
  return nullptr;
}

// Sends "vc_command <number>" to robot-main via TIDL RpcPort and returns JSON.
std::string SendViaTidl(int number) {
  const RvcEntry* entry = FindByNumber(number);
  std::string action  = entry ? entry->name : ("cmd" + std::to_string(number));
  std::string command = "vc_command " + std::to_string(number);

  using namespace rpc_port::rvc_local::proxy;

  CliListener listener;
  try {
    RVCTizenClawService proxy(&listener, "com.samsung.vr.robot-main");
    proxy.Connect(/*sync=*/true);

    ArmTimeout(5);
    int ret = proxy.SendCommand(command, 0);
    CancelTimeout();

    return "{\"status\":\"ok\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"ret\":" + std::to_string(ret) + "}";

  } catch (const InvalidProtocolException&) {
    CancelTimeout();
    if (g_tidl_timed_out)
      return "{\"status\":\"error\","
             "\"command\":" + std::to_string(number) + ","
             "\"action\":\"" + action + "\","
             "\"message\":\"timeout — robot-main did not respond within 5 s\"}";
    return "{\"status\":\"error\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"message\":\"invalid protocol response from robot-main\"}";
  } catch (const InvalidIDException&) {
    return "{\"status\":\"error\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"message\":\"invalid app ID — robot-main not found\"}";
  } catch (const InvalidIOException&) {
    return "{\"status\":\"error\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"message\":\"IO error connecting to robot-main\"}";
  } catch (const PermissionDeniedException&) {
    return "{\"status\":\"error\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"message\":\"permission denied\"}";
  } catch (const NotConnectedSocketException&) {
    return "{\"status\":\"error\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"message\":\"not connected\"}";
  } catch (...) {
    CancelTimeout();
    return "{\"status\":\"error\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\","
           "\"message\":\"unexpected internal error\"}";
  }
}

// Returns the full command table as a JSON object.
std::string ListCommands() {
  std::string out = "{\"status\":\"ok\",\"commands\":[";
  for (int i = 0; i < kCommandCount; ++i) {
    if (i > 0) out += ",";
    out += "{\"number\":" + std::to_string(kCommands[i].number) + ","
           "\"name\":\"" + kCommands[i].name + "\","
           "\"description\":\"" + kCommands[i].description + "\"}";
  }
  out += "]}";
  return out;
}

// Parses "--number N" from argv, starting at index start.
// Returns -1 if not found or invalid.
int ParseNumber(int argc, char* argv[], int start) {
  for (int i = start; i < argc - 1; ++i) {
    if (std::string(argv[i]) == "--number") {
      try {
        return std::stoi(argv[i + 1]);
      } catch (...) {
        return -1;
      }
    }
  }
  return -1;
}

void PrintUsage() {
  std::cerr << kUsage;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  std::string cmd = argv[1];

  // ── list ──────────────────────────────────────────────────────────────────
  if (cmd == "list") {
    std::cout << ListCommands() << std::endl;
    return 0;
  }

  // ── run --number N ────────────────────────────────────────────────────────
  if (cmd == "run") {
    int number = ParseNumber(argc, argv, 2);
    if (number < 1 || number > 15) {
      std::cout << "{\"status\":\"error\","
                   "\"message\":\"--number must be between 1 and 15\"}"
                << std::endl;
      return 1;
    }
    std::cout << SendViaTidl(number) << std::endl;
    return 0;
  }

  // ── named subcommand ──────────────────────────────────────────────────────
  const RvcEntry* entry = FindByName(cmd);
  if (entry) {
    std::cout << SendViaTidl(entry->number) << std::endl;
    return 0;
  }

  // ── unknown ───────────────────────────────────────────────────────────────
  std::cout << "{\"status\":\"error\",\"message\":\"unknown subcommand: "
            << cmd << "\"}" << std::endl;
  PrintUsage();
  return 1;
}
