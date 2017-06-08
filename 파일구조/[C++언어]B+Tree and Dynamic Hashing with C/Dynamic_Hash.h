#include <iostream>
#include <fstream>
#include <string>

#define BLOCKSIZE 4096
#pragma pack(1)
using namespace std;


class HashTable{
public:
	HashTable();
	long Table_Block_Offset[BLOCKSIZE/sizeof(long)];		         // 1024
};

class Dynamic_Hash{
private:
	unsigned int HASH(string str);                               // ID ���� ������ Hash_Table�� Index�� �����ϴ� Key�� Return.
	void Extend_Table(unsigned int Hash_Key, fstream& DB_File);		  
	
	HashTable Hash_Table;                                            // Hash Table.
	fstream Hash_File;                                               // Hash Table�� ����Ǵ� File.
	int Table_Bit_Num;                                               // Table Bit.

public:
	Dynamic_Hash(char* Hash_File_name);
	~Dynamic_Hash();
	long Get_Hash_Offset(string s_ID);		                         // ID ���� ������ �ͼ� �ش� Block Offset�� Return.
	void Block_Full(string s_ID, int Block_Bit_Num, fstream& DB_File);	               
};
