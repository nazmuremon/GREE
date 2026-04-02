#pragma once

// Copy this file to include/wifi_config.h and fill in your Wi-Fi details.
// If wifi_config.h is missing or left blank, the ESP32 starts its own
// fallback access point:
//   SSID: ESP32-IR-Bridge
//   Password: irbridge123
//
// When connected to your home Wi-Fi, the ESP serves the IR control page on:
//   http://<device-ip>/
// It also tries to expose a stable local hostname on many home networks:
//   http://gree.local/
//
// Optional:
// - Set WIFI_USE_STATIC_IP and the IP values below if you want the same LAN IP
//   every time.
// - Set WIFI_PUBLIC_BASE_URL if you expose the ESP32 through port forwarding,
//   DDNS, a reverse proxy, or a tunnel. Example:
//   #define WIFI_PUBLIC_BASE_URL "https://your-remote.example.com"
// - Public access from any network requires one of those external methods.

constexpr char kWifiSsid[] = "YOUR_WIFI_SSID";
constexpr char kWifiPassword[] = "YOUR_WIFI_PASSWORD";

#define WIFI_PUBLIC_BASE_URL ""

#define WIFI_USE_STATIC_IP 0
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
