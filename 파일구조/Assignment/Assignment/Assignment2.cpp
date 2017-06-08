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
	char Name[20];//20
	unsigned ID;//4
	float Score;//4
	char Dept[10];//10
	Student();
};

class Block{
public :
	Student Record[BLOCKSIZE/sizeof(Student)];                       
	Block();
	int  Record_Count;                                               
	int  Bit_Num;																	
	char Block_Garbage[22];                                           
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


class Element{
public : 
	long  Offset;													 // External Node�� ��� DB File���� Offset.(ID�� ����Ѵ�.) 
	// Internal Node, Root Node�� ���� ���� Node�� ����Ų��.
	// �� �� Point�Ǵ� Node�� �� Element�� Key���� ���� ������ ���� Node.  
	float  Key;                                                       // Key(Value).
	Element();
};

// Constructor : Offset�� Key�� -1�� ��Ÿ���� Invalid�ϴ�.

Element::Element()
{
	Offset = -1;
	Key    = -1;
}

// ************************************************************************************************
// **************************** Node Class : B+Tree������ �ϳ��� Node. ****************************
// ************************************************************************************************ 


class Node{
public :
	Element Node_Element[BLOCKSIZE/sizeof(Element)-2];   // �ϳ��� Node�� ���� Elements.(510)
	long    Last_Offset;                                             // �ϳ��� Node���� Last Fan-Out.
	long    Next_Node_Offset;                                        // External Node�� ���. ���� Node�� Offset. -1�� ��� Invalid.
	int     Element_Count;                                           // Node���� ä���� Element ����.
	int     Node_Mode;                                               // 1: Root Node 2: Internal Node 3: External Node. 4 : Root Node & External Node.
	Node();                                                                                                     
};

Node::Node()                                                         
{	
	Last_Offset = -1;
	Next_Node_Offset = -1;
	Element_Count = 0;
	Node_Mode = 0;	             
}

// ************************************************************************************************
// ********************************** B_Plus_Tree Class : B+Tree **********************************
// ************************************************************************************************ 

class B_Plus_Tree{
private :
	int      Node_Count_In_File;                                     // Tree�� Node�� ����.
	long     Root_Node_Offset;                                       // Root Node Offset.
	Node* Current_Node;                                        // ���� Node.
	long     Current_Node_Offset;                                    // ���� Node�� ����� File�� Offset.
	bool     Dirty;                                                  // ���� Node�� Update ����.
	int      Element_In_Node;                                        // �� Node ���� Element ��.
	long     Small_Pointer;                                          // Split�� �� ����   Node.
	long     Big_Pointer;                                            // Split�� �� ������ Node.
	fstream  Tree_File;                                              // Tree�� ����� File.
	void Init(char* Filename);                                       // �����ڿ��� Call�ϴ� Initialization Procedure.
	void GetNode(long m_Offset);				                     // Param�� Offset�� ��ġ�� Node�� Current_Node�� Set.
	int  FindInNode(Node* m_Current_Node, float m_Find_Key, bool& m_Is_Find);       // m_Current_Node���� Key�� ã�´�.
	void Insert_Element(float m_Inserted_Key, long m_Inserted_Offset, Node* m_Current_Node, int m_Insert_Index);
	void JustInsert(float m_Inserted_Key,  long m_Inserted_Offset, Node* m_Current_Node, int m_Insert_Index);
	void SplitInsert(float m_Inserted_Key, long m_Inserted_Offset, Node* m_Current_Node, int m_Insert_Index);
	int  Get_Parent_Node(float m_Inserted_Key, long m_Current_Node_Offset);               // Parent Node ��´�.

public :
	B_Plus_Tree(char* Filename);
	~B_Plus_Tree();
	long Find(float m_Find_Key, int& m_Index, bool& m_Is_Find);       // Key ���� ������ Tree���� �����ϴ��� ���θ� Check.															 
	bool Insert(float m_Inserted_Key, long m_Inserted_Offset);        // �ϳ��� Element�� Insert.
	bool Change_Block(float m_Change_Key, long Old_Offset, long New_Offset);    // Hash Function�� ���� Split�� ��, Key�� ����� Block Offset�� ����.
	void Rsearch(float m_Small_Key, float m_Big_Key, int* Result);
};

B_Plus_Tree::B_Plus_Tree(char* Filename)
{
	Current_Node = new Node<Type>;                                   // Current_Node Allocation.
	Current_Node_Offset = -1;
	Dirty = false;
	Element_In_Node = BLOCKSIZE/sizeof(Element)-2;             // ���⼭�� 510�̴�.
	Small_Pointer = -1;
	Big_Pointer   = -1;
	Init(Filename);
}

B_Plus_Tree::~B_Plus_Tree()
{
	if(Dirty && Current_Node != NULL)
	{
		Tree_File.seekp(Current_Node_Offset, ios::beg);
		Tree_File.write((char*)Current_Node, sizeof(Node));
	}

	if(Current_Node != NULL)
		delete Current_Node;

	Tree_File.seekp(0, ios::beg);
	Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
	Tree_File.close();
}


void B_Plus_Tree::Init(char* Filename)
{
	// B+ Tree File�� Open�Ѵ�. �����ϴ� ��� File�κ��� Root_Node_Offset�� ��´�. 
	// File�� �������� ������ Root Node�� Create�Ѵ�.
	Tree_File.open(Filename, ios::in | ios::out | ios::binary);
	if(!Tree_File)                                                   // File Open�� ������ ���.
	{
		Tree_File.clear();                                           // Open�� ������ Error Bit�� ��� Clear.
		Tree_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);

		// Root Node�� �����Ͽ� Tree_File�� Offset 4�� �����Ѵ�. Offset[0..4]�� Root Node�� ����Ű�� Pointer�̴�. 
		Node* Root = new Node;                                      
		Root->Node_Mode = 4;                                         // Root & External.
		Tree_File.seekp(sizeof(long), ios::beg);
		Root_Node_Offset = Tree_File.tellp();
		Tree_File.write((char*)Root, sizeof(Node));            // Root Node ����.
		Tree_File.seekp(0, ios::beg);
		Tree_File.write((char*)&Root_Node_Offset, sizeof(long));     // Root Node Offset ����.
		delete Root;
	}
	else                                                             // Tree_File�� �̹� �����ϴ� ���.
	{
		Tree_File.seekg(0, ios::beg);        
		Tree_File.read((char*)&Root_Node_Offset, sizeof(long));      // Root_Node_Offset Get.     
	}

