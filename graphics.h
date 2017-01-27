#if !defined (GRAPHICS_H)
#define GRAPHICS_H
#include <windows.h>
#include <cstdio>

#define DARK_BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define DARK_PURPLE 5
#define BROWN 6
#define GRAY 7
#define DARK_GRAY 8
#define BLUE 9
#define NEON_GREEN 10
#define LIGHT_BLUE 11
#define LIGHT_RED 12
#define PURPLE 13
#define YELLOW 14
#define WHITE 15


BOOL settextcolor( WORD color );
BOOL setbkcolor( WORD color );
BOOL gotoxy( short x, short y );
void GetHandle( HANDLE &hStdout );

#endif