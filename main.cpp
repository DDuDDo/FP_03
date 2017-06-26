#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>
#include <iomanip>
#include <string>
#include <sstream>
#include <cstring>

#define TABLESIZE  8192
#define BLOCKSIZE  4096
#define BLOCKCOUNT 12000
#define BLOCK_MAX  127
#pragma pack(1)

using namespace std;

ofstream query_out("query.res");
int table_type ;

unsigned str2unsign (const string &str) {
  stringstream ss(str);
  unsigned num;
  if((ss >> num).fail())
  {
      /* 에러 */
  }
  return num;
}
int str2int (const string &str) {
  stringstream ss(str);
  int num;
  if((ss >> num).fail())
  {
      /* 에러 */
  }
  return num;
}
float str2float (const string &str) {
  stringstream ss(str);
  float num;
  if((ss >> num).fail())
  {
      /* 에러 */
  }
  return num;
}

// 32
class Student{
public :
   char Name[20];//20
   unsigned ID;//4
   float Score;//4
   unsigned advisorID;
   Student();
};
// 28
class Proffesor{
public:
    char Name[20];//20
    unsigned ProfID;//4
    int Salary; // 4
    Proffesor();
};

class Block{
public :
   Student Record[127];
   Block();
   int  Record_Count;
   int  Bit_Number;
   char Block_Garbage[24];
};

class Block_p{
public :
   Proffesor Record[146];
   Block_p();
   int  Record_Count;
   int  Bit_Number;
};

Student::Student()
{
   ID     = 0;
   Score  = 0;
   memset(Name, '\0', sizeof(Name));
    advisorID = 0;
}
Proffesor::Proffesor()
{
   ProfID     = 0;
   Salary  = 0;
   memset(Name, '\0', sizeof(Name));
}

Block::Block()
{
   Record_Count      = 0;
   Bit_Number        = 0;
}
Block_p::Block_p()
{
   Record_Count      = 0;
   Bit_Number        = 0;
}
Block Dat_block;
Block_p Dat_block_p;

class Hash_Table{
public:
   Hash_Table();
   long Table_Block_Offset[TABLESIZE];
};

class Dynamic_Hash{

private:
   unsigned HASH(string str);
   void Expand_Table(unsigned Primary_Index, fstream& Dat_File);
   void Expand_Table_p(unsigned Primary_Index, fstream& Dat_File);
   int Table_Bit_Number;
   Hash_Table Hash_table;
   fstream Hash_File;

public:
    int return_Table_bit();
   Dynamic_Hash(char* filename);
   ~Dynamic_Hash();
   unsigned Get_Hash_Offset(string stu_id);
   void Block_Full(string stu_id, int Block_Bit_Number, fstream& Dat_File);
   void Block_Full_p(string stu_id, int Block_Bit_Number, fstream& Dat_File);
   void Make_txt(fstream& Dat_File,char* filename);
   void Print_Hash();
};

Hash_Table::Hash_Table()
{
   for(int i=0; i<TABLESIZE; i++)
      Table_Block_Offset[i] = -1;
}
int Dynamic_Hash::return_Table_bit(){
    return Table_Bit_Number;
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
      Hash_table.Table_Block_Offset[0] = 0;
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
   }
   else
   {
      Hash_File.read((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.read((char*)&Hash_table, sizeof(Hash_table));
   }

}

Dynamic_Hash::~Dynamic_Hash()
{
   Hash_File.seekp(0, ios::beg);
   Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
   Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
   Hash_File.close();
}

unsigned Dynamic_Hash::HASH(string str)
{
   unsigned key = 0;
   unsigned i;

   for(i = 0; i < str.length(); i++)
   {
      key = (key * 2014) + str[i];
   }

   key = key%TABLESIZE;

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

unsigned Dynamic_Hash::Get_Hash_Offset(string stu_id)
{

   unsigned Primary_Index = HASH(stu_id);

   if(Table_Bit_Number == 0)
   {
      Primary_Index = 0;
   }
   else
   {
      Primary_Index = Primary_Index >> (32-Table_Bit_Number);
   }
   return Hash_table.Table_Block_Offset[Primary_Index];
}


void Dynamic_Hash::Block_Full(string stu_id, int Block_Bit_Number, fstream& Dat_File)
{
   unsigned Primary_Index = HASH(stu_id);
   if(Table_Bit_Number != 0)
      Primary_Index = Primary_Index >> (32-Table_Bit_Number);
   else
      Primary_Index = 0;

   if(Block_Bit_Number == Table_Bit_Number)
   {

      Expand_Table(Primary_Index, Dat_File);
      return;
   }
   else if(Block_Bit_Number < Table_Bit_Number)
   {
      Block* Buffer = new Block;
      Dat_File.seekp(0, ios::end);
      std::streamoff a = Dat_File.tellg();
      long New_Block_Offset = a;
      Block* New_Block = new Block;
      Block* Old_Block = new Block;

      Dat_File.seekg(Hash_table.Table_Block_Offset[(int)Primary_Index], ios::beg);
      Dat_File.read((char*)Buffer, sizeof(Block));
      Old_Block->Bit_Number = Buffer->Bit_Number + 1;
      New_Block->Bit_Number = Buffer->Bit_Number + 1;

      int start = -1, end = -1;
      int i;
      for(i=0; i<TABLESIZE; i++)
      {
         if(start == -1 && Hash_table.Table_Block_Offset[i] == Hash_table.Table_Block_Offset[(int)Primary_Index])
            start = i;
         if(start != -1 && Hash_table.Table_Block_Offset[i] != Hash_table.Table_Block_Offset[(int)Primary_Index])
         {
            end = i-1;
            break;
         }
      }

      for(i=start; i<=(start+(end-start)/2); i++)
         Hash_table.Table_Block_Offset[i] = Hash_table.Table_Block_Offset[Primary_Index];

      for(i=(start+(end-start)/2)+1; i<=end; i++)
         Hash_table.Table_Block_Offset[i] = New_Block_Offset;

      unsigned Hash_primary_index;
      char temp2[30]="";
      string id="";

      for(int j=0; j<127; j++)
      {
         unsigned t=Buffer->Record[j].ID;
         sprintf(temp2, "%d",t);
         id = temp2;
         Hash_primary_index = HASH(id);
         Hash_primary_index = Hash_primary_index >> (32-Table_Bit_Number);
         if((int)Hash_primary_index < (start+(end-start)/2)+1)
            Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
         else
         {
            New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];
         }
      }

      Dat_File.seekp(Hash_table.Table_Block_Offset[start], ios::beg);
      Dat_File.write((char*)Old_Block, sizeof(Block));
      Dat_File.seekp(New_Block_Offset, ios::beg);
      Dat_File.write((char*)New_Block, sizeof(Block));

      Hash_File.seekp(0, ios::beg);
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));

      delete Buffer;
      delete New_Block;
      delete Old_Block;
   }

}

