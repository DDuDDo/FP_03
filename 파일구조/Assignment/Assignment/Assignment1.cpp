#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <cmath>

#define BLOCKSIZE  4096
#define BLOCKCOUNT 12000                                             
#define IN_BLOCK_MAX  BLOCKSIZE/sizeof(Student)                      
#pragma pack(1)

using namespace std;

fstream DB_File;

// =========================You don't need to touch=========================
typedef struct {
	unsigned blockNumber;
	unsigned studentId;
} SaveInfo;

vector<SaveInfo> infoVector;

void insertBlockNumber(unsigned blockNumber, unsigned studentId){

	SaveInfo s;
	s.blockNumber = blockNumber;
	s.studentId = studentId;
	infoVector.push_back(s);
}

bool checkBlockNumber(unsigned blockNumber, unsigned studentId){
	bool isThere = false;
	for(int i = 0; i < infoVector.size(); i++){
		if(infoVector[i].studentId == studentId) {
			if(infoVector[i].blockNumber == blockNumber){
				isThere = true;
			}              
		}                
	}     

	return isThere;
}
// ======================You don't need to touch (End)======================

// =========================Your code here down!!=========================

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
	Student Record[BLOCKSIZE/sizeof(Student)];                       
	Block();
	int  Record_Count;                                               
	int  Bit_Num;																	
	char Block_Garbage[3];                                           
};


Student::Student()
{
	ID     = 0;
	Score  = 0;
	memset(Name, '\0', sizeof(Name));
	memset(Dept, '\0', sizeof(Dept));
}
// Constructor
Block::Block()
{
	Record_Count      = 0;
	Bit_Num           = 0;  
}

Block DB_block;
class HashTable{
public:
	HashTable();
	long Table_Block_Offset[BLOCKSIZE/sizeof(long)];		         
};

class Dynamic_Hash{

private:
	unsigned int HASH(string str);                               
	void Extend_Table(unsigned int Hash_Key, fstream& DB_File);		  

	HashTable Hash_Table;                                            
	fstream Hash_File;                                               
	int Table_Bit_Num;                                               

public:
	Dynamic_Hash(char* Hash_File_name);
	~Dynamic_Hash();
	int Get_Hash_Offset(string s_ID);		                         
	void Block_Full(string s_ID, int Block_Bit_Num, fstream& DB_File);	               
};


HashTable::HashTable()
{
	for(int i=0; i<(BLOCKSIZE/sizeof(long)); i++)                   
		Table_Block_Offset[i] = -1;
}

Dynamic_Hash::Dynamic_Hash(char* Hash_File_Name)
{
	Hash_File.open(Hash_File_Name, ios::binary | ios::in | ios::out);
	if(!Hash_File)                                                   
	{
		Hash_File.clear();
		Hash_File.open(Hash_File_Name, ios::binary | ios::in | ios::out | ios::trunc);
		Table_Bit_Num = 0;
		Hash_File.write((char*)&Table_Bit_Num, sizeof(Table_Bit_Num)); 
		Hash_Table.Table_Block_Offset[0] = 0;                        
		Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));
	}
	else
	{
		Hash_File.read((char*)&Table_Bit_Num, sizeof(Table_Bit_Num));
		Hash_File.read((char*)&Hash_Table, sizeof(Hash_Table));      
	}
}


Dynamic_Hash::~Dynamic_Hash()
{
	Hash_File.seekp(0, ios::beg);
	Hash_File.write((char*)&Table_Bit_Num, sizeof(Table_Bit_Num));
	Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));
	Hash_File.close();
}

unsigned int Dynamic_Hash::HASH(string str)
{
	unsigned int seed = 131; 
	unsigned int key = 0;


	for(unsigned int i = 0; i < str.length(); i++)
	{
		key = (key * seed) + str[i];
	}

	key = key%1024;

	unsigned start=0;
	unsigned end=32;
	unsigned hash=key;
	char cf=0;

   while(1){
      
	  cf = hash>>31;
	  hash = hash<<1;
      
	  if(cf==1){
         start = start>>1;
         start += unsigned(1<<31);
         end--;
         if(end == 0)
			 break;	 
      }
      else{
         start = unsigned(start>>1);
         end--;
         if(end == 0)
			 break;
	  }
   }
	key = start;
	return key;
}

int Dynamic_Hash::Get_Hash_Offset(string s_ID)
{
	unsigned int Hash_Key = HASH(s_ID);              


	if(Table_Bit_Num == 0)
	{
		Hash_Key = 0;
	}
	else
	{
		Hash_Key = Hash_Key >> (32-Table_Bit_Num);  
	}	
	return Hash_Table.Table_Block_Offset[Hash_Key]; 
}

