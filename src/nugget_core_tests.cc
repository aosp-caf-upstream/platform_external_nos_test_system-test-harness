
#include <app_nugget.h>
#include <nos/NuggetClientInterface.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "nugget_tools.h"
#include "util.h"


using std::string;
using std::vector;
using std::unique_ptr;

namespace {

class NuggetCoreTest: public testing::Test {
 protected:
  static void SetUpTestCase();
  static void TearDownTestCase();

  static unique_ptr<nos::NuggetClientInterface> client;
  static vector<uint8_t> input_buffer;
  static vector<uint8_t> output_buffer;
};

unique_ptr<nos::NuggetClientInterface> NuggetCoreTest::client;

vector<uint8_t> NuggetCoreTest::input_buffer;
vector<uint8_t> NuggetCoreTest::output_buffer;

void NuggetCoreTest::SetUpTestCase() {
  client = nugget_tools::MakeNuggetClient();
  client->Open();
  input_buffer.reserve(0x4000);
  output_buffer.reserve(0x4000);
  EXPECT_TRUE(client->IsOpen()) << "Unable to connect";
}

void NuggetCoreTest::TearDownTestCase() {
  client->Close();
  client = unique_ptr<nos::NuggetClientInterface>();
}

TEST_F(NuggetCoreTest, GetVersionStringTest) {
  input_buffer.resize(0);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_VERSION, input_buffer, &output_buffer), "");
  ASSERT_GT(output_buffer.size(), 0u);
}

TEST_F(NuggetCoreTest, GetDeviceIdTest) {
  input_buffer.resize(0);
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_DEVICE_ID, input_buffer, &output_buffer), "");
  ASSERT_EQ(output_buffer.size(), 18u);
  for (size_t i = 0; i < output_buffer.size(); i++) {
    if (i == 8) {
      ASSERT_EQ(output_buffer[i], ':');
    } else if (i == 17) {
      ASSERT_EQ(output_buffer[i], '\0');
    } else {
      ASSERT_TRUE(std::isxdigit(output_buffer[i]));
    }
  }
}

TEST_F(NuggetCoreTest, SoftRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get(), NUGGET_REBOOT_SOFT));
}

TEST_F(NuggetCoreTest, HardRebootTest) {
  ASSERT_TRUE(nugget_tools::RebootNugget(client.get(), NUGGET_REBOOT_HARD));
}

TEST_F(NuggetCoreTest, WipeUserData) {
  ASSERT_TRUE(nugget_tools::WipeUserData(client.get()));
}

TEST_F(NuggetCoreTest, GetLowPowerStats) {
  struct nugget_app_low_power_stats stats;
  vector<uint8_t> buffer;

  buffer.reserve(1000);                         // Much more than needed
  ASSERT_NO_ERROR(NuggetCoreTest::client->CallApp(
      APP_ID_NUGGET, NUGGET_PARAM_GET_LOW_POWER_STATS,
      buffer, &buffer), "");
  ASSERT_GE(buffer.size(), sizeof(stats));

  memcpy(&stats, buffer.data(), sizeof(stats));

  /* We must have booted once and been awake long enough to reply, but that's
   * about all we can be certain of. */
  ASSERT_GT(stats.hard_reset_count, 0UL);
  ASSERT_GT(stats.time_since_hard_reset, 0UL);
  ASSERT_GT(stats.time_spent_awake, 0UL);
}

}  // namespace