void Dynamic_Hash::Block_Full_p(string stu_id, int Block_Bit_Number, fstream& Dat_File)
{
   unsigned Primary_Index = HASH(stu_id);
   if(Table_Bit_Number != 0)
      Primary_Index = Primary_Index >> (32-Table_Bit_Number);
   else
      Primary_Index = 0;

   if(Block_Bit_Number == Table_Bit_Number)
   {

      Expand_Table_p(Primary_Index, Dat_File);
      return;
   }
   else if(Block_Bit_Number < Table_Bit_Number)
   {

      Block_p* Buffer = new Block_p;
      Dat_File.seekp(0, ios::end);
      std::streamoff a = Dat_File.tellg();
      long New_Block_Offset = a;
      Block_p* New_Block = new Block_p;
      Block_p* Old_Block = new Block_p;

      Dat_File.seekg(Hash_table.Table_Block_Offset[(int)Primary_Index], ios::beg);
      Dat_File.read((char*)Buffer, sizeof(Block_p));
      Old_Block->Bit_Number = Buffer->Bit_Number + 1;
      New_Block->Bit_Number = Buffer->Bit_Number + 1;

      int start = -1, end = -1;
      int i;
      for(i=0; i<TABLESIZE; i++)
      {
         if(start == -1 && Hash_table.Table_Block_Offset[i] == Hash_table.Table_Block_Offset[(int)Primary_Index])
            start = i;
         if(start != -1 && Hash_table.Table_Block_Offset[i] != Hash_table.Table_Block_Offset[(int)Primary_Index])
         {
            end = i-1;
            break;
         }
      }

      for(i=start; i<=(start+(end-start)/2); i++)
         Hash_table.Table_Block_Offset[i] = Hash_table.Table_Block_Offset[Primary_Index];

      for(i=(start+(end-start)/2)+1; i<=end; i++)
         Hash_table.Table_Block_Offset[i] = New_Block_Offset;

      unsigned Hash_primary_index;
      char temp2[30]="";
      string id="";

      for(int j=0; j<127; j++)
      {
         unsigned t=Buffer->Record[j].ProfID;
         sprintf(temp2, "%d",t);
         id = temp2;
         Hash_primary_index = HASH(id);
         Hash_primary_index = Hash_primary_index >> (32-Table_Bit_Number);
         if((int)Hash_primary_index < (start+(end-start)/2)+1)
            Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
         else
         {
            New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];
         }
      }

      Dat_File.seekp(Hash_table.Table_Block_Offset[start], ios::beg);
      Dat_File.write((char*)Old_Block, sizeof(Block_p));
      Dat_File.seekp(New_Block_Offset, ios::beg);
      Dat_File.write((char*)New_Block, sizeof(Block_p));

      Hash_File.seekp(0, ios::beg);
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));

      delete Buffer;
      delete New_Block;
      delete Old_Block;
   }

}
void Dynamic_Hash::Expand_Table(unsigned Primary_Index, fstream& Dat_File)
{
   Block* Buffer = new Block;
    int i = 0;
   Dat_File.seekp(0, ios::end);
   std::streamoff b = Dat_File.tellg();
   long New_Block_Offset = b;
   Block* New_Block = new Block;
   Block* Old_Block = new Block;
   unsigned Table_Block_Number = 2;

   if(Table_Bit_Number != 0)
      Table_Block_Number = Table_Block_Number << (Table_Bit_Number-1);
   else
      Table_Block_Number = 1;


   for(int i=Table_Block_Number-1; i>=0; i--)
   {
      if(i == (int)Primary_Index)
      {
         Hash_table.Table_Block_Offset[2*i]   = Hash_table.Table_Block_Offset[i];
         Hash_table.Table_Block_Offset[2*i+1] = New_Block_Offset;


         Dat_File.seekg(Hash_table.Table_Block_Offset[i], ios::beg);
         Dat_File.read((char*)Buffer, sizeof(Block));

         Table_Bit_Number++;
         Old_Block->Bit_Number = Table_Bit_Number;
         New_Block->Bit_Number = Table_Bit_Number;

         unsigned Hash_primary_index;
         char temp3[30]="";
         string id="";
         unsigned j;
         for(j=0; j<127; j++)
         {

            unsigned t= Buffer->Record[j].ID;
            sprintf(temp3, "%d",t);
            id = temp3;
            Hash_primary_index = HASH(id);
            Hash_primary_index = Hash_primary_index >> (32-Table_Bit_Number);
            if(Hash_primary_index == 2*i)
               Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
            else
            {
               New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];

            }
         }
         Dat_File.seekp(Hash_table.Table_Block_Offset[2*i], ios::beg);
         Dat_File.write((char*)Old_Block, sizeof(Block));
         Dat_File.seekp(New_Block_Offset, ios::beg);
         Dat_File.write((char*)New_Block, sizeof(Block));
      }
      else
      {
         Hash_table.Table_Block_Offset[2*i]   = Hash_table.Table_Block_Offset[i];
         Hash_table.Table_Block_Offset[2*i+1] = Hash_table.Table_Block_Offset[i];
      }


   }
      Hash_File.seekp(0, ios::beg);
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
      delete Buffer;
      delete New_Block;
      delete Old_Block;
}
void Dynamic_Hash::Expand_Table_p(unsigned Primary_Index, fstream& Dat_File)
{
   Block_p* Buffer = new Block_p;
    int i = 0;
   Dat_File.seekp(0, ios::end);
   std::streamoff b = Dat_File.tellg();
   long New_Block_Offset = b;
   Block_p* New_Block = new Block_p;
   Block_p* Old_Block = new Block_p;
   unsigned Table_Block_Number = 2;

   if(Table_Bit_Number != 0)
      Table_Block_Number = Table_Block_Number << (Table_Bit_Number-1);
   else
      Table_Block_Number = 1;


   for(int i=Table_Block_Number-1; i>=0; i--)
   {
      if(i == (int)Primary_Index)
      {
         Hash_table.Table_Block_Offset[2*i]   = Hash_table.Table_Block_Offset[i];
         Hash_table.Table_Block_Offset[2*i+1] = New_Block_Offset;


         Dat_File.seekg(Hash_table.Table_Block_Offset[i], ios::beg);
         Dat_File.read((char*)Buffer, sizeof(Block_p));

         Table_Bit_Number++;
         Old_Block->Bit_Number = Table_Bit_Number;
         New_Block->Bit_Number = Table_Bit_Number;

         unsigned Hash_primary_index;
         char temp3[30]="";
         string id="";
         unsigned j;
         for(j=0; j<127; j++)
         {

            unsigned t= Buffer->Record[j].ProfID;
            sprintf(temp3, "%d",t);
            id = temp3;
            Hash_primary_index = HASH(id);
            Hash_primary_index = Hash_primary_index >> (32-Table_Bit_Number);
            if(Hash_primary_index == 2*i)
               Old_Block->Record[Old_Block->Record_Count++] = Buffer->Record[j];
            else
            {
               New_Block->Record[New_Block->Record_Count++] = Buffer->Record[j];

            }
         }
         Dat_File.seekp(Hash_table.Table_Block_Offset[2*i], ios::beg);
         Dat_File.write((char*)Old_Block, sizeof(Block_p));
         Dat_File.seekp(New_Block_Offset, ios::beg);
         Dat_File.write((char*)New_Block, sizeof(Block_p));
      }
      else
      {
         Hash_table.Table_Block_Offset[2*i]   = Hash_table.Table_Block_Offset[i];
         Hash_table.Table_Block_Offset[2*i+1] = Hash_table.Table_Block_Offset[i];
      }


   }
      Hash_File.seekp(0, ios::beg);
      Hash_File.write((char*)&Table_Bit_Number, sizeof(Table_Bit_Number));
      Hash_File.write((char*)&Hash_table, sizeof(Hash_table));
      delete Buffer;
      delete New_Block;
      delete Old_Block;
}
// new func
void Dynamic_Hash::Make_txt(fstream& Dat_File,char* filename){
    char Filename[80] = "";
   sprintf(Filename, "%s_DB.txt", filename);
    ofstream outfile(Filename);
    int total = 0;
    int DB_size = 0;

    Dat_File.seekg(0,ios::end);
    DB_size = Dat_File.tellg();
    Dat_File.seekg(0,ios::beg);
    if(table_type == 0){
         for(int i = 0; i< DB_size/BLOCKSIZE ; i++){
            Dat_File.read((char*)&Dat_block,sizeof(Dat_block));
            for(int j = 0; j< Dat_block.Record_Count; j++){
                total++;
                outfile << Dat_block.Record[j].Name << " " << Dat_block.Record[j].ID << " " << Dat_block.Record[j].Score << " "  << Dat_block.Record[j].advisorID <<" " << Dat_block.Record_Count << " total : " << total <<endl;
            }
            outfile << endl ;
        }
    }
    else{
         for(int i = 0; i< DB_size/BLOCKSIZE ; i++){
            Dat_File.read((char*)&Dat_block_p,sizeof(Dat_block_p));
            for(int j = 0; j< Dat_block_p.Record_Count; j++){
                total++;
                outfile << Dat_block_p.Record[j].Name << " " << Dat_block_p.Record[j].ProfID << " "  << Dat_block_p.Record[j].Salary <<" " << Dat_block_p.Record_Count << " total : " << total <<endl;
            }
            outfile << endl ;
        }
    }
    cout << endl;
    cout << "=========================================================="<< endl << endl;
    cout << " ■ Total Entered Record Num : "<< total << endl;
    cout << " ■ Total BLOCK Num IN Students.DB : "<< DB_size/4096  << endl;
    cout << endl;
    outfile.close();

}

