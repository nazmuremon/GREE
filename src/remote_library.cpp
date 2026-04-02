#include "remote_library.h"

namespace {

String escapeJson(const String& input) {
  String escaped;
  escaped.reserve(input.length() + 8);

  for (size_t index = 0; index < input.length(); index++) {
    const char current = input[index];
    switch (current) {
      case '\"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped += current;
        break;
    }
  }

  return escaped;
}

const RemoteButton kGreeAcButtons[] = {
    {"power", "Power", "top", "power", 1},
    {"wifi", "WiFi", "top", "dark", 1},
    {"light", "Light", "top", "", 1},
    {"temp_up", "Temp +", "temperature", "temperature", 1},
    {"temp_down", "Temp -", "temperature", "temperature", 1},
    {"mode", "Mode", "core", "accent", 1},
    {"fan", "Fan", "core", "dark", 1},
    {"swing", "Swing V", "core", "", 1},
    {"swing_h", "Swing H", "core", "", 1},
    {"timer", "Timer", "core", "", 1},
    {"fan_only", "Fan", "modes", "dark", 1},
    {"auto", "Auto", "modes", "dark", 1},
    {"cool", "Cool", "climate", "accent", 1},
    {"heat", "Heat", "climate", "accent", 1},
    {"dry", "Dry", "climate", "dark", 1},
    {"sleep", "Sleep", "climate", "", 1},
    {"turbo", "Turbo", "climate", "accent", 1},
    {"xfan", "X-Fan", "climate", "", 1},
    {"ifeel", "I Feel", "climate", "", 1},
    {"display", "Display", "climate", "", 1},
    {"energy", "Eco", "climate", "", 1},
};

}  // namespace

const RemoteModel kRemoteLibrary[] = {{
    "gree_ac",
    "Gree YAW1F AC",
    "Gree",
    "Air Conditioner",
    "ac",
    "native-ac",
    "frost",
    "Built-in YAW1F-style Gree protocol with a remote LCD-style web UI.",
    kGreeAcButtons,
    static_cast<uint8_t>(sizeof(kGreeAcButtons) / sizeof(kGreeAcButtons[0])),
}};

const size_t kRemoteLibraryCount =
    sizeof(kRemoteLibrary) / sizeof(kRemoteLibrary[0]);

const RemoteModel* findRemoteModel(const String& id) {
  for (size_t index = 0; index < kRemoteLibraryCount; index++) {
    if (id == kRemoteLibrary[index].id) {
      return &kRemoteLibrary[index];
    }
  }

  return nullptr;
}

const RemoteButton* findRemoteButton(const RemoteModel& remote,
                                     const String& buttonId) {
  for (uint8_t index = 0; index < remote.buttonCount; index++) {
    if (buttonId == remote.buttons[index].id) {
      return &remote.buttons[index];
    }
  }

  return nullptr;
}

String getLibraryJson() {
  String payload = "[";

  for (size_t remoteIndex = 0; remoteIndex < kRemoteLibraryCount;
       remoteIndex++) {
    if (remoteIndex > 0) {
      payload += ",";
    }

    const RemoteModel& remote = kRemoteLibrary[remoteIndex];
    payload += "{";
    payload += "\"id\":\"" + escapeJson(remote.id) + "\",";
    payload += "\"name\":\"" + escapeJson(remote.name) + "\",";
    payload += "\"company\":\"" + escapeJson(remote.company) + "\",";
    payload += "\"category\":\"" + escapeJson(remote.category) + "\",";
    payload += "\"layout\":\"" + escapeJson(remote.layout) + "\",";
    payload += "\"engine\":\"" + escapeJson(remote.engine) + "\",";
    payload += "\"finish\":\"" + escapeJson(remote.finish) + "\",";
    payload += "\"note\":\"" + escapeJson(remote.note) + "\",";
    payload += "\"buttons\":[";

    for (uint8_t buttonIndex = 0; buttonIndex < remote.buttonCount;
         buttonIndex++) {
      if (buttonIndex > 0) {
        payload += ",";
      }

      const RemoteButton& button = remote.buttons[buttonIndex];
      payload += "{";
      payload += "\"id\":\"" + escapeJson(button.id) + "\",";
      payload += "\"label\":\"" + escapeJson(button.label) + "\",";
      payload += "\"group\":\"" + escapeJson(button.group) + "\",";
      payload += "\"variant\":\"" + escapeJson(button.variant) + "\",";
      payload += "\"span\":" + String(button.span);
      payload += "}";
    }

    payload += "]}";
  }

  payload += "]";
  return payload;
}