	Tree_File.seekg(0, ios::end);        
	Node_Count_In_File = (Tree_File.tellg()-(long)4)/(sizeof(Node<float>));
}

// Find Function���� �־��� Key���� ������ B_Plus_Tree���� ã�´�.
// �̶� Range Search�� �����ϰ� �ϱ����Ͽ� ������ ���� �����ϴ� ��� �� ó���� Key�� ���� DB_FIle Offset�� ��ȯ�Ѵ�.
// �׷��Ƿ� �ڿ� ���������� �����ϴ� ������ Key Element���� ��� ���� �� �� �ִ�.
// 1st Param : Find�ϰԵ� Key. 2nd Param : ã�� Key���� Index or Insert�� Index ����. 3th Param : �����ϴ� ��� True, �ƴϸ� False.  
// Current_Node : Key�� �����ϴ� ��� �� Key�� �����ϴ� Node��, �ƴ� ��� �� key���� Insert�Ǿ��ϴ� Node�� �����ȴ�.
// Return Value : Key�� �����ϴ� ��� �� Key�� DB_file �� Block Offset�� Return, �ƴ� ��� -1 Return.  

long B_Plus_Tree::Find(float m_Find_Key, int& m_Index, bool& m_Is_Find)
{
	m_Is_Find = false;
	m_Index = 0;
	long m_Node_Offset = Root_Node_Offset;                           // Root Node���� Find ����.

	while(1)
	{	
		GetNode(m_Node_Offset);                                      // m_Node_Offset�� ������ �ش� Node�� Current_Node�� Set.

		// External Node�� ��� Break.
		if(Current_Node->Node_Mode == 3 || Current_Node->Node_Mode == 4)
			break;
		else
		{
			for(int i=0; i<Current_Node->Element_Count; i++)         // �� Node�� Valid�� Element �� ��ŭ Loop.
			{
				if(m_Find_Key <= Current_Node->Node_Element[i].Key)  // m_Find_Key�� ���ϴ� Key���� �۰ų� ������ ���� Node�� �̵�.
				{
					m_Node_Offset = Current_Node->Node_Element[i].Offset;
					break;
				}
			}
			if(i == Current_Node->Element_Count)					 // m_Find_Key�� �ϳ��� Node�� ��� Element���� Key���� ū ���.
			{
				if(Current_Node->Element_Count == Element_In_Node)   // ���� Node�� Element���� �� ���ִ� ���.  
					m_Node_Offset = Current_Node->Last_Offset;
				else
					m_Node_Offset = Current_Node->Node_Element[i].Offset;
			}
		}
	}

	// Statement : ���� While Loop�� ���� Current Node�� Key���� ������ ���ɼ��� �ִ� Node�� ����Ǿ��ְ� �̴� External Node�̴�.  
	m_Index = FindInNode(Current_Node, m_Find_Key, m_Is_Find);

	if(m_Is_Find == true)                                            
		return Current_Node->Node_Element[m_Index].Offset;
	else
		return -1;
}

