/* KeyCapture.h
 * Cocoa-based keyboard input capture with C/C++ interface
 *
 * Usage:
 *   1. Call initKeyCaptureApp() once at startup (required for Cocoa)
 *   2. Call startKeyCapture() with your callback
 *   3. Call runKeyCaptureLoop() to start the event loop (blocks)
 *   - OR use your own run loop and just call startKeyCapture()
 *   4. Call stopKeyCapture() when done
 */

#pragma once
#include <cstdint>

namespace device_io {

// Key event types
enum class KeyEventType {
  KeyDown,        // Key initially pressed
  KeyUp,          // Key released
  KeyRepeat,      // Key held (repeated)
  ModifierChanged // Modifier key state changed (shift/ctrl/alt/cmd/capslock/fn)
};

// Key event data passed to callback
struct KeyEvent {
  KeyEventType type;
  uint16_t keyCode; // Hardware scan code (e.g., 0 = 'A', 1 = 'S', etc.)
  char character;   // ASCII character if printable, 0 otherwise
  bool shift;
  bool ctrl;
  bool alt;      // Option key on macOS
  bool cmd;      // Command key on macOS
  bool capsLock; // Caps Lock state
  bool fn;       // Fn key state
};

// Common modifier key codes (for interpreting ModifierChanged events)
namespace key_code {
enum : uint16_t {
  CapsLock = 57,
  ShiftLeft = 56,
  ShiftRight = 60,
  CtrlLeft = 59,
  CtrlRight = 62,
  AltLeft = 58,
  AltRight = 61,
  CmdLeft = 55,
  CmdRight = 54,
  Fn = 63,
};
} // namespace key_code

/* Callback function type
 * event: the key event data
 * userData: pointer passed to startKeyCapture
 */
using KeyCallback = void (*)(KeyEvent event, void* userData);

// Capture mode configuration
enum class CaptureMode {
  Local,  // Only when app window is focused (no permissions needed)
  Global, // Even when app not focused (needs Accessibility permissions)
  Both    // Local + Global
};

// Window configuration (for local capture mode)
struct WindowConfig {
  const char* title; // Window title
  int width;         // Window width
  int height;        // Window height
  bool showOnStart;  // Show window immediately
};

// Default window config
inline WindowConfig defaultWindowConfig() {
  return WindowConfig{"Synth", 300, 100, true};
}

// Initialize Cocoa application (call once at startup)
// Required before any other KeyCapture functions
void initKeyCaptureApp();

/* Create a minimal window for local key capture
 * Returns true on success
 * Only needed if using CaptureMode::Local or CaptureMode::Both
 */
bool createCaptureWindow(WindowConfig config);

// Update the window's display text
void setWindowText(const char* text);

/* Start capturing keyboard events
 * callback: function to call for each key event
 * userData: passed to callback (can be nullptr)
 * mode: which capture mode to use
 * Returns true on success
 */
bool startKeyCapture(KeyCallback callback, void* userData, CaptureMode mode);

// Stop capturing keyboard events
void stopKeyCapture();

// Run the Cocoa event loop (blocks until app terminates)
// Call this from main() after setup
void runKeyCaptureLoop();

// Request the app to terminate and immediately exit (preferred)
void terminateKeyCaptureLoop();

// Request the app to stop (breaks out of runKeyCaptureLoop)
void stopKeyCaptureLoop();

} // namespace device_io
