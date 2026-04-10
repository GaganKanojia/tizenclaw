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
// rvc_command <N>. No internet or SmartThings required; works offline.
//
// To add a newly discovered command:
//   1. Update the kCommands table entry (change "cmdN" name and description).
//   2. Update tool.md with the real subcommand name and description.
//   3. Rebuild: ./deploy.sh -a x86_64 -d <device>

#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/wait.h>

namespace {

constexpr const char kUsage[] = R"(Usage:
  rvc-local-cli <subcommand> [options]

Named subcommands (known):
  mapping     Start room mapping (rvc_command 1)
  clean-map   Map and clean simultaneously (rvc_command 2)

Placeholder subcommands (update when function is discovered):
  cmd3 .. cmd15   Run rvc_command 3..15 by name

Run by number:
  run --number <N>   Run rvc_command N directly (N: 1-15)

Utility:
  list   Show all command numbers, names, and descriptions as JSON

All output is JSON. rvc_command itself produces no output.
)";

// Table of all 15 known/placeholder rvc_command mappings.
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

// Looks up the entry for a given command number. Returns nullptr if not found.
const RvcEntry* FindByNumber(int number) {
  for (int i = 0; i < kCommandCount; ++i)
    if (kCommands[i].number == number) return &kCommands[i];
  return nullptr;
}

// Looks up the entry for a given subcommand name. Returns nullptr if not found.
const RvcEntry* FindByName(const std::string& name) {
  for (int i = 0; i < kCommandCount; ++i)
    if (name == kCommands[i].name) return &kCommands[i];
  return nullptr;
}

// Executes rvc_command N and returns a JSON result string.
// rvc_command produces no stdout output; success is determined by exit code.
std::string RunRvcCommand(int number) {
  const RvcEntry* entry = FindByNumber(number);
  std::string action = entry ? entry->name
                             : ("cmd" + std::to_string(number));

  std::string cmd = "rvc_command " + std::to_string(number);
  int raw_ret = std::system(cmd.c_str());
  int exit_code = WEXITSTATUS(raw_ret);

  if (exit_code == 0)
    return "{\"status\":\"ok\","
           "\"command\":" + std::to_string(number) + ","
           "\"action\":\"" + action + "\"}";

  return "{\"status\":\"error\","
         "\"command\":" + std::to_string(number) + ","
         "\"action\":\"" + action + "\","
         "\"exit_code\":" + std::to_string(exit_code) + "}";
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
        int n = std::stoi(argv[i + 1]);
        return n;
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
    std::cout << RunRvcCommand(number) << std::endl;
    return 0;
  }

  // ── named subcommand ──────────────────────────────────────────────────────
  const RvcEntry* entry = FindByName(cmd);
  if (entry) {
    std::cout << RunRvcCommand(entry->number) << std::endl;
    return 0;
  }

  // ── unknown ───────────────────────────────────────────────────────────────
  PrintUsage();
  return 1;
}
