#include "env.h"

#define ETH_PHY_TYPE        ETH_PHY_RTL8201
#define ETH_PHY_ADDR         0
#define ETH_PHY_MDC         16
#define ETH_PHY_MDIO        17
#define ETH_PHY_POWER       -1
#define ETH_CLK_MODE        ETH_CLOCK_GPIO0_IN

#include <ETH.h>

// States to keep track of

#define STATE_READY          1  // Ethernet is up and we are not in lockout; button can be pushed
#define STATE_LOCKOUT        2  // Ethernet is up but button was pushed within lockout delay
#define STATE_ERROR          3  // Ethernet is down or no DHCP lease assigned

// LED patterns
#define BLINK_LED_LEN        2
const int blink_led[] = {1000, 1000}; // Blink LED 1s on, 1s off
#define SOS_LED_LEN         17
const int sos_led[]   = {250, 250, 250, 250, 250, 250, 1000, 250, 1000, 250, 1000, 250, 250, 250, 250, 250, 1000}; // SOS LED is ...---... with 250ms pauses and a 1000ms pause at the end

// Useful declared variables
unsigned long runtime = 0;
unsigned long uptime = 0;
static bool eth_connected = false;

// WARNING: onEvent is called from a separate FreeRTOS task (thread)!
// From https://github.com/espressif/arduino-esp32/blob/master/libraries/Ethernet/examples/ETH_LAN8720/ETH_LAN8720.ino
void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname("wESP-one");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Connected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH);
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default: break;
  }
}

void testClient(const char *host, uint16_t port) {
  Serial.print("\nconnecting to ");
  Serial.println(host);

  NetworkClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available());
  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("closing connection\n");
  client.stop();
}

void setup() {
  Serial.begin(115200);
  Network.onEvent(onEvent);
  ETH.begin();
}

void loop() {
  if (eth_connected) {
    testClient("google.com", 80);
  } else {
    uptime = 0;
  }
  delay(60000);
  uptime = uptime + 60;
  Serial.write("Uptime: ");
  Serial.println(uptime);
}