// 
void Dynamic_Hash::Block_Full(string s_ID, int Block_Bit_Num, fstream& DB_File)
{
	unsigned int Hash_Key = HASH(s_ID);
	if(Table_Bit_Num != 0)                                           
		Hash_Key = Hash_Key >> (32-Table_Bit_Num);  
	else
		Hash_Key = 0;

	if(Block_Bit_Num == Table_Bit_Num)                              
	{
		Extend_Table(Hash_Key, DB_File);
		return;
	}
	else if(Block_Bit_Num < Table_Bit_Num)                                        
	{
		Block* Buffer = new Block;                                 
		DB_File.seekp(0, ios::end);
		long New_Block_Offset = DB_File.tellp();                  
		Block* New_Block = new Block;                             
		Block* Old_Block = new Block;                              

		DB_File.seekg(Hash_Table.Table_Block_Offset[(int)Hash_Key], ios::beg);
		DB_File.read((char*)Buffer, sizeof(Block));
		Old_Block->Bit_Num = Buffer->Bit_Num + 1;
		New_Block->Bit_Num = Buffer->Bit_Num + 1;

		int How_Many = Table_Bit_Num - Block_Bit_Num;
		int start = -1, end = -1;
		int i;
		for(i=0; i<1024; i++)
		{
			if(start == -1 && Hash_Table.Table_Block_Offset[i] == Hash_Table.Table_Block_Offset[(int)Hash_Key])
				start = i;
			if(start != -1 && Hash_Table.Table_Block_Offset[i] != Hash_Table.Table_Block_Offset[(int)Hash_Key])
			{
				end = i-1;
				break;
			}
		}


		for(i=start; i<=(start+(end-start)/2); i++)
			Hash_Table.Table_Block_Offset[i] = Hash_Table.Table_Block_Offset[Hash_Key];

		for(i=(start+(end-start)/2)+1; i<=end; i++)
			Hash_Table.Table_Block_Offset[i] = New_Block_Offset;



		int Low_Count  = 0; 
		int High_Count = 0;
		unsigned int m_Hash_Key;
		char m_c_ID[30];
		string m_s_ID;

		for(int j=0; j<95; j++)
		{
			sprintf(m_c_ID, "%d", Buffer->Record[j].ID);
			m_s_ID = m_c_ID;
			m_Hash_Key = HASH(m_s_ID);
			m_Hash_Key = m_Hash_Key >> (32-Table_Bit_Num);  
			if((int)m_Hash_Key < (start+(end-start)/2)+1)
				Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
				
			else
			{
				New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];

			}
		}

		DB_File.seekp(Hash_Table.Table_Block_Offset[start], ios::beg);
		DB_File.write((char*)Old_Block, sizeof(Block));
		DB_File.seekp(New_Block_Offset, ios::beg);
		DB_File.write((char*)New_Block, sizeof(Block));

		Hash_File.seekp(0, ios::beg);
		Hash_File.write((char*)&Table_Bit_Num, sizeof(Table_Bit_Num)); 
		Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));

		delete Buffer;
		delete New_Block;
		delete Old_Block;
	}

}

void Dynamic_Hash::Extend_Table(unsigned int Hash_Key, fstream& DB_File)
{	
	Block* Buffer = new Block;                                     

	DB_File.seekp(0, ios::end);
	long New_Block_Offset = DB_File.tellp();                          
	Block* New_Block = new Block;                                   
	Block* Old_Block = new Block;	
	unsigned int Table_Block_Num = 2;                                                           
	if(Table_Bit_Num != 0)
		Table_Block_Num = Table_Block_Num << (Table_Bit_Num-1);
	else
		Table_Block_Num = 1;
	for(int i=Table_Block_Num-1; i>=0; i--)                         
	{
		if(i == (int)Hash_Key)                                  
		{
			Hash_Table.Table_Block_Offset[2*i]   = Hash_Table.Table_Block_Offset[i];
			Hash_Table.Table_Block_Offset[2*i+1] = New_Block_Offset;


			DB_File.seekg(Hash_Table.Table_Block_Offset[i], ios::beg);
			DB_File.read((char*)Buffer, sizeof(Block));

			Table_Bit_Num++;
			Old_Block->Bit_Num = Table_Bit_Num;
			New_Block->Bit_Num = Table_Bit_Num;

			int Low_Count  = 0; 
			int High_Count = 0;
			unsigned int m_Hash_Key;
			char c_ID[30];
			string s_ID;

			for(unsigned int j=0; j<95; j++)
			{
				sprintf(c_ID, "%d", Buffer->Record[j].ID);
				s_ID = c_ID;
				m_Hash_Key = HASH(s_ID);
				m_Hash_Key = m_Hash_Key >> (32-Table_Bit_Num);  
				if(m_Hash_Key == 2*i)
					Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
				else
				{
					New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];

				}
			}

			DB_File.seekp(Hash_Table.Table_Block_Offset[2*i], ios::beg);
			DB_File.write((char*)Old_Block, sizeof(Block));
			DB_File.seekp(New_Block_Offset, ios::beg);
			DB_File.write((char*)New_Block, sizeof(Block));
		}
		else                                                         
		{
			Hash_Table.Table_Block_Offset[2*i]   = Hash_Table.Table_Block_Offset[i];
			Hash_Table.Table_Block_Offset[2*i+1] = Hash_Table.Table_Block_Offset[i];
		}
	}

	Hash_File.seekp(0, ios::beg);
	Hash_File.write((char*)&Table_Bit_Num, sizeof(Table_Bit_Num)); 
	Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));

	delete Buffer;
	delete New_Block;
	delete Old_Block;
}
Dynamic_Hash* H; 
bool openDB(char* filename){

	char Filename[80] = "";
	fstream IDX_FILE;
	sprintf(Filename, "%s.dat", filename);
	DB_File.open(Filename, ios::in | ios::out | ios::binary);               
	
	if(!DB_File)
	{
		DB_File.clear();
		DB_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
		DB_File.seekp(0, ios::beg);                                        
		DB_File.write((char*)&DB_block, sizeof(Block));
	}

	strcpy(Filename,""); 
	sprintf(Filename, "%s.hash", filename);
	H = new Dynamic_Hash(Filename);

	strcpy(Filename,""); 
	sprintf(Filename, "%s.idx", filename);
	IDX_FILE.open(Filename, ios::binary | ios::in | ios::out | ios::trunc);

	return false;
}

