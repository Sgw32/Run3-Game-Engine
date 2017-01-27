#ifndef _RECORDER_H_
#define _RECORDER_H_

//#include "graphics.h"
#include <string>
#include <ctime>
#include <vector>
//#include <conio.h>


double countTime(void);
void sleep(double goal);
void save_delay(void);

void read_time_delay(std::vector<double> &time_delay);
void print_string(const char *string, std::vector<double> time_delay, size_t linePos);

#endif