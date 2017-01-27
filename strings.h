#ifndef _STRINGS_H_
#define _STRINGS_H_

#include "tokenizer.h"
#include <iostream>

void trimLR(std::string &str, std::string characters);
void trimLeft(std::string &str, char c);
void trimRight(std::string &str, char c);

bool replace(std::string &str, std::string oldsubstr, std::string newsubstr);
bool remove(std::string &str, std::string substr);

void UpperCase( std::string &str );
void LowerCase( std::string &str );

int find(vstring v, std::string str);
int rfind(vstring v, std::string str);

template<typename T>
void shuffle(T &array, size_t size) {
	for(int i = 0; i < size; ++i) {
		int index = rand() % size;
		std::swap(array[i], array[index]);
	}
}


#endif