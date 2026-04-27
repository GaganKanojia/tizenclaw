/*
 * rvc-tidl-test — Diagnostic tool for com.samsung.vr.robot-main TIDL interface.
 *
 * Validates the full RPC roundtrip:
 *   connect → SendCommand("vc_command N", 0) → receive __Result → report
 *
 * Usage:
 *   rvc-tidl-test [N]    N = vc_command number (1–15), default: 1
 *   rvc-tidl-test list   print known command numbers to stdout as JSON
 *
 * Output: one JSON line on stdout + step-by-step diagnostics on stderr.
 * Exit code: 0 = success, 1 = any error.
 */

#include <csignal>
#include <ctime>
#include <iostream>
#include <string>
#include <unistd.h>

#include "tidl/rvc_local.h"

// ─── timing ──────────────────────────────────────────────────────────────────

static long long NowMs() {
  struct timespec ts {};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<long long>(ts.tv_sec) * 1000LL +
         static_cast<long long>(ts.tv_nsec) / 1000000LL;
}

// ─── SIGALRM timeout ─────────────────────────────────────────────────────────

static volatile sig_atomic_t g_timed_out = 0;

static void OnAlarm(int) { g_timed_out = 1; }

static void ArmTimeout(unsigned int seconds) {
  g_timed_out = 0;
  struct sigaction sa {};
  sa.sa_handler = OnAlarm;
  // SA_RESTART intentionally omitted — blocking reads must return EINTR.
  sigaction(SIGALRM, &sa, nullptr);
  alarm(seconds);
}

static void CancelTimeout() { alarm(0); }

// ─── TIDL listener ───────────────────────────────────────────────────────────

class DiagListener
    : public rpc_port::rvc_local::proxy::RVCTizenClawService::IEventListener {
 public:
  void OnConnected() override {
    std::cerr << "[rvc-tidl-test] connected to RVCTizenClawService\n";
  }
  void OnDisconnected() override {
    std::cerr << "[rvc-tidl-test] disconnected\n";
  }
  void OnRejected() override {
    std::cerr << "[rvc-tidl-test] connection rejected by robot-main\n";
  }
};

// ─── known commands ──────────────────────────────────────────────────────────

struct Entry { int number; const char* name; };
static const Entry kCommands[] = {
  {1,  "mapping"},
  {2,  "clean-map"},
};
static constexpr int kKnownCount =
    static_cast<int>(sizeof(kCommands) / sizeof(kCommands[0]));

static const char* NameOf(int number) {
  for (int i = 0; i < kKnownCount; ++i)
    if (kCommands[i].number == number) return kCommands[i].name;
  return nullptr;
}

// ─── core test ───────────────────────────────────────────────────────────────

static int RunTest(int number) {
  using namespace rpc_port::rvc_local::proxy;

  std::string command = "vc_command " + std::to_string(number);
  const char* name    = NameOf(number);

  std::cerr << "[rvc-tidl-test] target   : com.samsung.vr.robot-main\n"
            << "[rvc-tidl-test] port     : RVCTizenClawService\n"
            << "[rvc-tidl-test] command  : " << command << "\n";
  if (name)
    std::cerr << "[rvc-tidl-test] alias    : " << name << "\n";
  std::cerr << "[rvc-tidl-test] connecting (sync)...\n";

  long long t0 = NowMs();

  DiagListener listener;
  try {
    RVCTizenClawService proxy(&listener, "com.samsung.vr.robot-main");
    proxy.Connect(/*sync=*/true);

    std::cerr << "[rvc-tidl-test] sending " << command << "...\n";
    ArmTimeout(5);
    int ret = proxy.SendCommand(command, 0);
    CancelTimeout();

    long long elapsed = NowMs() - t0;
    std::cerr << "[rvc-tidl-test] got __Result: " << ret
              << " (" << elapsed << " ms)\n";

    std::cout << "{\"status\":\"ok\","
              << "\"command\":" << number << ","
              << "\"action\":\"" << (name ? name : command.c_str()) << "\","
              << "\"ret\":" << ret << ","
              << "\"elapsed_ms\":" << elapsed << "}"
              << std::endl;
    return 0;

  } catch (const InvalidProtocolException&) {
    CancelTimeout();
    long long elapsed = NowMs() - t0;
    std::string msg = g_timed_out
        ? "timeout — no __Result within 5 s"
        : "invalid protocol response from robot-main";
    std::cerr << "[rvc-tidl-test] error: " << msg << "\n";
    std::cout << "{\"status\":\"error\","
              << "\"command\":" << number << ","
              << "\"elapsed_ms\":" << elapsed << ","
              << "\"message\":\"" << msg << "\"}"
              << std::endl;
    return 1;

  } catch (const InvalidIDException&) {
    std::string msg = "invalid app ID — robot-main not found or not running";
    std::cerr << "[rvc-tidl-test] error: " << msg << "\n";
    std::cout << "{\"status\":\"error\","
              << "\"command\":" << number << ","
              << "\"message\":\"" << msg << "\"}"
              << std::endl;
    return 1;

  } catch (const InvalidIOException&) {
    CancelTimeout();
    std::string msg = "IO error — RPC port unavailable";
    std::cerr << "[rvc-tidl-test] error: " << msg << "\n";
    std::cout << "{\"status\":\"error\","
              << "\"command\":" << number << ","
              << "\"message\":\"" << msg << "\"}"
              << std::endl;
    return 1;

  } catch (const PermissionDeniedException&) {
    std::string msg = "permission denied — check SMACK label / privileges";
    std::cerr << "[rvc-tidl-test] error: " << msg << "\n";
    std::cout << "{\"status\":\"error\","
              << "\"command\":" << number << ","
              << "\"message\":\"" << msg << "\"}"
              << std::endl;
    return 1;

  } catch (const NotConnectedSocketException&) {
    CancelTimeout();
    std::string msg = "not connected — Connect() succeeded but port is null";
    std::cerr << "[rvc-tidl-test] error: " << msg << "\n";
    std::cout << "{\"status\":\"error\","
              << "\"command\":" << number << ","
              << "\"message\":\"" << msg << "\"}"
              << std::endl;
    return 1;

  } catch (...) {
    CancelTimeout();
    std::string msg = "unexpected internal error";
    std::cerr << "[rvc-tidl-test] error: " << msg << "\n";
    std::cout << "{\"status\":\"error\","
              << "\"command\":" << number << ","
              << "\"message\":\"" << msg << "\"}"
              << std::endl;
    return 1;
  }
}

// ─── list ─────────────────────────────────────────────────────────────────────

static void ListCommands() {
  std::cout << "{\"commands\":[";
  for (int n = 1; n <= 15; ++n) {
    if (n > 1) std::cout << ",";
    const char* name = NameOf(n);
    std::string fallback = name ? std::string(name) : ("cmd" + std::to_string(n));
    std::cout << "{\"number\":" << n << ",\"name\":\"" << fallback << "\"}";
  }
  std::cout << "]}" << std::endl;
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
  if (argc >= 2 && std::string(argv[1]) == "list") {
    ListCommands();
    return 0;
  }

  int number = 1;
  if (argc >= 2) {
    try {
      number = std::stoi(argv[1]);
    } catch (...) {
      std::cerr << "Usage: rvc-tidl-test [N]   N = 1-15, default 1\n"
                << "       rvc-tidl-test list\n";
      return 1;
    }
    if (number < 1 || number > 15) {
      std::cerr << "Error: N must be between 1 and 15\n";
      return 1;
    }
  }

  return RunTest(number);
}
