#ifndef NUGGET_TOOLS_H
#define NUGGET_TOOLS_H

#include <app_nugget.h>
#include <application.h>
#include <nos/debug.h>
#include <nos/NuggetClientInterface.h>

#include <memory>
#include <string>

#define ASSERT_NO_ERROR(code, msg) \
  do { \
    int value = code; \
    ASSERT_EQ(value, app_status::APP_SUCCESS) \
        << value << " is " << nos::StatusCodeString(value) << msg; \
  } while(0)

namespace nugget_tools {

std::unique_ptr<nos::NuggetClientInterface> MakeNuggetClient();

bool RebootNugget(nos::NuggetClientInterface *client, uint8_t type);

// Returns an underestimate of the number of seconds waited.
uint32_t WaitForSleep();

bool WipeUserData(nos::NuggetClientInterface *client);

}  // namespace nugget_tools

#endif  // NUGGET_TOOLS_H
