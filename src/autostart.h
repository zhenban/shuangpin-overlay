// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

/// Starting with the desktop session, through an XDG autostart entry.
namespace Autostart {

bool isEnabled();
void setEnabled(bool enabled);

} // namespace Autostart
