#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <string.h>
#include <cmath>

#define BLOCKSIZE  4096                                     
#define BLOCK_MAX  BLOCKSIZE/sizeof(Student)                      
#pragma pack(1)

using namespace std;

fstream Dat_File;

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
		if(infoVector[i].studentId == studentId){
			if(infoVector[i].blockNumber == blockNumber){
				isThere = true;
			}              
		}                
	}     

	return isThere;
}

void DeleteBlockNumber(unsigned blockNumber, unsigned studentId)
{
	for(int i = 0; i < infoVector.size(); i++){
		if(infoVector[i].studentId == studentId){
			if(infoVector[i].blockNumber == blockNumber){
				infoVector.erase(infoVector.begin() + i);   
			}
		}
	}
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
	int  Record_Count;                                               
	int  Bit_Number;																	
	char Block_Garbage[22];
	Block();
};

Student::Student()
{
	ID     = 0;
	Score  = 0;
	memset(Name, '\0', sizeof(Name));
	memset(Dept, '\0', sizeof(Dept));
}

Block::Block()
{
	Record_Count      = 0;
	Bit_Number        = 0;  
}

Block Dat_Block;

class HashTable{
public:
	HashTable();
	long Table_Block_Offset[BLOCKSIZE/sizeof(long)];		         
};

class Dynamic_Hash{

private:
	fstream Hash_File; 
	int Table_Bit_Number; 
	HashTable Hash_Table;  

	unsigned HASH(unsigned ID);                               
	void Add_Table(unsigned Hash_Key, fstream& Dat_File);		  

public:
	Dynamic_Hash(char* filename);
	~Dynamic_Hash();
	unsigned Get_blockNumber(unsigned stu_id);		                         
	void Split_Block(unsigned stu_id, int Block_Bit_Number, fstream& Dat_File);

};

HashTable::HashTable()
{
	for(int i=0; i<(BLOCKSIZE/sizeof(long)); i++)                   
		Table_Block_Offset[i] = -1;
}

Dynamic_Hash::Dynamic_Hash(char* filename)
{
	Hash_File.open(filename, ios::binary | ios::in | ios::out);

	if(!Hash_File)                                                   
	{
		Hash_File.clear();
		Hash_File.open(filename, ios::binary | ios::in | ios::out | ios::trunc);
		Table_Bit_Number = 0;
		Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number)); 
		Hash_Table.Table_Block_Offset[0] = 0;                        
		Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));
	}
	else
	{
		Hash_File.read((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
		Hash_File.read((char*)&Hash_Table, sizeof(Hash_Table));      
	}
}

Dynamic_Hash::~Dynamic_Hash()
{
	Hash_File.seekp(0, ios::beg);
	Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
	Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));
	Hash_File.close();
}

