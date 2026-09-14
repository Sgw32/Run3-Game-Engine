#include "graphics.h"

#include <run3/core/Log.hpp>

namespace {

bool retiredConsoleOperation() {
  static bool reported = false;
  if (!reported) {
    run3::logWarning(
        "Legacy console colour/cursor control is retired; use structured logging");
    reported = true;
  }
  return false;
}

} // namespace

bool settextcolor(unsigned short) { return retiredConsoleOperation(); }
bool setbkcolor(unsigned short) { return retiredConsoleOperation(); }
bool gotoxy(short, short) { return retiredConsoleOperation(); }
