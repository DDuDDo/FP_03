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
	Student Record[BLOCKSIZE/sizeof(Student)];                       // 95���� Record�� �ϳ��� Block�ȿ� ����.
	Block();
	int  Record_Count;                                               // �ϳ��� Block���� ����� Record�� ����.
	int  Bit_Num;																	
	char Block_Garbage[3];                                           // 4096Byte�� ���߱� ���Ͽ� ���� 3Byte�� ó��.
};
