#pragma once

// Retained source-compatible names for the retired console-colour helper.
// Terminal presentation is deliberately not part of the engine API.
constexpr unsigned short DARK_BLUE = 1;
constexpr unsigned short GREEN = 2;
constexpr unsigned short CYAN = 3;
constexpr unsigned short RED = 4;
constexpr unsigned short DARK_PURPLE = 5;
constexpr unsigned short BROWN = 6;
constexpr unsigned short GRAY = 7;
constexpr unsigned short DARK_GRAY = 8;
constexpr unsigned short BLUE = 9;
constexpr unsigned short NEON_GREEN = 10;
constexpr unsigned short LIGHT_BLUE = 11;
constexpr unsigned short LIGHT_RED = 12;
constexpr unsigned short PURPLE = 13;
constexpr unsigned short YELLOW = 14;
constexpr unsigned short WHITE = 15;

bool settextcolor(unsigned short color);
bool setbkcolor(unsigned short color);
bool gotoxy(short x, short y);
