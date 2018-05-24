#include "nugget_tools.h"

#include <app_nugget.h>
#include <nos/NuggetClient.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#ifdef ANDROID
#include <android-base/endian.h>
#include "nos/CitadeldProxyClient.h"
#else
#include "gflags/gflags.h"

DEFINE_string(nos_core_serial, "", "USB device serial number to open");
#endif  // ANDROID

#ifndef LOG
#define LOG(x) std::cerr << __FILE__ << ":" << __LINE__ << " " << #x << ": "
#endif  // LOG

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;

namespace nugget_tools {

namespace {

void WaitForHardReboot() {
  // POST (which takes ~50ms) runs on a hard-reboot, plus an
  // additional ~30ms for RO+RW verification.
  std::this_thread::sleep_for(std::chrono::milliseconds(80));
}

} // namesapce

std::unique_ptr<nos::NuggetClientInterface> MakeNuggetClient() {
#ifdef ANDROID
  std::unique_ptr<nos::NuggetClientInterface> client =
      std::unique_ptr<nos::NuggetClientInterface>(new nos::NuggetClient());
  client->Open();
  if (!client->IsOpen()) {
    client = std::unique_ptr<nos::NuggetClientInterface>(
        new nos::CitadeldProxyClient());
  }
  return client;
#else
  if (FLAGS_nos_core_serial.empty()) {
    const char *env_default = secure_getenv("CITADEL_DEVICE");
    if (env_default && *env_default) {
      FLAGS_nos_core_serial.assign(env_default);
      std::cerr << "Using CITADEL_DEVICE=" << FLAGS_nos_core_serial << "\n";
    }
  }
  return std::unique_ptr<nos::NuggetClientInterface>(
      new nos::NuggetClient(FLAGS_nos_core_serial));
#endif
}

bool CyclesSinceBoot(nos::NuggetClientInterface *client, uint32_t *cycles) {
  std::vector<uint8_t> buffer;
  buffer.reserve(sizeof(uint32_t));
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_CYCLES_SINCE_BOOT,
                      buffer, &buffer) != app_status::APP_SUCCESS) {
    perror("test");
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_CYCLES_SINCE_BOOT, ...) failed!\n";
    return false;
  };
  if (buffer.size() != sizeof(uint32_t)) {
    LOG(ERROR) << "Unexpected size of cycle count!\n";
    return false;
  }
  *cycles = le32toh(*reinterpret_cast<uint32_t *>(buffer.data()));
  return true;
}

bool RebootNugget(nos::NuggetClientInterface *client, uint8_t type) {
  // Capture the time here to allow for some tolerance on the reported time.
  auto start = high_resolution_clock::now();

  // See what time Nugget OS has now
  uint32_t pre_reboot;
  if (!CyclesSinceBoot(client, &pre_reboot)) {
    return false;
  }

  // Tell it to reboot: 0 = soft reboot, 1 = hard reboot
  std::vector<uint8_t> input_buffer(1, type);
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_REBOOT, input_buffer,
                      nullptr) != app_status::APP_SUCCESS) {
    LOG(ERROR) << "CallApp(..., NUGGET_PARAM_REBOOT, ...) failed!\n";
    return false;
  }

  if (!type) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  } else {
    WaitForHardReboot();
  }

  // See what time Nugget OS has after rebooting.
  uint32_t post_reboot;
  if (!CyclesSinceBoot(client, &post_reboot)) {
    return false;
  }

  // Hard reboots reset the clock to zero, but soft reboots should keep counting
  if (!type) {
    // Make sure time advanced
    if (post_reboot <= pre_reboot) {
      LOG(ERROR) << "pre_reboot time (" << pre_reboot << ") should be less than "
                 << "post_reboot time (" << post_reboot << ")\n";
      return false;
    }
    // Change this to elapsed time, not absolute time
    post_reboot -= pre_reboot;
  }

  // Verify that the Nugget OS counter shows a reasonable value.
  // Use the elapsed time +5% for the threshold.
  auto threshold_microseconds =
      duration_cast<microseconds>(high_resolution_clock::now() - start) *
          105 / 100;
  if (std::chrono::microseconds(post_reboot) > threshold_microseconds ) {
    LOG(ERROR) << "Counter is " << post_reboot
               << " but is expected to be less than "
               << threshold_microseconds.count() * 1.05 << "!\n";
    return false;
  }

  // Looks okay
  return true;
}

uint32_t WaitForSleep() {
  constexpr uint32_t wait_seconds = 5;
  std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));
  // TODO: Can we check it has gone to sleep?
  return wait_seconds;
}

bool WipeUserData(nos::NuggetClientInterface *client) {
  // Request wipe of user data which should hard reboot
  std::vector<uint8_t> buffer(4);
  *reinterpret_cast<uint32_t *>(buffer.data()) = htole32(ERASE_CONFIRMATION);
  if (client->CallApp(APP_ID_NUGGET, NUGGET_PARAM_NUKE_FROM_ORBIT,
                         buffer, nullptr) != app_status::APP_SUCCESS) {
    return false;
  }
  WaitForHardReboot();
  return true;
}

}  // namespace nugget_tools
