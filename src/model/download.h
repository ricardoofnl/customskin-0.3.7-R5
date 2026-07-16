// minimal http downloader (WinINet). sends user-agent "samp/0.3", which open.mp's
// artwork webserver requires
//
// note: Fetch() is blocking. it is currently called from the rpc handler (game main
// thread); fine for small files over localhost, but making it async is a phase 5 item
#pragma once

#include <string>

namespace dl {

// get `url` into `destPath`. returns true if any bytes were written
bool Fetch(const std::string& url, const std::string& destPath);

} // namespace dl