unsigned Dynamic_Hash::HASH(unsigned ID)
{
	char temp[10];
	string id;
	unsigned seed =131;

	unsigned key = 0;
	sprintf(temp, "%d", ID);
	id = temp;

	for(unsigned int i = 0; i < id.length(); i++)
	{
		key = (key * seed) + id[i];
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

unsigned Dynamic_Hash::Get_blockNumber(unsigned stu_id)
{
	unsigned Hash_Key = HASH(stu_id);              
	if(Table_Bit_Number == 0)
	{
		Hash_Key = 0;
	}
	else
	{
		Hash_Key = Hash_Key >> (32-Table_Bit_Number);  
	}	
	return Hash_Table.Table_Block_Offset[Hash_Key]; 
}


void Dynamic_Hash::Split_Block(unsigned stu_id, int Block_Bit_Number, fstream& Dat_File)
{
	unsigned Hash_Key = HASH(stu_id);

	if(Table_Bit_Number != 0)                                           
		Hash_Key = Hash_Key >> (32-Table_Bit_Number);  
	else
		Hash_Key = 0;

	if(Block_Bit_Number == Table_Bit_Number)                              
	{
		Add_Table(Hash_Key, Dat_File);
		return;
	}
	else if(Block_Bit_Number < Table_Bit_Number)                                        
	{
		Block* Buffer = new Block;                                 
		Dat_File.seekp(0, ios::end);
		long Add_Block_Offset = Dat_File.tellp();                  
		Block* Add_Block = new Block;                             
		Block* Before_Block = new Block;                              

		Dat_File.seekg(Hash_Table.Table_Block_Offset[(int)Hash_Key], ios::beg);
		Dat_File.read((char*)Buffer, sizeof(Block));

		Before_Block->Bit_Number = Buffer->Bit_Number + 1;
		Add_Block->Bit_Number = Buffer->Bit_Number + 1;

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
			Hash_Table.Table_Block_Offset[i] = Add_Block_Offset;

		for(int j=0; j<107; j++)
		{
			unsigned s_id = Buffer->Record[j].ID;
			unsigned student_key = HASH(s_id);
			student_key = student_key >> (32-Table_Bit_Number);  
			if((int)student_key < (start+(end-start)/2)+1)
				Before_Block->Record[Before_Block->Record_Count++] = Buffer->Record[j];

			else
			{
				Add_Block->Record[Add_Block->Record_Count++] = Buffer->Record[j];

			}
		}

		Dat_File.seekp(Hash_Table.Table_Block_Offset[start], ios::beg);
		Dat_File.write((char*)Before_Block, sizeof(Block));
		Dat_File.seekp(Add_Block_Offset, ios::beg);
		Dat_File.write((char*)Add_Block, sizeof(Block));

		Hash_File.seekp(0, ios::beg);
		Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number)); 
		Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));

		delete Buffer;
		delete Add_Block;
		delete Before_Block;
	}
}


void Dynamic_Hash::Add_Table(unsigned Hash_Key, fstream& Dat_File)
{	
	Block* Buffer = new Block;                                     
	Dat_File.seekp(0, ios::end);

	long Add_Block_Offset = Dat_File.tellp();                          
	Block* Add_Block = new Block;                                   
	Block* Before_Block = new Block;	

	unsigned Table_Block_Number = 2;                                                           
	if(Table_Bit_Number != 0)
		Table_Block_Number = Table_Block_Number << (Table_Bit_Number-1);
	else
		Table_Block_Number = 1;
	for(int i=Table_Block_Number-1; i>=0; i--)                         
	{
		if(i == (int)Hash_Key)                                  
		{
			Hash_Table.Table_Block_Offset[2*i]   = Hash_Table.Table_Block_Offset[i];
			Hash_Table.Table_Block_Offset[2*i+1] = Add_Block_Offset;

			Dat_File.seekg(Hash_Table.Table_Block_Offset[i], ios::beg);
			Dat_File.read((char*)Buffer, sizeof(Block));

			Table_Bit_Number++;
			Before_Block->Bit_Number = Table_Bit_Number;
			Add_Block->Bit_Number = Table_Bit_Number;

			for(unsigned j=0; j<107; j++)
			{
				unsigned s_id=Buffer->Record[j].ID;
				unsigned student_key = HASH(s_id);
				student_key = student_key >> (32-Table_Bit_Number);  
				if(student_key == 2*i)
					Before_Block->Record[Before_Block->Record_Count++] = Buffer->Record[j];
				else
				{
					Add_Block->Record[Add_Block->Record_Count++] = Buffer->Record[j];
				}
			}
			Dat_File.seekp(Hash_Table.Table_Block_Offset[2*i], ios::beg);
			Dat_File.write((char*)Before_Block, sizeof(Block));
			Dat_File.seekp(Add_Block_Offset, ios::beg);
			Dat_File.write((char*)Add_Block, sizeof(Block));
		}
		else                                                         
		{
			Hash_Table.Table_Block_Offset[2*i]   = Hash_Table.Table_Block_Offset[i];
			Hash_Table.Table_Block_Offset[2*i+1] = Hash_Table.Table_Block_Offset[i];
		}
	}

	Hash_File.seekp(0, ios::beg);
	Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number)); 
	Hash_File.write((char*)&Hash_Table, sizeof(Hash_Table));

	delete Buffer;
	delete Add_Block;
	delete Before_Block;
}

