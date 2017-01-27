#include "graphics.h"

void GetHandle( HANDLE &hStdout )
{
	/*DWORD dw;
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if(hStdout == INVALID_HANDLE_VALUE)
	{
		dw = GetLastError();
		fprintf( stderr, "Invalid Handle : GetLastError has return %d\n", dw );
	}*/
}

BOOL settextcolor( WORD color )
{
	/*HANDLE hStdout;
	GetHandle( hStdout );
	BOOL rt = SetConsoleTextAttribute( hStdout, color );
	return rt;*/
	return 1;
}

BOOL setbkcolor( WORD color )
{
	/*HANDLE hStdout;
	GetHandle( hStdout );
	DWORD nLength, nAttWriten;
	COORD coord;
	coord.X = 0;
	coord.Y = 0;
	nLength = 80 * 50; //80*50
	BOOL rt = FillConsoleOutputAttribute( hStdout, color, nLength, coord, &nAttWriten ); 
	return rt;*/
	return 1;
}

BOOL gotoxy( short x, short y )
{
	/*HANDLE hStdout;
	COORD coord;
	BOOL rt;
	coord.X = x;
	coord.Y = y;
	GetHandle( hStdout );
	rt = SetConsoleCursorPosition( hStdout, coord );
	return rt;*/
	return 1;
}
