#include "strings.h"

// trim the left character of a given string
void trimLeft(std::string &str, char c) {
	int pos = 0;
	for(; str[pos] == c; ++pos)
		;
	str.erase( 0, pos );
}

// trim the right character of a given string
void trimRight(std::string &str, char c) {
	int pos = str.length();
	if( pos == 0 ) return;
	for(--pos; str[pos] == c; --pos)
		;
	str.erase(++pos, str.length() - pos);
}

// trim all left and right characters that is specified by
// the string "characters"
void trimLR(std::string &str, std::string characters) {
	int len = characters.length();
	for(int i = 0; i < len; ++i) {
		trimLeft( str, characters[i] );
		trimRight( str, characters[i] );
	}
}

// removes a substring from a given string
bool remove(std::string &str, std::string substr) {
	int pos = str.find(substr);
	if(pos != std::string::npos) {
		str.erase( pos, substr.length());
	}
	return (pos != std::string::npos);
}

// replace a substring by another substring
bool replace(std::string &str, std::string oldsubstr, std::string newsubstr) {
	int pos = str.find(oldsubstr);
	if( pos != std::string::npos ) {
		remove( str, oldsubstr );
		str.insert( pos, newsubstr );
	}
	return (pos != std::string::npos);
}

void UpperCase( std::string &str ) {
	int len = str.length();
	for( int i = 0; i < len; i++ ) {
		if ( str[i] >= 'a' && str[i] <= 'z' ) {
			str[i] -= 'a' - 'A';
		}
	}
}

void LowerCase( std::string &str ) {
	int len = str.length();
	for( int i = 0; i < len; i++ ) {
		if ( str[i] >= 'A' && str[i] <= 'Z' ) {
			str[i] += 'a' - 'A';
		}
	}
}

int find(vstring v, std::string str) {
	int iLim = v.size();
	for(int i = 0; i < iLim; ++i) {
		if(v[i] == str) {
			return i;
		}
	}
	return -1;
}

int rfind(vstring v, std::string str) {
	int start = v.size() - 1;
	for(int i = start; i >= 0; --i) {
		if(v[i] == str) {
			return start - i;
		} 
	}
	return -1;
}
