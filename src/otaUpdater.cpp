#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include "version.h"
#include "otaUpdater.h"
#include "globals.h"
#include "constants.h"
#include "debug.h"

String getLatestReleaseTag(const char* repoOwner, const char* repoName) {
    HTTPClient http;
    String apiUrl = "https://api.github.com/repos/" + String(repoOwner) + "/" + String(repoName) + "/releases/latest";
    DBG_VERBOSE("OTA API: %s\n", apiUrl.c_str());

    if (http.begin(clientSecure, apiUrl)) {
      int httpCode = http.GET();
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();

        int tagPos = payload.indexOf("\"tag_name\":");
        if (tagPos != -1) {
          int startQuote = payload.indexOf("\"", tagPos + 11);
          int endQuote = payload.indexOf("\"", startQuote + 1);
          if (startQuote != -1 && endQuote != -1) {
            return payload.substring(startQuote + 1, endQuote);
          }
        }

        DBG_ERROR("OTA: tag not found\n");
      } else {
        DBG_ERROR("OTA GET failed: %d\n", httpCode);
      }
    } else {
      DBG_ERROR("OTA: GitHub connect failed\n");
    }
    http.end();
    return "";
  }

void checkForUpdates() {
    String latestTag = getLatestReleaseTag(YOUR_GITHUB_USERNAME, YOUR_REPO_NAME);
    DBG_VERBOSE("Version: %s -> %s\n", FIRMWARE_VERSION, latestTag.c_str());

    if (latestTag != "") {
      if (latestTag != FIRMWARE_VERSION) {
        const esp_partition_t* update_partition = esp_ota_get_next_update_partition(NULL);

        String firmwareURL = "https://github.com/" + String(YOUR_GITHUB_USERNAME) + "/" + String(YOUR_REPO_NAME) + "/releases/download/" + latestTag + "/firmware.bin";
        DBG_INFO("OTA URL: %s\n", firmwareURL.c_str());

        HTTPClient redirectHttp;
        redirectHttp.begin(clientSecure, firmwareURL);

        const char *headerKeys[] = {"Location"};
        redirectHttp.collectHeaders(headerKeys, 1);

        int redirectCode = redirectHttp.GET();
        if (redirectCode == HTTP_CODE_FOUND || redirectCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String redirectedURL = redirectHttp.header("Location");
          if (redirectedURL.length() > 0) {
            DBG_VERBOSE("OTA redirect: %s\n", redirectedURL.c_str());
            firmwareURL = redirectedURL;
          } else {
            DBG_ERROR("OTA: no Location header\n");
          }

          HTTPUpdate httpUpdate;

          httpUpdate.onStart([](){ DBG_INFO("OTA started\n"); });
          httpUpdate.onEnd([](){ DBG_INFO("OTA finished\n"); });
          httpUpdate.onProgress([](int cur, int total) { DBG_VERBOSE("OTA: %d/%d\n", cur, total); });
          httpUpdate.onError([](int err) { DBG_ERROR("OTA error: %d\n", err); });

          t_httpUpdate_return ret = httpUpdate.update(clientSecure, firmwareURL);

          if (ret == HTTP_UPDATE_OK) {
            DBG_INFO("OTA success!\n");
            esp_ota_set_boot_partition(update_partition);
          } else {
            DBG_ERROR("OTA failed: %d\n", httpUpdate.getLastError());
          }

        } else {
          DBG_ERROR("OTA redirect error: %d\n", redirectCode);
        }
        redirectHttp.end();
      } else {
        DBG_VERBOSE("Firmware up to date\n");
      }
    } else {
      DBG_ERROR("OTA: empty release tag\n");
    }
}
