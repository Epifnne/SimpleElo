#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "client/app/guiApp.h"

int main() {

#if defined(_WIN32)
  ::FreeConsole();
#endif

  return simpleelo::client::app::runGuiApp();
}