Dynamic_Hash* H; 

class Index{
public : 
	long  Offset;													
	float  student_score;                                                       
	Index();
};

Index::Index()
{
	Offset = -1;
	student_score = -1;
}

class Node{
public :
	Index Node_Element[BLOCKSIZE/sizeof(Index)-2];   
	long    Last_Offset;                                           
	long    Next_Node_Offset;                                        
	int     Element_Count;                                         
	int     Node_part;       // 1: Root Node 2: Internal Node 3: External Node. 4 : Root Node & External Node.                                        
	Node();                                                                                                     
};

Node::Node()                                                         
{	
	Last_Offset = -1;
	Next_Node_Offset = -1;
	Element_Count = 0;
	Node_part = 0;	             
}

class B_Tree{
private :

	Node* Current_Node;
	fstream  Tree_File;

	int      Node_Count;                                     
	long     Root_Node_Offset;                                       
	long     Current_Node_Offset;                                    
	bool     checkok;                                                 
	int      Element_In_Node;                                      
	long     Left_Node;                                         
	long     Right_Node;                                            

	void Open_BTree(char* filename);                                       
	void Get_Node(long s_Offset);				                  
	int  Find_Score_Node(Node* Execution_Node, float score_key, bool& score_find);       
	void Insert_Kind(float Student_score, unsigned Student_ID, Node* Execution_Node, int Insert_Index);
	void Direct_Insert(float Student_score,  unsigned Student_ID, Node* Execution_Node, int Insert_Index);
	void Split_Insert(float Student_score, unsigned Student_ID, Node* Execution_Node, int Insert_Index);
	int  Parent_Node(float Student_score, long Execution_Node_Offset);            

public :
	B_Tree(char* filename);
	~B_Tree();

	long ScoreFind(float score_key, int& score_index, bool& score_find);      												 
	bool ScoreInsert(float Student_score, unsigned Student_ID); 
	unsigned RangeScoreSearch(float lower, float upper);
	void Delete(float Deleted_Key, unsigned Deleted_Offset); 
};


B_Tree::B_Tree(char* filename)
{
	Current_Node = new Node;                                   
	Current_Node_Offset = -1;
	checkok = false;

	Element_In_Node = BLOCKSIZE/sizeof(Index)-2;           

	Left_Node = -1;
	Right_Node   = -1;

	Open_BTree(filename);
}


