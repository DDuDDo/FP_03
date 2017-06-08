#include <iostream>
#include <fstream>
#include <string>

#define BLOCKSIZE  4096
#define BLOCKCOUNT 12000                                             
#define IN_BLOCK_MAX  BLOCKSIZE/sizeof(Student)                      // 95
#pragma pack(1)
using namespace std;


class Student{
public :
	char Name[20];
	unsigned ID;
	float Score;
	char Dept[10];
	Student();
};

class Block{
public :
	Student Record[BLOCKSIZE/sizeof(Student)];                       // 95개의 Record가 하나의 Block안에 들어간다.
	Block();
	int  Record_Count;                                               // 하나의 Block에서 저장된 Record의 갯수.
	int  Bit_Num;																	
	char Block_Garbage[3];                                           // 4096Byte를 맞추기 위하여 남는 3Byte를 처리.
};