class Score_Element{
public :
   long  Offset;
   float Key_Score;
   Score_Element();
};

Score_Element::Score_Element()
{
   Offset = -1;
   Key_Score  = -1;
}


class Node{
public :
   Score_Element Node_Element[BLOCKSIZE/sizeof(Score_Element)-2];
   long    Last_Offset;
   long    Next_Node_Offset;
   int     Score_Element_Count;
   int     Node_part;                                               // 1: Root Node 2: Internal Node 3: External Node. 4 : Root Node & External Node.
   Node();
};

Node::Node()
{
   Last_Offset = -1;
   Next_Node_Offset = -1;
   Score_Element_Count = 0;
   Node_part = 0;
}

class B_Plus_Tree{
private :
   int      Node_Count;             // Tree 의 총 노드 갯수
   long     Root_Node_Offset;       // Root node Offset
   long     Parent_Node_Offset;

   long     Current_Node_Offset;    // 현재 Node Offset
   bool     checkok;                //현재 Node의 Update 유무.
   int      Element_In_Node;        // 한 Node 안의 Element 수.
   long     Left_element;           // Split될 때 왼쪽   Node.
   long     Right_element;          // Split될 때 오른쪽 Node.

   Node* Current_Node;
   Node* Parent_Node;

   void Open_Tree(char* Filename);
   void GetNode(long s_Offset);
   int  FindInNode(Node* score_current_node, int find_score, bool& score_is_find);
   void Insert_Element(int score, long ID_offset, Node* score_current_node, int Insert_Index);
   void NoSplitInsert(int score,  long ID_offset, Node* score_current_node, int Insert_Index);
   void SplitInsert(int score, long ID_offset, Node* score_current_node, int Insert_Index);


public :
    fstream  Tree_File;
   B_Plus_Tree(char* Filename);
   ~B_Plus_Tree();
   long Find(int find_score, int& S_Index, bool& score_is_find);
   bool Insert(int score, long ID_offset);
   void Score_search(int lower, int uppper, int* Result);
   void check();
   void check_p();
   void check_j();
};