B_Tree::~B_Tree()
{
	if(checkok && Current_Node != NULL)
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


void B_Tree::Open_BTree(char* filename)
{
	Tree_File.open(filename, ios::in | ios::out | ios::binary);
	if(!Tree_File)                                                   
	{
		Tree_File.open(filename, ios::in | ios::out | ios::binary | ios::trunc);
		Node* Root = new Node;                                      
		Root->Node_part = 4;                                         
		Tree_File.seekp(sizeof(long), ios::beg);
		Root_Node_Offset = Tree_File.tellp();
		Tree_File.write((char*)Root, sizeof(Node));            
		Tree_File.seekp(0, ios::beg);
		Tree_File.write((char*)&Root_Node_Offset, sizeof(long));     
		delete Root;
	}
	else                                                             
	{
		Tree_File.seekg(0, ios::beg);        
		Tree_File.read((char*)&Root_Node_Offset, sizeof(long));    
	}

	Tree_File.seekg(0, ios::end);    
	long tellg = Tree_File.tellg();
	Node_Count = (tellg-(long)4)/(sizeof(Node));
}


long B_Tree::ScoreFind(float score_key, int& score_index, bool& score_find)
{
	score_find = false;
	score_index = 0;
	int i=0;
	long offset = Root_Node_Offset;                          

	while(1)
	{	
		Get_Node(offset);                                     
		if(Current_Node->Node_part == 3 || Current_Node->Node_part == 4)
			break;
		else
		{	
			for(i=0; i<Current_Node->Element_Count; i++)      
			{
				if(score_key <= Current_Node->Node_Element[i].student_score)  
				{
					offset = Current_Node->Node_Element[i].Offset;
					break;
				}
			}
			if(i == Current_Node->Element_Count)					
			{
				if(Current_Node->Element_Count == Element_In_Node)   
					offset = Current_Node->Last_Offset;
				else
					offset = Current_Node->Node_Element[i].Offset;
			}
		}
	}
	score_index = Find_Score_Node(Current_Node, score_key, score_find);

	if(score_find == true)                                            
		return Current_Node->Node_Element[score_index].Offset;
	else
		return -1;
}

void B_Tree::Get_Node(long s_Offset)
{
	if(Current_Node_Offset == s_Offset)                            
	{
		checkok = true;
		return;
	}
	if(checkok)                                                       
	{
		Tree_File.seekp(Current_Node_Offset, ios::beg);              
		Tree_File.write((char*)Current_Node, sizeof(Node));    
		checkok = false;                                              
	}

	Current_Node_Offset = s_Offset;
	Tree_File.seekg(Current_Node_Offset, ios::beg);                
	Tree_File.read((char*)Current_Node, sizeof(Node));         
}

int B_Tree::Find_Score_Node(Node* Execution_Node, float score_key, bool& score_find)
{                                                                    
	for(int i=0; i<Execution_Node->Element_Count; i++)
	{
		if(Execution_Node->Node_Element[i].student_score == score_key)        
		{
			score_find = true;
			return i;
		}
		if(score_key < Execution_Node->Node_Element[i].student_score)         
		{
			score_find = false;
			return i;
		}
	}
	score_find = false;
	return Execution_Node->Element_Count;
}

bool B_Tree::ScoreInsert(float Student_score, unsigned Student_ID)
{
	bool score_find = false;                                                  
	int Insert_Index = -1;

	ScoreFind(Student_score, Insert_Index, score_find);    
	Insert_Kind(Student_score, Student_ID, Current_Node, Insert_Index);
	return true;
}

void B_Tree::Insert_Kind(float Student_score, unsigned Student_ID, Node* Execution_Node, int Insert_Index)
{
	if(Execution_Node->Element_Count != Element_In_Node)      
		Direct_Insert(Student_score, Student_ID, Execution_Node, Insert_Index);
	else														
		Split_Insert(Student_score, Student_ID, Execution_Node, Insert_Index);               
}

void B_Tree::Direct_Insert(float Student_score, unsigned Student_ID, Node* Execution_Node, int Insert_Index)
{ 			
	if(Execution_Node->Element_Count <= 106)
		Execution_Node->Node_Element[Execution_Node->Element_Count+1].Offset = Execution_Node->Node_Element[Execution_Node->Element_Count].Offset;
	else
		Execution_Node->Last_Offset = Execution_Node->Node_Element[Execution_Node->Element_Count].Offset;

	for(int j=Execution_Node->Element_Count; j>Insert_Index; j--)  
	{
		Execution_Node->Node_Element[j].student_score    = Execution_Node->Node_Element[j-1].student_score;
		Execution_Node->Node_Element[j].Offset = Execution_Node->Node_Element[j-1].Offset;
	}

	Execution_Node->Node_Element[Insert_Index].student_score    = Student_score;
	Execution_Node->Node_Element[Insert_Index].Offset = Student_ID;
	Execution_Node->Element_Count++;                               
	checkok = true;

	if(Right_Node != -1 || Left_Node != -1)
	{
		Execution_Node->Node_Element[Insert_Index].Offset = Left_Node;
		Execution_Node->Node_Element[Insert_Index+1].Offset = Right_Node;
		Right_Node = -1;
		Left_Node = -1;
	}

}


void B_Tree::Split_Insert(float Student_score, unsigned Student_ID, Node* Execution_Node, int Insert_Index)
{
	int s, t;
	Node* New_Node = new Node;
	Tree_File.seekg(0, ios::end);
	long  New_Node_Offset = Tree_File.tellg();   


	if(Execution_Node->Node_part == 1 || Execution_Node->Node_part == 2)       
		New_Node->Node_part = 2;	
	else if(Execution_Node->Node_part == 3 || Execution_Node->Node_part == 4)  
	{
		New_Node->Node_part = 3;	
		New_Node->Next_Node_Offset = Execution_Node->Next_Node_Offset;
		Execution_Node->Next_Node_Offset = New_Node_Offset;
	}
	Tree_File.seekp(0, ios::end);
	New_Node_Offset = Tree_File.tellp();                          
	Tree_File.write((char*)New_Node, sizeof(Node));
	Node_Count++;

	for(s=Element_In_Node/2, t=0; s<Element_In_Node; s++, t++)
	{
		New_Node->Node_Element[t].student_score    = Execution_Node->Node_Element[s].student_score;
		New_Node->Node_Element[t].Offset = Execution_Node->Node_Element[s].Offset;

		Execution_Node->Node_Element[s].student_score    = -1;
		Execution_Node->Node_Element[s].Offset = -1;
	}

	checkok = true;

	Execution_Node->Element_Count = Element_In_Node/2;
	New_Node->Element_Count = Element_In_Node - Element_In_Node/2;

	New_Node->Node_Element[t].Offset = Execution_Node->Last_Offset;
	Execution_Node->Last_Offset = -1;


	if(Insert_Index < Element_In_Node/2)                           
	{	
		Direct_Insert(Student_score, Student_ID, Execution_Node, Insert_Index);
	}
	else                                                            
	{
		Direct_Insert(Student_score, Student_ID, New_Node, (Insert_Index - Element_In_Node/2));
	}

	Tree_File.seekp(New_Node_Offset, ios::beg);
	Tree_File.write((char*)New_Node, sizeof(Node));
	delete New_Node;

	Left_Node = Current_Node_Offset; 
	Right_Node   = New_Node_Offset;

	float  Parent_Insert_Score    = Execution_Node->Node_Element[Execution_Node->Element_Count-1].student_score;

	if(Parent_Node(Parent_Insert_Score, Current_Node_Offset) != -1)
	{
		bool score_find = false;
		int score_index = Find_Score_Node(Current_Node, Parent_Insert_Score, score_find);
		int i=0;

		for(i=0; i<Current_Node->Element_Count; i++)
		{
			if(Parent_Insert_Score <= Current_Node->Node_Element[i].student_score || Current_Node->Node_Element[i].student_score == -1)                
			{
				break;
			}
		}
		Insert_Kind(Parent_Insert_Score, Left_Node, Current_Node, i);

	}
	else															
	{				
		Node* New_Root_Node = new Node;	
		Tree_File.seekp(0, ios::end);
		Root_Node_Offset = Tree_File.tellp();                      

		New_Root_Node->Node_Element[0].student_score = Parent_Insert_Score; 
		New_Root_Node->Node_Element[0].Offset = Left_Node;
		New_Root_Node->Node_Element[1].Offset = Right_Node;

		Right_Node = -1;
		Left_Node = -1;

		New_Root_Node->Node_part = 1;
		Current_Node->Node_part = 3;

		New_Root_Node->Element_Count = 1;
		Node_Count++;

		Tree_File.write((char*)New_Root_Node, sizeof(Node));
		Tree_File.seekp(0, ios::beg);
		Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
		delete New_Root_Node; 
	}	
}

int B_Tree::Parent_Node(float Student_score, long Execution_Node_Offset)
{	
	long Parent_Offset = -1;

	Tree_File.seekg(0, ios::beg);
	Tree_File.read((char*)&Parent_Offset, sizeof(long));

	for(int i=0; i<Node_Count; i++, Parent_Offset += sizeof(Node))
	{
		Get_Node(Parent_Offset);

		if(Current_Node->Node_part == 4)
			return -1;
		if(Current_Node->Node_part == 3) 
			continue;

		for(int j=0; j<=Current_Node->Element_Count; j++)           
		{
			if(Execution_Node_Offset == Current_Node->Node_Element[j].Offset)   
				return j;
		}
	}
	return -1;
}

unsigned B_Tree::RangeScoreSearch(float lower, float upper)
{
	int score_index=0;
	unsigned Score_Number = 0;
	bool score_find;
	ScoreFind(lower, score_index, score_find);
	int i = score_index;

	while(1)
	{
		if(i == (Current_Node->Element_Count-1) && Current_Node->Node_Element[i].student_score <=  upper)
		{
			Score_Number++;
			if(Current_Node->Next_Node_Offset != -1)
			{
				Get_Node(Current_Node->Next_Node_Offset);
				i = 0;
			}
			else
				break;
		}
		if(lower <= Current_Node->Node_Element[i].student_score && Current_Node->Node_Element[i].student_score <= upper)
			Score_Number++;
		if(upper < Current_Node->Node_Element[i].student_score)
			break;
		i++;
	}
	return Score_Number;
}

void B_Tree::Delete(float Deleted_Key, unsigned Deleted_Offset)           
{
	bool Is_Delete;
	int Deleted_Index = -1;
	int j=0;

	ScoreFind(Deleted_Key, Deleted_Index, Is_Delete);
	int i=Deleted_Index;

	while(1)
	{
		for(i; i<Current_Node->Element_Count; i++)
		{
			if(Current_Node->Node_Element[i].student_score    == Deleted_Key &&
				Current_Node->Node_Element[i].Offset == Deleted_Offset)
			{
				for(j=i+1; j<Current_Node->Element_Count; j++)
				{
					Current_Node->Node_Element[j-1].student_score    = Current_Node->Node_Element[j].student_score;
					Current_Node->Node_Element[j-1].Offset = Current_Node->Node_Element[j].Offset;
				}

				if(Current_Node->Element_Count == Element_In_Node)
				{
					Current_Node->Node_Element[j-1].student_score = -1;
					Current_Node->Node_Element[j-1].Offset = Current_Node->Last_Offset;
					Current_Node->Last_Offset = -1;
				}
				else
				{		
					Current_Node->Node_Element[j-1].student_score = -1;
					Current_Node->Node_Element[j-1].Offset = Current_Node->Node_Element[j].Offset;
				}
				Current_Node->Element_Count--;
				checkok = true;
				return;                                         
			}		
		}

		if(i == Current_Node->Element_Count && Current_Node->Node_Element[i-1].student_score == Deleted_Key)
		{
			Get_Node(Current_Node->Next_Node_Offset);
			i = 0;
		}
		else
			return;
	}	
}

B_Tree* T;

bool openDB(char* filename){

	char Filename[80] = "";
	sprintf(Filename, "%s.dat", filename);
	Dat_File.open(Filename, ios::in | ios::out | ios::binary);               

	if(!Dat_File)
	{
		Dat_File.clear();
		Dat_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
		Dat_File.seekp(0, ios::beg);                                        
		Dat_File.write((char*)&Dat_Block, sizeof(Block));
	} 
	strcpy(Filename,"");
	sprintf(Filename, "%s.hash", filename);
	H = new Dynamic_Hash(Filename);

	strcpy(Filename,""); 
	sprintf(Filename, "%s.idx", filename);
	T = new B_Tree(Filename);

	return true;
}

unsigned insertRecord(char* name, unsigned ID, float score, char* dept)
{
	unsigned blockNumber = 0;

	blockNumber = H->Get_blockNumber(ID);

	Dat_File.seekg(blockNumber, ios::beg);            
	Dat_File.read((char*)&Dat_Block, sizeof(Block));

	bool checkok=true;

	for(int i=0; i<Dat_Block.Record_Count; i++)			           
		if(Dat_Block.Record[i].ID == ID)		
			return false;

	if(checkok) {

		if(Dat_Block.Record_Count < BLOCK_MAX)                        
		{		
			strcpy(Dat_Block.Record[Dat_Block.Record_Count].Name, name);			
			Dat_Block.Record[Dat_Block.Record_Count].ID = ID;
			Dat_Block.Record[Dat_Block.Record_Count].Score = score;
			strcpy(Dat_Block.Record[Dat_Block.Record_Count].Dept,dept);

			Dat_Block.Record_Count++;

			Dat_File.seekp(blockNumber, ios::beg);      
			Dat_File.write((char*)&Dat_Block, sizeof(Block));	
			T->ScoreInsert(score, ID);
			return blockNumber;
		}
		else                                                             
		{															
			H->Split_Block(ID, Dat_Block.Bit_Number, Dat_File);
			for(int i=0; i<Dat_Block.Record_Count; i++)
			{
				unsigned studentid = Dat_Block.Record[i].ID;
				blockNumber = H->Get_blockNumber(studentid);
				insertBlockNumber(blockNumber, studentid);
			}
			insertRecord(name,ID,score,dept);
		}
	}

}
unsigned searchID(unsigned ID){

	unsigned blockNumber = 0;
	blockNumber = H->Get_blockNumber(ID);     
	return blockNumber;
}

bool deleteRecord(unsigned ID){

	unsigned DB_File_Offset = H->Get_blockNumber(ID);             

	Dat_File.seekg(DB_File_Offset, ios::beg);                              
	Dat_File.read((char*)&Dat_Block, sizeof(Block));

	int i;
	for(i=0; i<Dat_Block.Record_Count; i++)			           
		if(Dat_Block.Record[i].ID == ID)        
			break;

	if(i==Dat_Block.Record_Count)                                   
	{
		return false;
	}

	DeleteBlockNumber(DB_File_Offset,ID);
	float Delete_Score = Dat_Block.Record[i].Score;

	int j;
	for(j=i; j<Dat_Block.Record_Count-1; j++)                  
		Dat_Block.Record[j] = Dat_Block.Record[j+1];

	Dat_Block.Record[j].ID     = 0;
	Dat_Block.Record[j].Score  = 0;
	strcpy(Dat_Block.Record[j].Name, "");
	strcpy(Dat_Block.Record[j].Dept, "");
	Dat_Block.Record_Count--;

	Dat_File.seekp(DB_File_Offset, ios::beg);                               
	Dat_File.write((char*)&Dat_Block, sizeof(Block));

	T->Delete(Delete_Score, ID);


	for(int i=0; i<Dat_Block.Record_Count; i++)
	{
		unsigned studentid = Dat_Block.Record[i].ID;
		unsigned blockNumber = H->Get_blockNumber(studentid);
		insertBlockNumber(blockNumber, studentid);
	}

	return true;

}
unsigned searchScore(float lower, float upper)
{
	unsigned numOfScore = 0;
	numOfScore=T->RangeScoreSearch(lower,upper);
	return numOfScore;
}

// =========================Your code here down!!(End)=========================




// =========================You don't need to touch=========================         
// main
int main()
{

	openDB("myDB");

	// Input data
	ifstream fin("Assignment3.inp", ios::in);
	// for ESPA
	ofstream fout("Assignment3.out", ios::out);

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

		} else if(type == "c"){    // searchScore
			float lower = 0;
			float upper = 0;

			fin >> lower >> upper;
			//cout << type << "\t" << lower << "\t" << upper << endl;

			// searchScore
			unsigned result = searchScore(lower, upper);
			fout << result << endl;   

		} else if(type == "d"){
			unsigned studentId = 0;

			fin >> studentId;
			// cout << type << "\t" << studentId << endl;

			// deleteRecord
			bool isThere = false; 
			isThere = deleteRecord(studentId);
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

