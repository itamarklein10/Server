#define _CRT_SECURE_NO_WARNINGS
#ifndef HTTP_PARSER
#define HTTP_PARSER

#include <stdio.h>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fstream>
using namespace std;

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int GET = 5;
const int PUT = 6;
const int OPTIONS = 7;
const int TRACE = 8;
const int HEAD = 9;
const int _DELETE = 10;
const int EXIT = 11;
const int ERROR_REQUEST = 12;

void parseHttpRequest(char* recvBuff, int* buffSize, string& req ,string& body);
string getLine(string& buff);
pair<string, string> parseHttpLine(string line);
string GetAnswer(string request);
int parseHttpMethod(string request);
string PutAnswer(string request, string body);
string DeleteAnswer(string request);
string OptionsAnswer();
string TraceAnswer(string request, string body);
string HeadAnswer(string request);
string GetFileContent(string request);
string writeToFile(string body, string fileName);
string deleteFile(string fileName);



#endif