unsigned insertRecord(char* name, unsigned ID, float score, char* dept)
{
	char* temp = new char[10];
	char* temp1 = new char[10];
	string id;

	unsigned blockNumber = 0;

	sprintf(temp, "%d",ID);
	id=temp;

	int DB_File_Offset = H->Get_Hash_Offset(id);               

	DB_File.seekg(DB_File_Offset, ios::beg);                                
	DB_File.read((char*)&DB_block, sizeof(Block));

	if(DB_block.Record_Count < IN_BLOCK_MAX)                        
	{		
		strcpy(DB_block.Record[DB_block.Record_Count].Name, name);			
		DB_block.Record[DB_block.Record_Count].ID = ID;
		DB_block.Record[DB_block.Record_Count].Score = score;
		strcpy(DB_block.Record[DB_block.Record_Count].Dept,dept);

		DB_block.Record_Count++;

		DB_File.seekp(DB_File_Offset, ios::beg);      
		DB_File.write((char*)&DB_block, sizeof(Block));		
	}
	else                                                             
	{															
		H->Block_Full(id, DB_block.Bit_Num, DB_File);
		for(int i=0; i<DB_block.Record_Count; i++)
		{
			unsigned studentid = DB_block.Record[i].ID;
			sprintf(temp1, "%d",studentid);
			string student_id=temp1;
			DB_File_Offset = H->Get_Hash_Offset(student_id);
			blockNumber=DB_File_Offset;
			insertBlockNumber(blockNumber, studentid);
		}
		insertRecord(name,ID,score,dept);
	}
	blockNumber=DB_File_Offset;
	return blockNumber;
}
unsigned searchID(unsigned ID){

	char* temp = new char[10];
	string id;

	unsigned blockNumber = 0;

	sprintf(temp, "%d",ID);
	id=temp;

	int DB_File_Offset = H->Get_Hash_Offset(id);     
	
	DB_File.seekg(DB_File_Offset, ios::beg);                                
	DB_File.read((char*)&DB_block, sizeof(Block));
	
	blockNumber=DB_File_Offset;		
	
	return blockNumber;
}


// =========================Your code here down!!(End)=========================




// =========================You don't need to touch=========================         
// main
int main()
{

	openDB("myDB");

	// Input data
	ifstream fin("Assignment1.inp", ios::in);

	// for ESPA
	ofstream fout("Assignment1.out", ios::out);

	while(!fin.eof()){

		string type = "";
		fin >> type;
		if(type == "i"){ // insertRecord

			string firstName = "";
			string lastName = "";
			string fullName = "";
			unsigned studentId = 0;
			float score = 0;
			string deptStr = "";

			fin >> firstName >> lastName >> studentId >> score >> deptStr;
			fullName = firstName + " " + lastName;
			char name[30];
			strcpy(name, fullName.c_str());
			// for Visual Studio 2012
			// strcpy_s(name, fullName.c_str());
			char dept[30];
			strcpy(dept, deptStr.c_str());
			// for Visual Studio 2012
			// strcpy_s(dept, deptStr.c_str());

			//cout << type << "\t" << name << "\t"  << studentId << "\t"  << score << "\t"  << dept << endl;

			// insertRecord
			unsigned blockNumber = insertRecord(name, studentId, score, dept);
			// for ESPA
			insertBlockNumber(blockNumber, studentId);


		} else if(type == "s"){  // searchID
			unsigned studentId = 0;

			fin >> studentId;
			//cout << type << "\t"  << studentId << endl;

			// searchID
			unsigned blockNumber = searchID(studentId);
			// for ESPA
			bool isThere = false;
			isThere = checkBlockNumber(blockNumber, studentId);
			if(isThere)
				fout << "true" << endl;
			else
				fout << "false" << endl;

		}
	}

	fout.close();
	fin.close();
	//DB_File.close();
	// for Dev C++
	//system("PAUSE");
	//return EXIT_SUCCESS;
	return 0;
}

// ======================You don't need to touch (End)======================
