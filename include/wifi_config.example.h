#pragma once

// Copy this file to include/wifi_config.h and fill in your Wi-Fi details.
// 
// If no WiFi credentials are configured, the ESP32 starts a provisioning hotspot:
//   SSID: UNIVERSAL IR
//   Password: 12345678
//   URL: http://192.168.4.1
// You can also press the reset button for 5 seconds to re-enter provisioning mode.
//
// When connected to your home Wi-Fi, the ESP32 serves the IR control page on:
//   http://<device-ip>/
// And also exposes a stable local hostname:
//   http://irremote.local/
//
// Recommended for stable local access:
// - Keep WIFI_USE_STATIC_IP enabled and set the IP values below to match your
//   router subnet if you want the same LAN IP every time.
// - Example bookmark after setup:
//   http://192.168.1.60/
// - Set WIFI_PUBLIC_BASE_URL if you expose the ESP32 through port forwarding,
//   DDNS, a reverse proxy, or a tunnel. Example:
//   #define WIFI_PUBLIC_BASE_URL "https://your-remote.example.com"
// - Public access from any network requires one of those external methods.

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";

#define WIFI_PUBLIC_BASE_URL ""

#define WIFI_USE_STATIC_IP 1
#define WIFI_STATIC_IP1 192
#define WIFI_STATIC_IP2 168
#define WIFI_STATIC_IP3 1
#define WIFI_STATIC_IP4 60
#define WIFI_GATEWAY_IP1 192
#define WIFI_GATEWAY_IP2 168
#define WIFI_GATEWAY_IP3 1
#define WIFI_GATEWAY_IP4 1
#define WIFI_SUBNET_MASK1 255
#define WIFI_SUBNET_MASK2 255
#define WIFI_SUBNET_MASK3 255
#define WIFI_SUBNET_MASK4 0
#define WIFI_DNS1_IP1 1
#define WIFI_DNS1_IP2 1
#define WIFI_DNS1_IP3 1
#define WIFI_DNS1_IP4 1
#define WIFI_DNS2_IP1 8
#define WIFI_DNS2_IP2 8
#define WIFI_DNS2_IP3 8
#define WIFI_DNS2_IP4 8
