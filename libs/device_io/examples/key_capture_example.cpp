/* key_capture_example.cpp
 * Example usage of KeyCapture API
 *
 * Build:
 *   clang++ -std=c++17 -framework Cocoa \
 *     -I../include \
 *     ../src/KeyCapture.mm \
 *     key_capture_example.cpp \
 *     -o key_capture_example
 */

#include "device_io/KeyCapture.h"
#include <cstdio>

namespace device_io {

// Helper to get modifier name from keyCode
const char* getModifierName(uint16_t keyCode) {
  switch (keyCode) {
  case key_code::ShiftLeft:
    return "LShift";
  case key_code::ShiftRight:
    return "RShift";
  case key_code::CtrlLeft:
    return "LCtrl";
  case key_code::CtrlRight:
    return "RCtrl";
  case key_code::AltLeft:
    return "LAlt";
  case key_code::AltRight:
    return "RAlt";
  case key_code::CmdLeft:
    return "LCmd";
  case key_code::CmdRight:
    return "RCmd";
  case key_code::CapsLock:
    return "CapsLock";
  case key_code::Fn:
    return "Fn";
  default:
    return "?";
  }
}

// Your callback function - receives all key events
void handleKeyEvent(KeyEvent event, void* /*userData*/) {
  const char* typeStr = "?";
  switch (event.type) {
  case KeyEventType::KeyDown:
    typeStr = "DOWN   ";
    break;
  case KeyEventType::KeyUp:
    typeStr = "UP     ";
    break;
  case KeyEventType::KeyRepeat:
    typeStr = "REPEAT ";
    break;
  case KeyEventType::ModifierChanged:
    typeStr = "MODCHG ";
    break;
  }

  if (event.type == KeyEventType::ModifierChanged) {
    printf("%s keyCode=%3d (%s) shift=%d ctrl=%d alt=%d cmd=%d caps=%d fn=%d\n",
           typeStr,
           event.keyCode,
           getModifierName(event.keyCode),
           event.shift,
           event.ctrl,
           event.alt,
           event.cmd,
           event.capsLock,
           event.fn);
  } else {
    printf("%s keyCode=%3d char='%c' shift=%d ctrl=%d alt=%d cmd=%d\n",
           typeStr,
           event.keyCode,
           event.character ? event.character : ' ',
           event.shift,
           event.ctrl,
           event.alt,
           event.cmd);
  }

  // Example: ESC to quit
  if (event.type == KeyEventType::KeyDown && event.keyCode == 53) {
    printf("ESC pressed, stopping...\n");
    stopKeyCaptureLoop();
  }
}

int main() {
  printf("KeyCapture Example\n");
  printf("------------------\n");
  printf("Press keys to see events. ESC to quit.\n\n");

  // 1. Initialize Cocoa app
  initKeyCaptureApp();

  // 2. Create a minimal window (required for local capture without permissions)
  WindowConfig config = defaultWindowConfig();
  config.title = "Key Capture Demo";
  config.width = 300;
  config.height = 80;

  if (!createCaptureWindow(config)) {
    printf("Failed to create window\n");
    return 1;
  }

  // 3. Start capturing with local mode (no permissions needed when window
  // focused)
  //    Change to CaptureMode::Global if you need capture when not focused
  //    Change to CaptureMode::Both if you want both behaviors
  if (!startKeyCapture(handleKeyEvent, nullptr, CaptureMode::Local)) {
    printf("Failed to start key capture\n");
    return 1;
  }

  // Update window text
  setWindowText("Press keys... (ESC to quit)");

  // 4. Run the event loop (blocks until stopKeyCaptureLoop() called)
  runKeyCaptureLoop();

  // 5. Cleanup
  stopKeyCapture();

  printf("Done.\n");
  return 0;
}

} // namespace device_io
