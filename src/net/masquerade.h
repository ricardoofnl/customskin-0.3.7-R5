// dl masquerade
#pragma once

namespace masq {

// apply the version + challenge-response byte patches. idempotent, R5-only, must run before connecting
bool Apply();

// true once Apply() has succeeded
bool Enabled();

} // namespace masq
