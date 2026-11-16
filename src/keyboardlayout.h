// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

/// The three letter rows of a US QWERTY keyboard, left to right. The middle
/// row ends with ';', which some schemes use.
namespace KeyboardLayout {

inline constexpr const char *rows[] = {"qwertyuiop", "asdfghjkl;", "zxcvbnm"};
inline constexpr int rowCount = 3;

} // namespace KeyboardLayout