B_Plus_Tree::B_Plus_Tree(char* Filename)
{
   Current_Node = new Node;
   Current_Node_Offset = -1;
   checkok = false;
   Element_In_Node = BLOCKSIZE/sizeof(Score_Element)-2;
   Left_element = -1;
   Right_element   = -1;
   Open_Tree(Filename);

}

B_Plus_Tree::~B_Plus_Tree()
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



void B_Plus_Tree::Open_Tree(char* Filename)
{

   Tree_File.open(Filename, ios::in | ios::out | ios::binary);
   if(!Tree_File)
   {
      Tree_File.clear();
      Tree_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);


      Node* Root = new Node;
      Root->Node_part = 4;
      Tree_File.seekp(sizeof(long), ios::beg);
      std::streamoff c = Tree_File.tellg();
      Root_Node_Offset = c;
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
   std::streamoff tree = Tree_File.tellg();
   Node_Count = (tree-(long)4)/(sizeof(Node));
   Tree_File.seekg(0);

}


long B_Plus_Tree::Find(int find_score, int& S_Index, bool& score_is_find)
{
   score_is_find = false;
   S_Index = 0;
   long s_Node_Offset = Root_Node_Offset;
   Parent_Node_Offset = s_Node_Offset;
   int i=0;
   int sign = 0;

   while(1) // 3이나 4될떄까지
   {

      GetNode(s_Node_Offset);


      if(Current_Node->Node_part != 1 && Current_Node->Node_part != 2) // 3 이나 4 일떄
         break;
      else
      {

        Parent_Node_Offset = s_Node_Offset;

         for(i=0; i<Current_Node->Score_Element_Count; i++) // 찾음
         {
            if(find_score <= Current_Node->Node_Element[i].Key_Score)
            {
               s_Node_Offset = Current_Node->Node_Element[i].Offset;
               break;
            }
         }
         if(i == Current_Node->Score_Element_Count) // 못찾음
         {
            if(Current_Node->Score_Element_Count == Element_In_Node) // 꽉찻을때
               s_Node_Offset = Current_Node->Last_Offset;
            else
               s_Node_Offset = Current_Node->Node_Element[i].Offset; // 꽉 ㄴㄴ
         }
      }

   }

 // 나옴 주소 저장하고

   S_Index = FindInNode(Current_Node, find_score, score_is_find);  // 저장할 인덱스

   if(score_is_find == true)
      return Current_Node->Node_Element[S_Index].Offset; // 같은값 있을떄
   else
      return -1;  // 없을때
}



void B_Plus_Tree::GetNode(long s_Offset)
{
    // Current Node 가 맞을경우 바로 Return
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
    // Current Node 로 바꿔줌
   Current_Node_Offset = s_Offset;
   Tree_File.seekg(Current_Node_Offset, ios::beg);
   Tree_File.read((char*)Current_Node, sizeof(Node));
}
// 3이나 4   잎노드에서 저장할 위치찾기
int B_Plus_Tree::FindInNode(Node* score_current_node, int find_score, bool& score_is_find)
{
   for(int i=0; i<score_current_node->Score_Element_Count; i++)
   {
      if(score_current_node->Node_Element[i].Key_Score == find_score) // 찾음
      {
         score_is_find = true;
         return i;
      }
      if(find_score < score_current_node->Node_Element[i].Key_Score) // 기존보다 더작
      {
         score_is_find = false;
         return i;
      }
   }
   score_is_find = false;
   return score_current_node->Score_Element_Count; // 없으면 제일 마지막 위치
}

bool B_Plus_Tree::Insert(int score, long ID_offset)
{
   bool score_is_find = false;
   int Insert_Index = -1; // 잎노드에서 위치
   long overlap_off = 0;
   overlap_off = Find(score, Insert_Index, score_is_find);

// 저장할 위치 , 인덱스 찾음

   Insert_Element(score, ID_offset, Current_Node, Insert_Index);
   return true;
}

void B_Plus_Tree::Insert_Element(int score, long ID_offset, Node* score_current_node, int Insert_Index)
{
   if(score_current_node->Score_Element_Count != Element_In_Node)
      NoSplitInsert(score, ID_offset, score_current_node, Insert_Index);
   else
      SplitInsert(score, ID_offset, score_current_node, Insert_Index);
}

void B_Plus_Tree::NoSplitInsert(int score, long ID_offset, Node* score_current_node, int Insert_Index)
{
   if(score_current_node->Score_Element_Count <= 509)
      score_current_node->Node_Element[score_current_node->Score_Element_Count+1].Offset = score_current_node->Node_Element[score_current_node->Score_Element_Count].Offset;
   else
      score_current_node->Last_Offset = score_current_node->Node_Element[score_current_node->Score_Element_Count].Offset;

   for(int j=score_current_node->Score_Element_Count; j>Insert_Index; j--)
   {
      score_current_node->Node_Element[j].Key_Score    = score_current_node->Node_Element[j-1].Key_Score;
      score_current_node->Node_Element[j].Offset = score_current_node->Node_Element[j-1].Offset;
   }

   score_current_node->Node_Element[Insert_Index].Key_Score = score;
   score_current_node->Node_Element[Insert_Index].Offset = ID_offset;
   score_current_node->Score_Element_Count++;

   checkok = true;
   if(Right_element != -1 || Left_element != -1)
   {
      score_current_node->Node_Element[Insert_Index].Offset = Left_element;
      score_current_node->Node_Element[Insert_Index+1].Offset = Right_element;
      Right_element = -1;
      Left_element = -1;
   }
}

