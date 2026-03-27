#include <unity.h>

// Existing tests
extern void testCreateConfigFile();
extern void testHandleMediciones();
extern void testHandleConfiguracion();
extern void testSendDataGrafana();
extern void testCheckForUpdates();
extern void testGetLatestReleaseTag();

// Grafana message tests
extern void testCreateGrafanaMessage();
extern void testCreateGrafanaMessage_WithSensorId();
extern void testCreateGrafanaMessage_NegativeValues();

// ESPNowManager tests (PR #11 mesh functionality)
extern void testHasSeenPacket_NewPacket();
extern void testHasSeenPacket_AfterMarking();
extern void testHasSeenPacket_DifferentSequence();
extern void testHasSeenPacket_DifferentMAC();
extern void testHasSeenPacket_Timeout();
extern void testSeenPacketCache_CircularBuffer();
extern void testSensorDataMessage_PR11_HasNewFields();
extern void testSensorDataMessage_SizeIncreased();
extern void testHopCount_InitialValue();
extern void testHopCount_Decrement();
extern void testHopCount_ShouldForward();
extern void testHopCount_ZeroStopsForwarding();
extern void testSensorId_CanHoldLongNames();
extern void testSensorId_NullTermination();

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();

    // Basic infrastructure tests
    RUN_TEST(testCreateConfigFile);
    RUN_TEST(testHandleMediciones);
    RUN_TEST(testHandleConfiguracion);
    RUN_TEST(testCheckForUpdates);
    RUN_TEST(testGetLatestReleaseTag);

    // Grafana message formatting tests
    RUN_TEST(testCreateGrafanaMessage);
    RUN_TEST(testCreateGrafanaMessage_WithSensorId);
    RUN_TEST(testCreateGrafanaMessage_NegativeValues);
    RUN_TEST(testSendDataGrafana);

    // ESPNowManager: Duplicate detection (PR #11)
    RUN_TEST(testHasSeenPacket_NewPacket);
    RUN_TEST(testHasSeenPacket_AfterMarking);
    RUN_TEST(testHasSeenPacket_DifferentSequence);
    RUN_TEST(testHasSeenPacket_DifferentMAC);
    RUN_TEST(testHasSeenPacket_Timeout);
    RUN_TEST(testSeenPacketCache_CircularBuffer);

    // ESPNowManager: Message structure (PR #11)
    RUN_TEST(testSensorDataMessage_PR11_HasNewFields);
    RUN_TEST(testSensorDataMessage_SizeIncreased);

    // ESPNowManager: Hop count / forwarding (PR #11)
    RUN_TEST(testHopCount_InitialValue);
    RUN_TEST(testHopCount_Decrement);
    RUN_TEST(testHopCount_ShouldForward);
    RUN_TEST(testHopCount_ZeroStopsForwarding);

    // ESPNowManager: SensorId field (PR #11)
    RUN_TEST(testSensorId_CanHoldLongNames);
    RUN_TEST(testSensorId_NullTermination);

    return UNITY_END();
}