// m_Node_Offset�� ������ �ش� Node�� Current_Node�� Set.

void B_Plus_Tree::GetNode(long m_Offset)
{
	if(Current_Node_Offset == m_Offset)                              // Current_Node�� ���. �׳� �ٷ� Return;
	{
		Dirty = true;
		return;
	}
	if(Dirty)                                                        // Current_Node�� ��ü�ϱ� ���� Dirty Bit�� Check.    
	{
		Tree_File.seekp(Current_Node_Offset, ios::beg);              // Current_Node�� ����� File�� Offset���� ����.
		Tree_File.write((char*)Current_Node, sizeof(Node<float>));    // Current_Node Update.
		Dirty = false;                                               // Dirty Bit Release.
	}

	Current_Node_Offset = m_Offset;
	Tree_File.seekg(Current_Node_Offset, ios::beg);                  // Current_Node_Offset���� Read File Pointer Set. 
	Tree_File.read((char*)Current_Node, sizeof(Node));         // Current_Node�� �о�´�. 
}

// m_Current_Node���� m_Find_Key�� �����ϴ��� Check�Ͽ� �����Ѵٸ� m_Is_Find�� True�� Set�ϰ� Index�� Return, 
// �������� ���� ���� m_Is_Find�� False�� Set�ϰ� Insert�� Index�� Return.

int B_Plus_Tree::FindInNode(Node* m_Current_Node, float m_Find_Key, bool& m_Is_Find)
{                                                                    
	for(int i=0; i<m_Current_Node->Element_Count; i++)
	{
		if(m_Current_Node->Node_Element[i].Key == m_Find_Key)        // Case : ���� Key �� �߰�.          
		{
			m_Is_Find = true;
			return i;
		}
		if(m_Find_Key < m_Current_Node->Node_Element[i].Key)         // Case : �߰����� ���� ���.
		{
			m_Is_Find = false;
			return i;
		}
	}
	// Statement : ���� Node�� �����ϴ� ��� Element���� Key������ ū ���.
	m_Is_Find = false;
	return m_Current_Node->Element_Count;
}

// Insert Function. 1st Param : Tree�� Insert�� Key, 2nd Param : �� Key�� ���� Record�� DB_File �� Block Offset.        

bool B_Plus_Tree::Insert(float m_Inserted_Key, long m_Inserted_Offset)
{
	bool m_Is_Find = false;                                                  
	int m_Insert_Index = -1;

	Find(m_Inserted_Key, m_Insert_Index, m_Is_Find);    

	// Statement 
	// m_Is_Find�� True�� ��� ==> ���� Key���� ���� Elements���� �� ó�� Index�� ������ Return.
	// �� �� ������ Key�� ���� Element�� ���� �� �ִ� ���. ��� �˻��Ͽ� Key ���� ���� ����Ű�� Block Offset�� 
	// ���� ��� Insert���� �ʴ´�. 
	// m_Is_Find�� False�� ��� ==> Current_Node�� m_Insert_Index�� Insert�Ѵ�.

	Insert_Element(m_Inserted_Key, m_Inserted_Offset, Current_Node, m_Insert_Index);
	return true;
}


void B_Plus_Tree::Insert_Element(float m_Inserted_Key, long m_Inserted_Offset, Node* m_Current_Node, int m_Insert_Index)
{
	if(m_Current_Node->Element_Count != Element_In_Node)             // Case : Node�� ������ �ʾƼ� Split�� �ʿ����� ���� ���.
		JustInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);
	else															 // Case : Node�� ������ Split�� �ʿ��� ���.
		SplitInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);               
}

// Node�� ������ �ʾƼ� Split �ʿ���� Insert�� �Ǵ� ���.

