// Test-only implementation - no Arduino dependency
#include <unity.h>
#include <cstdio>
#include <cstring>

// Local implementation for testing (mirrors production logic)
void createGrafanaMessage(char* buffer, size_t bufferSize, const char* device, float temperature, float humidity, float co2, unsigned long long timestamp) {
    snprintf(buffer, bufferSize,
             "medicionesCO2,device=%s temp=%.2f,hum=%.2f,co2=%.2f %llu",
             device, temperature, humidity, co2, timestamp);
}

void testCreateGrafanaMessage() {
    char buffer[256];
    createGrafanaMessage(buffer, sizeof(buffer), "ASC02", 23.45, 55.67, 789.0, 1234567890000000000ULL);

    const char* expected = "medicionesCO2,device=ASC02 temp=23.45,hum=55.67,co2=789.00 1234567890000000000";
    TEST_ASSERT_EQUAL_STRING(expected, buffer);
}

void testCreateGrafanaMessage_WithSensorId() {
    char buffer[256];
    createGrafanaMessage(buffer, sizeof(buffer), "sensor-abc123", 25.0, 60.0, 450.0, 1000000000000000000ULL);

    const char* expected = "medicionesCO2,device=sensor-abc123 temp=25.00,hum=60.00,co2=450.00 1000000000000000000";
    TEST_ASSERT_EQUAL_STRING(expected, buffer);
}

void testCreateGrafanaMessage_NegativeValues() {
    char buffer[256];
    createGrafanaMessage(buffer, sizeof(buffer), "test", -10.5, 0.0, -1.0, 0ULL);

    const char* expected = "medicionesCO2,device=test temp=-10.50,hum=0.00,co2=-1.00 0";
    TEST_ASSERT_EQUAL_STRING(expected, buffer);
}