void B_Plus_Tree::SplitInsert(int score, long ID_offset, Node* score_current_node, int Insert_Index)
{
    int s, t,i;
   Node* New_Node = new Node;
   Tree_File.seekg(0, ios::end);
   long  New_Node_Offset = Tree_File.tellg();

   if(score_current_node->Node_part == 1 || score_current_node->Node_part == 2){
      New_Node->Node_part = 2;
      New_Node->Next_Node_Offset = score_current_node->Next_Node_Offset;
      score_current_node->Next_Node_Offset = New_Node_Offset;
   }
   else if(score_current_node->Node_part == 3 || score_current_node->Node_part == 4)
   {
      New_Node->Node_part = 3;
      New_Node->Next_Node_Offset = score_current_node->Next_Node_Offset;
      score_current_node->Next_Node_Offset = New_Node_Offset;
   }
   Tree_File.seekp(0, ios::end);
   if(Tree_File.tellp() == -1){
        Tree_File.close();
        Tree_File.open("Students_score.idx", ios::in | ios::out | ios::binary);
        Tree_File.seekp(0, ios::end);
        New_Node_Offset = Tree_File.tellp();
   }
   else
        New_Node_Offset = Tree_File.tellp();
   Tree_File.write((char*)New_Node, sizeof(Node));
   Node_Count++;


   for(s=Element_In_Node/2, t=0; s<Element_In_Node; s++, t++)
   {
      New_Node->Node_Element[t].Key_Score    = score_current_node->Node_Element[s].Key_Score;
      New_Node->Node_Element[t].Offset = score_current_node->Node_Element[s].Offset;

      score_current_node->Node_Element[s].Key_Score    = -1;
      score_current_node->Node_Element[s].Offset = -1;
   }

   checkok = true;

   score_current_node->Score_Element_Count = Element_In_Node/2;
   New_Node->Score_Element_Count = Element_In_Node - Element_In_Node/2;


   New_Node->Node_Element[t].Offset = score_current_node->Last_Offset;
   score_current_node->Last_Offset = -1;

// 새로운 거
   if(Insert_Index < Element_In_Node/2)
   {
      NoSplitInsert(score, ID_offset, score_current_node, Insert_Index);
   }
   else
   {
      NoSplitInsert(score, ID_offset, New_Node, (Insert_Index - Element_In_Node/2));
   }


   Tree_File.seekp(New_Node_Offset, ios::beg);
   Tree_File.write((char*)New_Node, sizeof(Node));

   delete New_Node;

   Left_element = Current_Node_Offset;
   Right_element   = New_Node_Offset;
// 나누기 완료

   // 여기서 부터 조지자 여긴 인덱스 넣는거
   float  s_Parent_Insert_Score    = score_current_node->Node_Element[score_current_node->Score_Element_Count-1].Key_Score;

    // 부모노드 현재 노드 값 다암.

    if(score_current_node->Node_part == 1 || score_current_node->Node_part == 4){

          cout << score <<"       @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<< endl;
          Node* New_Root_Node = new Node;
          Tree_File.seekp( 0, ios::end );
          Root_Node_Offset = Tree_File.tellp();

          New_Root_Node->Node_Element[0].Key_Score = s_Parent_Insert_Score;
          New_Root_Node->Node_Element[0].Offset = Left_element;
          New_Root_Node->Node_Element[1].Key_Score = 999;
          New_Root_Node->Node_Element[1].Offset = Right_element;
          cout << Left_element << "          " << Right_element << endl;

          Right_element = -1;
          Left_element = -1;
          New_Root_Node->Node_part = 1;
          if(Current_Node->Node_part == 1){
            Current_Node->Node_part = 2;
            Current_Node->Next_Node_Offset  = New_Root_Node->Node_Element[1].Offset;
          }
          else{
            Current_Node->Node_part = 3;
            Current_Node->Next_Node_Offset  = New_Root_Node->Node_Element[1].Offset;
          }
          New_Root_Node->Score_Element_Count = 1;
          Node_Count++;

          Tree_File.write((char*)New_Root_Node, sizeof(Node));
          Tree_File.seekp(0, ios::beg);
          Tree_File.write((char*)&Root_Node_Offset, sizeof(long));

          delete New_Root_Node;
    }
    else {
        if(score_current_node->Node_part == 2){
            GetNode(Root_Node_Offset);
            bool score_is_find = false;
            int S_Index = FindInNode(Current_Node, s_Parent_Insert_Score, score_is_find);
            Insert_Element(s_Parent_Insert_Score, Left_element, Current_Node, S_Index);
        }
        else{
            GetNode(Parent_Node_Offset);
            bool score_is_find = false;
            int S_Index = FindInNode(Current_Node, s_Parent_Insert_Score, score_is_find);
            Insert_Element(s_Parent_Insert_Score, Left_element, Current_Node, S_Index);
        }
    }


}

void B_Plus_Tree::Score_search(int lower, int uppper, int* Result)
{
   int S_Index = 0;
   int Result_Count = 0;
   bool score_is_find;
   Find(lower, S_Index, score_is_find);
   int i = S_Index;

   while(1)
   {

      if(i == (Current_Node->Score_Element_Count-1) && Current_Node->Node_Element[i].Key_Score <=  uppper)
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

      if(lower <= Current_Node->Node_Element[i].Key_Score && Current_Node->Node_Element[i].Key_Score <= uppper)
         Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
      if(uppper < Current_Node->Node_Element[i].Key_Score)
         return;
      i++;
   }
   return;
}
void B_Plus_Tree::check(){

        ifstream test("Students_score.idx",ios::binary);
        ofstream btree("Students_tree.idx");

    Node plate;
    test.seekg(4,ios::beg);
    btree << " Root : " << Root_Node_Offset << endl;
    btree <<" pos : " << test.tellg() << endl;
    for(int i =0 ; i<Node_Count; i++){
        test.read((char*)&plate, sizeof(plate));
        btree << "------------------------  "<< i << endl;
        for(int j = 0; j<plate.Score_Element_Count;j++)
            btree<<"( " << plate.Node_Element[j].Key_Score << " , "<< plate.Node_Element[j].Offset << " ) ";
        btree<< test.tellg()-4096 << " next Node :  " << plate.Next_Node_Offset << " Mode : " << plate.Node_part<< endl;
        if(plate.Node_part==1){
            Root_Node_Offset = test.tellg() - 4096 ;
        }
    }
    cout << Root_Node_Offset << endl;
    test.close();
}
void B_Plus_Tree::check_j(){

        ifstream test("Students_ID.idx",ios::binary);
        ofstream btree("Students_ID_tree.idx");

    Node plate;
    test.seekg(4,ios::beg);
    btree << " Root : " << Root_Node_Offset << endl;
    btree <<" pos : " << test.tellg() << endl;
    for(int i =0 ; i<Node_Count; i++){
        test.read((char*)&plate, sizeof(plate));
        btree << "------------------------  "<< i << endl;
        for(int j = 0; j<plate.Score_Element_Count;j++)
            btree<<"( " << plate.Node_Element[j].Key_Score << " , "<< plate.Node_Element[j].Offset << " ) ";
        btree<< test.tellg()-4096 << " next Node :  " << plate.Next_Node_Offset << " Mode : " << plate.Node_part<< endl;
        if(plate.Node_part==1){
            Root_Node_Offset = test.tellg() - 4096 ;
        }
    }
    cout << Root_Node_Offset << endl;
    test.close();
}
void B_Plus_Tree::check_p(){

        ifstream test("Prof_score.idx",ios::binary);
        ofstream btree("Prof_tree.idx");

    Node plate;
    test.seekg(4,ios::beg);
    btree << " Root : " << Root_Node_Offset << endl;
    btree <<" pos : " << test.tellg() << endl;
    for(int i =0 ; i<Node_Count; i++){
        test.read((char*)&plate, sizeof(plate));
        btree << "------------------------  "<< i << endl;
        for(int j = 0; j<plate.Score_Element_Count;j++)
            btree<<"( " << plate.Node_Element[j].Key_Score << " , "<< plate.Node_Element[j].Offset << " ) ";
        btree<< test.tellg()-4096 << " next Node :  " << plate.Next_Node_Offset << " Mode : " << plate.Node_part<< endl;
        if(plate.Node_part==1){
            Root_Node_Offset = test.tellg() - 4096 ;
        }
    }
    cout << Root_Node_Offset << endl;
    test.close();
}