void B_Plus_Tree::JustInsert(float m_Inserted_Key, long m_Inserted_Offset, Node* m_Current_Node, int m_Insert_Index)
{ 			
	if(m_Current_Node->Element_Count <= 94)
		m_Current_Node->Node_Element[m_Current_Node->Element_Count+1].Offset = m_Current_Node->Node_Element[m_Current_Node->Element_Count].Offset;
	else
		m_Current_Node->Last_Offset = m_Current_Node->Node_Element[m_Current_Node->Element_Count].Offset;

	for(int j=m_Current_Node->Element_Count; j>m_Insert_Index; j--)  // Insert�� Index ���� ��� Element���� �� ĭ�� �̵��Ѵ�. 
	{
		m_Current_Node->Node_Element[j].Key    = m_Current_Node->Node_Element[j-1].Key;
		m_Current_Node->Node_Element[j].Offset = m_Current_Node->Node_Element[j-1].Offset;
	}

	m_Current_Node->Node_Element[m_Insert_Index].Key    = m_Inserted_Key;
	m_Current_Node->Node_Element[m_Insert_Index].Offset = m_Inserted_Offset;
	m_Current_Node->Element_Count++;                                 // Element Count ����.
	Dirty = true;
	if(Big_Pointer != -1 || Small_Pointer != -1)
	{
		m_Current_Node->Node_Element[m_Insert_Index].Offset = Small_Pointer;
		m_Current_Node->Node_Element[m_Insert_Index+1].Offset = Big_Pointer;
		Big_Pointer = -1;
		Small_Pointer = -1;
	}

	//fout << m_Inserted_Key << " " << Current_Node_Offset << " " << m_Insert_Index << endl;///////
}

// Insertion�� �� Node�� ������ Split�� �ʿ��� ���.

void B_Plus_Tree::SplitInsert(float m_Inserted_Key, long m_Inserted_Offset, Node* m_Current_Node, int m_Insert_Index)
{
	int s, t;
	Node* New_Node = new Node;                           // ���ο� Node�� Create.
	Tree_File.seekg(0, ios::end);
	long  New_Node_Offset = Tree_File.tellg();   

	// New_Node Mode Setting.
	if(m_Current_Node->Node_Mode == 1 || m_Current_Node->Node_Mode == 2)       // Root Node�� Split.		
		New_Node->Node_Mode = 2;	
	else if(m_Current_Node->Node_Mode == 3 || m_Current_Node->Node_Mode == 4)  
	{
		New_Node->Node_Mode = 3;	
		New_Node->Next_Node_Offset = m_Current_Node->Next_Node_Offset;
		m_Current_Node->Next_Node_Offset = New_Node_Offset;
	}
	Tree_File.seekp(0, ios::end);
	New_Node_Offset = Tree_File.tellp();                             // New_Node�� DB_File �� Offset�� �����´�.
	Tree_File.write((char*)New_Node, sizeof(Node));
	Node_Count_In_File++;

	// m_Current_Node�� [0..N/2-1]������ �״�� ���ܵΰ�, [N/2..N]���� Element���� New_Node�� �̵���Ų��.
	for(s=Element_In_Node/2, t=0; s<Element_In_Node; s++, t++)
	{
		New_Node->Node_Element[t].Key    = m_Current_Node->Node_Element[s].Key;
		New_Node->Node_Element[t].Offset = m_Current_Node->Node_Element[s].Offset;

		m_Current_Node->Node_Element[s].Key    = -1;
		m_Current_Node->Node_Element[s].Offset = -1;
	}

	Dirty = true;
	// Node�� Count ����.
	m_Current_Node->Element_Count = Element_In_Node/2;
	New_Node->Element_Count = Element_In_Node - Element_In_Node/2;

	// Node�� Offset ����.
	New_Node->Node_Element[t].Offset = m_Current_Node->Last_Offset;
	m_Current_Node->Last_Offset = -1;

	// m_Inserted_Key�� Insert�Ѵ�.
	if(m_Insert_Index < Element_In_Node/2)                           // Case : �߰��� Element�� Current_Node�� ���ԵǴ� ���.
	{	
		JustInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);
	}
	else                                                             // Case : �߰��� Element�� New_Node�� ���ԵǴ� ���.
	{
		JustInsert(m_Inserted_Key, m_Inserted_Offset, New_Node, (m_Insert_Index - Element_In_Node/2));
	}

	// New_Node�� Update�� ������ DB_File�� Write.
	Tree_File.seekp(New_Node_Offset, ios::beg);
	Tree_File.write((char*)New_Node, sizeof(Node));
	delete New_Node;

	// Parent_Node�� ���Ͽ� Big_Pointer, Small_Pointer�� �����صд�.

	Small_Pointer = Current_Node_Offset; 
	Big_Pointer   = New_Node_Offset;

	float  m_Parent_Insert_Key    = m_Current_Node->Node_Element[m_Current_Node->Element_Count-1].Key;

	if(Get_Parent_Node(m_Parent_Insert_Key, Current_Node_Offset) != -1)
	{
		bool m_Is_Find = false;
		int m_Index = FindInNode(Current_Node, m_Parent_Insert_Key, m_Is_Find);

		for(int i=0; i<Current_Node->Element_Count; i++)
		{
			if(m_Parent_Insert_Key <= Current_Node->Node_Element[i].Key || Current_Node->Node_Element[i].Key == -1)                
			{
				break;
			}
		}
		Insert_Element(m_Parent_Insert_Key, Small_Pointer, Current_Node, i);
	}
	else															 // Root Node�� Split.
	{				
		Node<Type>* New_Root_Node = new Node<Type>;	
		Tree_File.seekp(0, ios::end);
		Root_Node_Offset = Tree_File.tellp();                        // New_Node�� DB_File �� Offset�� �����´�.
		New_Root_Node->Node_Element[0].Key = m_Parent_Insert_Key; 
		New_Root_Node->Node_Element[0].Offset = Small_Pointer;
		New_Root_Node->Node_Element[1].Offset = Big_Pointer;
		Big_Pointer = -1;
		Small_Pointer = -1;
		New_Root_Node->Node_Mode = 1;
		Current_Node->Node_Mode = 3;
		New_Root_Node->Element_Count = 1;
		Node_Count_In_File++;
		Tree_File.write((char*)New_Root_Node, sizeof(Node));
		Tree_File.seekp(0, ios::beg);
		Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
		delete New_Root_Node; 
	}	
}

