#include "TempTests.h"

#include "json/Json.h"
#include <cstdio>

namespace synth::test {

void json() {
  // Parse
  auto result = json::parse(R"({
        "name": "Test",
        "value": 42,
        "enabled": true,
        "items": [1, 2, 3],
        "nested": { "key": "val" }
    })");

  if (!result.ok()) {
    std::printf("Parse error: %s\n", result.error.c_str());
    return;
  }

  auto& root = result.value;
  std::printf("name: %s\n", root["name"].asString().c_str());
  std::printf("value: %d\n", root["value"].asInt());
  std::printf("enabled: %s\n", root["enabled"].asBool() ? "true" : "false");
  std::printf("items[1]: %d\n", root["items"].at(1).asInt());
  std::printf("nested.key: %s\n", root["nested"]["key"].asString().c_str());

  // Round-trip
  std::string serialized = json::serialize(root);
  std::printf("\n--- Serialized ---\n%s", serialized.c_str());

  auto result2 = json::parse(serialized);
  if (!result2.ok()) {
    std::printf("Round-trip parse error: %s\n", result2.error.c_str());
    return;
  }
  std::printf("Round-trip OK\n");
}
} // namespace synth::test