bool openDB(char* filename,Dynamic_Hash** Hash,fstream& Dat_File,B_Plus_Tree** Tree){

   char Filename[80] = "";
   sprintf(Filename, "%s.DB", filename);
   remove(Filename);
   Dat_File.open(Filename, ios::in | ios::out | ios::binary);

   if(!Dat_File)
   {
      Dat_File.clear();
      Dat_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
      Dat_File.seekp(0, ios::beg);
      if(table_type == 0){
          Block Dat_block1;
          Dat_File.write((char*)&Dat_block1, sizeof(Block));
      }
      else{
          Block_p Dat_block1;
          Dat_File.write((char*)&Dat_block1, sizeof(Block_p));
      }
   }
   strcpy(Filename,"");
   sprintf(Filename, "%s.hash", filename);
   remove(Filename);
   *Hash = new Dynamic_Hash(Filename);


   strcpy(Filename,"");
   sprintf(Filename, "%s_score.idx",filename);
   remove(Filename);
   *Tree = new B_Plus_Tree(Filename);

   return true;
}

unsigned insertRecord(char* name, unsigned ID, float score, unsigned advisorID,Dynamic_Hash* Hash,fstream& Dat_File,B_Plus_Tree* Tree,B_Plus_Tree* Tree_j)
{
   char* temp = new char[10];
   char* temp1 = new char[10];
   string id="";
   bool Is_Insert = false;

   unsigned blockNumber = 0;

   sprintf(temp, "%d",ID);
   id=temp;

   int Dat_File_Offset = Hash->Get_Hash_Offset(id);


   Dat_File.seekg(Dat_File_Offset, ios::beg);
   Dat_File.read((char*)&Dat_block, sizeof(Block));

   if(Dat_block.Record_Count < BLOCK_MAX)
      {
         strcpy(Dat_block.Record[Dat_block.Record_Count].Name, name);
         Dat_block.Record[Dat_block.Record_Count].ID = ID;
         Dat_block.Record[Dat_block.Record_Count].Score = score;
         Dat_block.Record[Dat_block.Record_Count].advisorID = advisorID;

         Dat_block.Record_Count++;

         Dat_File.seekp(Dat_File_Offset, ios::beg);
         Dat_File.write((char*)&Dat_block, sizeof(Block));
         Is_Insert = true;

      }
      else
      {
          if(table_type == 0)
            Hash->Block_Full(id, Dat_block.Bit_Number, Dat_File);
          else
            Hash->Block_Full_p(id, Dat_block.Bit_Number, Dat_File);

         insertRecord(name,ID,score,advisorID,Hash,Dat_File,Tree,Tree_j);
      }

      if(Is_Insert == true)
      {
         Tree_j->Insert(advisorID,(long)ID);
         Tree->Insert(score*100000,(long)ID);
      }


   blockNumber=Dat_File_Offset;
   delete temp;
   delete temp1;
   return blockNumber;
}
unsigned insertRecord_p(char* name, unsigned ID, int salary ,Dynamic_Hash* Hash,fstream& Dat_File,B_Plus_Tree* Tree)
{
   char* temp = new char[10];
   char* temp1 = new char[10];
   string id="";
   bool Is_Insert = false;

   unsigned blockNumber = 0;

   sprintf(temp, "%d",ID);
   id=temp;

   int Dat_File_Offset = Hash->Get_Hash_Offset(id);


   Dat_File.seekg(Dat_File_Offset, ios::beg);
   Dat_File.read((char*)&Dat_block_p, sizeof(Block_p));

   if(Dat_block_p.Record_Count < BLOCK_MAX)
      {
         strcpy(Dat_block_p.Record[Dat_block_p.Record_Count].Name, name);
         Dat_block_p.Record[Dat_block_p.Record_Count].ProfID = ID;
         Dat_block_p.Record[Dat_block_p.Record_Count].Salary = salary;

         Dat_block_p.Record_Count++;

         Dat_File.seekp(Dat_File_Offset, ios::beg);
         Dat_File.write((char*)&Dat_block_p, sizeof(Block_p));
         Is_Insert = true;

      }
      else
      {
          if(table_type == 0)
            Hash->Block_Full(id, Dat_block_p.Bit_Number, Dat_File);
          else
            Hash->Block_Full_p(id, Dat_block_p.Bit_Number, Dat_File);

         insertRecord_p(name,ID,salary,Hash,Dat_File,Tree);
      }

      if(Is_Insert == true)
      {
         Tree->Insert(salary,(long)ID);
      }


   blockNumber=Dat_File_Offset;
   delete temp;
   delete temp1;
   return blockNumber;
}
// exact mactch query
unsigned searchID(unsigned ID,fstream& Dat_File,Dynamic_Hash* Hash){

   char* temp = new char[10];
   string id="";

   unsigned blockNumber = 0;

   sprintf(temp, "%d",ID);
   id=temp;

   int Dat_File_Offset = Hash->Get_Hash_Offset(id);

   if(table_type == 0){
       Dat_File.seekg(Dat_File_Offset, ios::beg);
       Dat_File.read((char*)&Dat_block, sizeof(Dat_block_p));
   }
   else{
       Dat_File.seekg(Dat_File_Offset, ios::beg);
       Dat_File.read((char*)&Dat_block_p, sizeof(Dat_block_p));
   }
   blockNumber=Dat_File_Offset;

   delete temp;

   return blockNumber;
}
void get_info(Dynamic_Hash* Hash,fstream& Dat_File,char* csvname,B_Plus_Tree* Tree,B_Plus_Tree* Tree_join){
    char Filename[80] = "";
   sprintf(Filename, "%s.csv", csvname);

    ifstream fin(Filename, ios::in);


    string line;

    if(table_type == 0){
        int total_num;
        char name[20]= "";
        unsigned studentID = 0;
        float score = 0;
        unsigned advisorID = 0;
        int  total = 0;


        int input_num = 0;
        int sign = 0;
            while(getline(fin,line))
            {
                stringstream  lineStream(line);
                string        cell= "";

                int a = 0;
                if(sign == 0){
                    getline(lineStream,cell,',');
                    total_num = str2int(cell);
                    for(int i =0 ; i<3; i++){
                        getline(lineStream,cell,',');
                    }
                    sign++;
                }

                else if(sign > 0){
                    getline(lineStream,cell,',');
                    if(cell.length() > 19){
                        cell = cell.substr(0,19);

                    }
                    strcpy(name,cell.c_str());
                    getline(lineStream,cell,',');
                    studentID = str2unsign(cell);
                    getline(lineStream,cell,',');
                    score = str2float(cell);
                    getline(lineStream,cell,',');
                    advisorID = str2unsign(cell);

                    total ++;
                   // cout << total << " ";
                    // insertRecord
                    unsigned blockNumber = insertRecord(name, studentID, score, advisorID,Hash,Dat_File,Tree,Tree_join);


                }
            }
    }
    else{
         int total_num;
        char name[20]= "";
        unsigned profID = 0;
        int salary = 0;
        int  total = 0;


        int input_num = 0;
        int sign = 0;
            while(getline(fin,line))
            {
                stringstream  lineStream(line);
                string        cell= "";

                int a = 0;
                if(sign == 0){
                    getline(lineStream,cell,',');
                    total_num = str2int(cell);
                    for(int i =0 ; i<2; i++){
                        getline(lineStream,cell,',');
                    }
                    sign++;
                }

                else if(sign > 0){
                    getline(lineStream,cell,',');
                    if(cell.length() > 19){
                        cell = cell.substr(0,19);

                    }
                    strcpy(name,cell.c_str());
                    getline(lineStream,cell,',');
                    profID = str2unsign(cell);
                    getline(lineStream,cell,',');
                    salary = str2int(cell);

                    total ++;

                    // insertRecord
                    unsigned blockNumber = insertRecord_p(name, profID,salary,Hash,Dat_File,Tree);


                }
            }
    }
    fin.close();
}
unsigned searchScore(float lower, float upper,Dynamic_Hash* Hash,fstream& Dat_File,B_Plus_Tree* Tree){
   unsigned numOfScore = 0;
   int Result[500000];
   unsigned Dat_File_Offset;
   int i;
   for(i=0; i<500000; i++)
      Result[i] = -1;

   Tree->Score_search((int)(lower*100000), (int)(upper*100000), Result);

   int Total_Count = 0;

   char temp[80] = "";
   string id   = "";
   for(i=0; i<500000; i++)
   {
      if(Result[i] != -1)
      {
         sprintf(temp, "%d", Result[i]);
         id = temp;
         Dat_File_Offset = Hash->Get_Hash_Offset(id);
         Dat_File.seekg(Dat_File_Offset, ios::beg);
         Dat_File.read((char*)&Dat_block, sizeof(Block));

         for(int j=0; j<Dat_block.Record_Count; j++)
            if(Result[i] == Dat_block.Record[j].ID)
            {
                query_out << Dat_block.Record[j].ID << "  " << Dat_block.Record[j].Name << "        "<< Dat_block.Record[j].Score << "       " << Dat_block.Record[j].advisorID << endl;
               Total_Count++;
               break;
            }
      }
      else
         break;
   }
   numOfScore=Total_Count;
   query_out << "---------------------------------------------------------------------------------" << endl;

   return numOfScore;
}
unsigned searchSalary(int lower, int upper,Dynamic_Hash* Hash,fstream& Dat_File,B_Plus_Tree* Tree){
   unsigned numOfScore = 0;
   int Result[500000];
   unsigned Dat_File_Offset;
   int i;
   for(i=0; i<500000; i++)
      Result[i] = -1;

   Tree->Score_search(lower, upper, Result);

   int Total_Count = 0;

   char temp[80] = "";
   string id   = "";
   for(i=0; i<500000; i++)
   {
      if(Result[i] != -1)
      {
         sprintf(temp, "%d", Result[i]);
         id = temp;
         Dat_File_Offset = Hash->Get_Hash_Offset(id);
         Dat_File.seekg(Dat_File_Offset, ios::beg);
         Dat_File.read((char*)&Dat_block_p, sizeof(Dat_block_p));

         for(int j=0; j<Dat_block_p.Record_Count; j++)
            if(Result[i] == Dat_block_p.Record[j].ProfID)
            {
                query_out << Dat_block_p.Record[j].ProfID << "  " << Dat_block_p.Record[j].Name << "        "<< Dat_block_p.Record[j].Salary <<   endl;
               Total_Count++;
               break;
            }
      }
      else
         break;
   }
   numOfScore=Total_Count;
   query_out << "---------------------------------------------------------------------------------" << endl;

   return numOfScore;
}
unsigned searchProfID(int lower, int upper,Dynamic_Hash* Hash,fstream& Dat_File,B_Plus_Tree* Tree,int index){
   unsigned numOfScore = 0;
   int Result[500000];
   unsigned Dat_File_Offset;
   int i;
   for(i=0; i<500000; i++)
      Result[i] = -1;

   Tree->Score_search(lower, upper, Result);

   int Total_Count = 0;

   char temp[80] = "";
   string id   = "";
   for(i=0; i<500000; i++)
   {
      if(Result[i] != -1)
      {
         sprintf(temp, "%d", Result[i]);
         id = temp;
         Dat_File_Offset = Hash->Get_Hash_Offset(id);
         Dat_File.seekg(Dat_File_Offset, ios::beg);
         Dat_File.read((char*)&Dat_block, sizeof(Block));

         for(int j=0; j<Dat_block.Record_Count; j++)
            if(Result[i] == Dat_block.Record[j].ID)
            {
                query_out <<Dat_block_p.Record[index].ProfID<< "  " <<Dat_block_p.Record[index].Name<< "  " <<Dat_block_p.Record[index].Salary<< "  " ;
                query_out << Dat_block.Record[j].ID << "  " << Dat_block.Record[j].Name << "        "<< Dat_block.Record[j].Score << "       " << Dat_block.Record[j].advisorID << endl;
               Total_Count++;
               break;
            }
      }
      else
         break;
   }
   numOfScore=Total_Count;
   query_out << "---------------------------------------------------------------------------------" << endl;

   return numOfScore;
}
// main
int main()
{
    table_type = 0;
    Dynamic_Hash* Hash1;
    B_Plus_Tree* Tree1;
    fstream Dat_File1;
    table_type = 1;
    Dynamic_Hash* Hash2;
    B_Plus_Tree* Tree2;

    B_Plus_Tree* Tree_join;
    Tree_join = new B_Plus_Tree("Students_ID.idx");

    fstream Dat_File2;

    table_type = 0;

    string choice_query = "";

    openDB("Students",&Hash1,Dat_File1,&Tree1);
   get_info(Hash1,Dat_File1,"student_data",Tree1,Tree_join);

    cout << " one - Complete" << endl;
    Hash1->Make_txt(Dat_File1,"Students");
 //   Hash1->Print_Hash();
    Tree1->check();
    Tree_join->check_j();

    table_type = 1;
    openDB("Prof",&Hash2,Dat_File2,&Tree2);

    get_info(Hash2,Dat_File2,"prof_data",Tree2,Tree_join);

    Hash2->Make_txt(Dat_File2,"Prof");
  //  Hash2->Print_Hash();
    Tree2->check_p();

    ifstream query_file("query.csv");

    string line;

    int total_num;
    string query_name= "";
    string Table_name = "";
    string Table_name2 = "";
    string key_name = "";
    unsigned query_ID = 0;
    float low_score = 0.0;
    float high_score = 0.0;
     float low_salary = 0;
    float high_salary = 0;
    int sign = 0;

    while(getline(query_file,line))
    {
        stringstream  lineStream(line);
        string        cell= "";
        if(sign == 0){
            getline(lineStream,cell,',');
            total_num = str2int(cell);
            for(int i =0 ; i<4; i++){
                getline(lineStream,cell,',');
            }
            sign++;
        }
        else if(sign > 0){
            getline(lineStream,cell,',');
            query_name = cell;

            if(query_name == "Search-Exact"){
                query_out.seekp(0,ios::end);
                getline(lineStream,cell,',');
                Table_name = cell;
                getline(lineStream,cell,',');
                key_name = cell;
                getline(lineStream,cell,',');
                query_ID = str2unsign(cell);
                getline(lineStream,cell,',');
                if(Table_name == "Students"){
                    table_type = 0;
                    searchID(query_ID,Dat_File1,Hash1);
                    for(int i = 0 ; i < Dat_block.Record_Count; i++){
                        if(Dat_block.Record[i].ID == query_ID){
                            query_out << Dat_block.Record[i].ID << " " << Dat_block.Record[i].Name << " " << Dat_block.Record[i].Score << " " << Dat_block.Record[i].advisorID << endl;
                        }
                    }
                    query_out << "---------------------------------------------------------------------------------" << endl;
                }
                else{
                    table_type = 1;
                     searchID(query_ID,Dat_File2,Hash2);
                     for(int i = 0 ; i < Dat_block_p.Record_Count; i++){
                        if(Dat_block_p.Record[i].ProfID == query_ID){
                            query_out << Dat_block_p.Record[i].ProfID << " " << Dat_block_p.Record[i].Name << " " << Dat_block_p.Record[i].Salary << endl;
                        }
                    }
                    query_out << "---------------------------------------------------------------------------------" << endl;
                }
            }
            else if ( query_name == "Search-Range"){
                getline(lineStream,cell,',');
                Table_name = cell;
                // 함수 적기
                if(Table_name == "Students"){
                    getline(lineStream,cell,',');
                    key_name = cell;
                    getline(lineStream,cell,',');
                    low_score = str2float(cell);
                    getline(lineStream,cell,',');
                    high_score = str2float(cell);
                    searchScore(low_score,high_score,Hash1,Dat_File1,Tree1);
                }
                else{
                    getline(lineStream,cell,',');
                    key_name = cell;
                    getline(lineStream,cell,',');
                    low_salary = str2int(cell);
                    getline(lineStream,cell,',');
                    high_salary = str2int(cell);
                    searchSalary(low_salary,high_salary,Hash2,Dat_File2,Tree2);
                }
            }
            else if ( query_name == "Join"){
                getline(lineStream,cell,',');
                Table_name = cell;
                getline(lineStream,cell,',');
                Table_name2 = cell;
                getline(lineStream,cell,',');
                getline(lineStream,cell,',');

                Dat_File2.seekg(0,ios::end);
                int DB_size  = Dat_File2.tellg();
                Dat_File2.seekg(0,ios::beg);//교수
                for(int i = 0 ; i < DB_size/BLOCKSIZE ; i ++){
                    Dat_File2.read((char*)&Dat_block_p, sizeof(Dat_block_p));
                    for(int j = 0 ; j < Dat_block_p.Record_Count; j++){ // 교수 블락 1번 루프
                        searchProfID(Dat_block_p.Record[j].ProfID,Dat_block_p.Record[j].ProfID,Hash1,Dat_File1,Tree_join,j);
                    }
                    cout << i << " " << DB_size/BLOCKSIZE << endl;
                }
            }
            else
                return 0;

        }

    }


   return 0;
}