int B_Plus_Tree::Get_Parent_Node(float m_Inserted_Key, long m_Current_Node_Offset)
{	
	long Temp_P_Node_Offset = -1;
	// Root Node�� �о�´�.
	Tree_File.seekg(0, ios::beg);
	Tree_File.read((char*)&Temp_P_Node_Offset, sizeof(long));

	for(int i=0; i<Node_Count_In_File; i++, Temp_P_Node_Offset += sizeof(Node))
	{
		GetNode(Temp_P_Node_Offset);

		if(Current_Node->Node_Mode == 4)
			return -1;
		if(Current_Node->Node_Mode == 3) 
			continue;

		for(int j=0; j<=Current_Node->Element_Count; j++)            // �� Node�� Valid�� Element �� ��ŭ Loop.
		{
			if(m_Current_Node_Offset == Current_Node->Node_Element[j].Offset)   
				return j;
		}
	}
	return -1;
}

// Parameter�� �ش� Key [Min..Max]�� �����ͼ� Rsearch�� �Ѵ�.

void B_Plus_Tree::Rsearch(float m_Small_Key, float m_Big_Key, int* Result)
{
	int m_Index, Result_Count = 0;
	bool m_Is_Find;
	Find(m_Small_Key, m_Index, m_Is_Find);
	int i = m_Index;

	while(1)
	{
		// �ϳ��� block�� �� ������, ���ϴ� Element�� �������� ���ɼ��� �ִ� ���. ���� Block���� �̵�.
		if(i == (Current_Node->Element_Count-1) && Current_Node->Node_Element[i].Key <=  m_Big_Key)
		{
			Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
			if(Current_Node->Next_Node_Offset != -1)
			{
				GetNode(Current_Node->Next_Node_Offset);
				i = 0;
			}
			else
				return;

		}

		// Change�Ǵ� Key ���� �߰��� ���.
		if(m_Small_Key <= Current_Node->Node_Element[i].Key && Current_Node->Node_Element[i].Key <= m_Big_Key)
			Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
		if(m_Big_Key < Current_Node->Node_Element[i].Key)
			return;
		i++;
	}
	return;
}
Dynamic_Hash* H; 
B_Plus_Tree* Tree;	

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
	Tree = new B_Plus_Tree<float>(Filename);

	return true;
}

