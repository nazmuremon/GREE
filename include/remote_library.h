#pragma once

#include <Arduino.h>

struct RemoteButton {
  const char* id;
  const char* label;
  const char* group;
  const char* variant;
  uint8_t span;
};

struct RemoteModel {
  const char* id;
  const char* name;
  const char* company;
  const char* category;
  const char* layout;
  const char* engine;
  const char* finish;
  const char* note;
  const RemoteButton* buttons;
  uint8_t buttonCount;
};

extern const RemoteModel kRemoteLibrary[];
extern const size_t kRemoteLibraryCount;

const RemoteModel* findRemoteModel(const String& id);
const RemoteButton* findRemoteButton(const RemoteModel& remote,
                                     const String& buttonId);
String getLibraryJson();