unsigned insertRecord(char* name, unsigned ID, float score, char* dept)
{
	char* temp = new char[10];
	char* temp1 = new char[10];
	string id;
	bool Is_Insert = false;

	unsigned blockNumber = 0;

	sprintf(temp, "%d",ID);
	id=temp;

	int DB_File_Offset = H->Get_Hash_Offset(id);               

	DB_File.seekg(DB_File_Offset, ios::beg);                                
	DB_File.read((char*)&DB_block, sizeof(Block));
	
	for(int i=0; i<DB_block.Record_Count; i++)			           
		if(DB_block.Record[i].ID == ID)		
			return false;

	if(DB_block.Record_Count < IN_BLOCK_MAX)                        
	{		
		strcpy(DB_block.Record[DB_block.Record_Count].Name, name);			
		DB_block.Record[DB_block.Record_Count].ID = ID;
		DB_block.Record[DB_block.Record_Count].Score = score;
		strcpy(DB_block.Record[DB_block.Record_Count].Dept,dept);

		DB_block.Record_Count++;

		DB_File.seekp(DB_File_Offset, ios::beg);      
		DB_File.write((char*)&DB_block, sizeof(Block));	
		Is_Insert = true;

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

	if(Is_Insert == true)
	{
		Tree->Insert(score,ID);
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
unsigned searchScore(float lower, float upper){
	
	unsigned numOfScore = 0;

	// Score_RSearch with B_Plus_Tree.
// Data[1] = Min. Data[2] = Max.

	int Result[20000];
	long DB_File_Offset;
	int i;
	for(i=0; i<20000; i++)
		Result[i] = -1;
	
	// Result�� �˻��� ID�� ����Ǿ� �ִ�.
	Tree->Rsearch(ID, score), Result);
	
	int m_Total_Count = 0;
	
	char c_m_ID[80] = "";
	string s_m_ID   = "";
	for(i=0; i<20000; i++)
	{	
		if(Result[i] != -1)
		{	
			sprintf(c_m_ID, "%d", Result[i]);
			s_m_ID = c_m_ID;
			DB_File_Offset = H->Get_Hash_Offset(s_m_ID);
			DB_File.seekg(DB_File_Offset, ios::beg);                  // �ش� Block�� �о� �´�.      
			DB_File.read((char*)&DB_Buffer, sizeof(Block));
			
			for(int j=0; j<DB_block.Record_Count; j++)			           
				if(Result[i] == DB_block.Record[j].ID)              // Case : ID�� �ش��ϴ� Record�� �����ϴ� ���.		
				{
					strncpy(Dept, DB_Buffer.Record[j].Dept, 10);
					Dept[10] = '\0';
					cout<<"| "<<right<<setw(6)<< ++m_Total_Count << " | " <<left <<setw(12)<< DB_Buffer.Record[j].Name <<"| "<< setw(12) << DB_Buffer.Record[j].ID ;
					cout<<"| "<<right<<setw(5) << DB_Buffer.Record[j].Score<<" | "<< setw(6)  << DB_Buffer.Record[j].Credit ;
					cout<<" | "<<left <<setw(11)<< Dept <<"| "<< setw(5)  << DB_Buffer.Record[j].Year << "|" << endl;
					cout<<"+-------------------------------------------------------------------------+" << endl;
					break;
				}
		}
		else
			break;
	}	

	cout << endl;
	if(m_Total_Count)
		cout << "Total " << m_Total_Count << endl;
	else
		cout << "[MSG] No Record Look Up." << endl;

}

	return numOfScore;
}


// =========================Your code here down!!(End)=========================




// =========================You don't need to touch=========================         
// main
int main()
{

	openDB("myDB");

	// Input data
	ifstream fin("Assignment2.inp", ios::in);
	// for ESPA
	ofstream fout("Assignment2.out", ios::out);

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
			char name[20];
			strcpy(name, fullName.c_str());
			// for Visual Studio 2012
			// strcpy_s(name, fullName.c_str());
			char dept[10];
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

		} else if(type == "c"){    // searchScore
			float lower = 0;
			float upper = 0;

			fin >> lower >> upper;
			//cout << type << "\t" << lower << "\t" << upper << endl;

			// searchScore
			unsigned result = searchScore(lower, upper);
			fout << result << endl;               
		}
	}

	fout.close();
	fin.close();

	// for Dev C++
	/*
	system("PAUSE");
	return EXIT_SUCCESS;
	*/
	return 0;
}

// ======================You don't need to touch (End)======================
