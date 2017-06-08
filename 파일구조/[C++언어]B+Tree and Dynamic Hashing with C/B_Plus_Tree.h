#include <iostream>
#include <fstream>
#include <string>

#define BLOCKSIZE 4096                                               // Block Size = 4K.
#pragma pack(1)

using namespace std;

// ************************************************************************************************
// ************************* Element Class : Node�� ���ԵǾ����� Element. *************************
// ************************************************************************************************ 
template <class Type>
class Element{
public : 
	long  Offset;													 // External Node�� ��� DB File���� Offset.(ID�� ����Ѵ�.) 
																	 // Internal Node, Root Node�� ���� ���� Node�� ����Ų��.
	                                                                 // �� �� Point�Ǵ� Node�� �� Element�� Key���� ���� ������ ���� Node.  
	Type  Key;                                                       // Key(Value).

	Element();
};

// Constructor : Offset�� Key�� -1�� ��Ÿ���� Invalid�ϴ�.
template <class Type>
Element<Type>::Element()
{
	Offset = -1;
	Key    = (Type)-1;
}

// ************************************************************************************************
// **************************** Node Class : B+Tree������ �ϳ��� Node. ****************************
// ************************************************************************************************ 
template <class Type>
class Node{
public :
	Element<Type> Node_Element[BLOCKSIZE/sizeof(Element<Type>)-2];   // �ϳ��� Node�� ���� Elements.(510)
	long    Last_Offset;                                             // �ϳ��� Node���� Last Fan-Out.
	long    Next_Node_Offset;                                        // External Node�� ���. ���� Node�� Offset. -1�� ��� Invalid.
	int     Element_Count;                                           // Node���� ä���� Element ����.
	int     Node_Mode;                                               // 1: Root Node 2: Internal Node 3: External Node. 4 : Root Node & External Node.

	Node();                                                                                                     
};

// Constructor.
template <class Type>
Node<Type>::Node()                                                         
{	
	Last_Offset = -1;
	Next_Node_Offset = -1;
	Element_Count = 0;
	Node_Mode = 0;	             
}

// ************************************************************************************************
// ********************************** B_Plus_Tree Class : B+Tree **********************************
// ************************************************************************************************ 
template <class Type>
class B_Plus_Tree{
private :
	int      Node_Count_In_File;                                     // Tree�� Node�� ����.
	long     Root_Node_Offset;                                       // Root Node Offset.
	Node<Type>* Current_Node;                                        // ���� Node.
	long     Current_Node_Offset;                                    // ���� Node�� ����� File�� Offset.
	bool     Dirty;                                                  // ���� Node�� Update ����.
	int      Element_In_Node;                                        // �� Node ���� Element ��.
	long     Small_Pointer;                                          // Split�� �� ����   Node.
	long     Big_Pointer;                                            // Split�� �� ������ Node.
	fstream  Tree_File;                                              // Tree�� ����� File.
	//ofstream  fout;/////
	void Init(char* Filename);                                       // �����ڿ��� Call�ϴ� Initialization Procedure.
	void GetNode(long m_Offset);				                     // Param�� Offset�� ��ġ�� Node�� Current_Node�� Set.
	int  FindInNode(Node<Type>* m_Current_Node, Type m_Find_Key, bool& m_Is_Find);       // m_Current_Node���� Key�� ã�´�.
	void Insert_Element(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index);
	void JustInsert(Type m_Inserted_Key,  long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index);
	void SplitInsert(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index);
	int  Get_Parent_Node(Type m_Inserted_Key, long m_Current_Node_Offset);               // Parent Node ��´�.

public :
	B_Plus_Tree(char* Filename);
	~B_Plus_Tree();
	long Find(Type m_Find_Key, int& m_Index, bool& m_Is_Find);       // Key ���� ������ Tree���� �����ϴ��� ���θ� Check.															 
	bool Insert(Type m_Inserted_Key, long m_Inserted_Offset);        // �ϳ��� Element�� Insert.
	bool Delete(Type m_Deleted_Key,  long m_Deleted_Offset);         // �ϳ��� Element�� Delete.
	bool Change_Block(Type m_Change_Key, long Old_Offset, long New_Offset);    // Hash Function�� ���� Split�� ��, Key�� ����� Block Offset�� ����.
	void Msearch(Type m_MSearch_Key, int* Result);
	void Rsearch(Type m_Small_Key, Type m_Big_Key, int* Result);
};

// Constructor. : Init.
template <class Type>
B_Plus_Tree<Type>::B_Plus_Tree(char* Filename)
{
	Current_Node = new Node<Type>;                                   // Current_Node Allocation.
	Current_Node_Offset = -1;
	Dirty = false;
	Element_In_Node = BLOCKSIZE/sizeof(Element<Type>)-2;             // ���⼭�� 510�̴�.
	Small_Pointer = -1;
	Big_Pointer   = -1;
	Init(Filename);
	//sprintf(Filename, "%s.txt", Filename);
	//fout.open(Filename);
}

// Destructor.
template <class Type>
B_Plus_Tree<Type>::~B_Plus_Tree()
{
	if(Dirty && Current_Node != NULL)
	{
		Tree_File.seekp(Current_Node_Offset, ios::beg);
		Tree_File.write((char*)Current_Node, sizeof(Node<Type>));
	}

	if(Current_Node != NULL)
		delete Current_Node;

	Tree_File.seekp(0, ios::beg);
	Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
	Tree_File.close();
}

// Initialization.
template <class Type>
void B_Plus_Tree<Type>::Init(char* Filename)
{
	// B+ Tree File�� Open�Ѵ�. �����ϴ� ��� File�κ��� Root_Node_Offset�� ��´�. 
	// File�� �������� ������ Root Node�� Create�Ѵ�.
	Tree_File.open(Filename, ios::in | ios::out | ios::binary);
	if(!Tree_File)                                                   // File Open�� ������ ���.
	{
		Tree_File.clear();                                           // Open�� ������ Error Bit�� ��� Clear.
		Tree_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
		
		// Root Node�� �����Ͽ� Tree_File�� Offset 4�� �����Ѵ�. Offset[0..4]�� Root Node�� ����Ű�� Pointer�̴�. 
		Node<Type>* Root = new Node<Type>;                                      
		Root->Node_Mode = 4;                                         // Root & External.
		Tree_File.seekp(sizeof(long), ios::beg);
		Root_Node_Offset = Tree_File.tellp();
		Tree_File.write((char*)Root, sizeof(Node<Type>));            // Root Node ����.
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
	Node_Count_In_File = (Tree_File.tellg()-(long)4)/(sizeof(Node<Type>));
}

// Find Function���� �־��� Key���� ������ B_Plus_Tree���� ã�´�.
// �̶� Range Search�� �����ϰ� �ϱ����Ͽ� ������ ���� �����ϴ� ��� �� ó���� Key�� ���� DB_FIle Offset�� ��ȯ�Ѵ�.
// �׷��Ƿ� �ڿ� ���������� �����ϴ� ������ Key Element���� ��� ���� �� �� �ִ�.
// 1st Param : Find�ϰԵ� Key. 2nd Param : ã�� Key���� Index or Insert�� Index ����. 3th Param : �����ϴ� ��� True, �ƴϸ� False.  
// Current_Node : Key�� �����ϴ� ��� �� Key�� �����ϴ� Node��, �ƴ� ��� �� key���� Insert�Ǿ��ϴ� Node�� �����ȴ�.
// Return Value : Key�� �����ϴ� ��� �� Key�� DB_file �� Block Offset�� Return, �ƴ� ��� -1 Return.  
template <class Type>
long B_Plus_Tree<Type>::Find(Type m_Find_Key, int& m_Index, bool& m_Is_Find)
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
template <class Type>
void B_Plus_Tree<Type>::GetNode(long m_Offset)
{
	if(Current_Node_Offset == m_Offset)                              // Current_Node�� ���. �׳� �ٷ� Return;
	{
		Dirty = true;
		return;
	}
	if(Dirty)                                                        // Current_Node�� ��ü�ϱ� ���� Dirty Bit�� Check.    
	{
		Tree_File.seekp(Current_Node_Offset, ios::beg);              // Current_Node�� ����� File�� Offset���� ����.
		Tree_File.write((char*)Current_Node, sizeof(Node<Type>));    // Current_Node Update.
		Dirty = false;                                               // Dirty Bit Release.
	}

	Current_Node_Offset = m_Offset;
	Tree_File.seekg(Current_Node_Offset, ios::beg);                  // Current_Node_Offset���� Read File Pointer Set. 
	Tree_File.read((char*)Current_Node, sizeof(Node<Type>));         // Current_Node�� �о�´�. 
}

// m_Current_Node���� m_Find_Key�� �����ϴ��� Check�Ͽ� �����Ѵٸ� m_Is_Find�� True�� Set�ϰ� Index�� Return, 
// �������� ���� ���� m_Is_Find�� False�� Set�ϰ� Insert�� Index�� Return.
template <class Type>
int B_Plus_Tree<Type>::FindInNode(Node<Type>* m_Current_Node, Type m_Find_Key, bool& m_Is_Find)
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
template <class Type>
bool B_Plus_Tree<Type>::Insert(Type m_Inserted_Key, long m_Inserted_Offset)
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

template <class Type>
void B_Plus_Tree<Type>::Insert_Element(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index)
{
	if(m_Current_Node->Element_Count != Element_In_Node)             // Case : Node�� ������ �ʾƼ� Split�� �ʿ����� ���� ���.
		JustInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);
	else															 // Case : Node�� ������ Split�� �ʿ��� ���.
		SplitInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);               
}

// Node�� ������ �ʾƼ� Split �ʿ���� Insert�� �Ǵ� ���.
template <class Type>
void B_Plus_Tree<Type>::JustInsert(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index)
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
template <class Type>
void B_Plus_Tree<Type>::SplitInsert(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index)
{
	int s, t;
	Node<Type>* New_Node = new Node<Type>;                           // ���ο� Node�� Create.
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
	Tree_File.write((char*)New_Node, sizeof(Node<Type>));
	Node_Count_In_File++;

	// m_Current_Node�� [0..N/2-1]������ �״�� ���ܵΰ�, [N/2..N]���� Element���� New_Node�� �̵���Ų��.
	for(s=Element_In_Node/2, t=0; s<Element_In_Node; s++, t++)
	{
		New_Node->Node_Element[t].Key    = m_Current_Node->Node_Element[s].Key;
		New_Node->Node_Element[t].Offset = m_Current_Node->Node_Element[s].Offset;

		m_Current_Node->Node_Element[s].Key    = (Type)-1;
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
	Tree_File.write((char*)New_Node, sizeof(Node<Type>));
	delete New_Node;

	// Parent_Node�� ���Ͽ� Big_Pointer, Small_Pointer�� �����صд�.
	
	Small_Pointer = Current_Node_Offset; 
	Big_Pointer   = New_Node_Offset;
	
	Type  m_Parent_Insert_Key    = m_Current_Node->Node_Element[m_Current_Node->Element_Count-1].Key;
	//long m_Parent_Insert_Offset = m_Current_Node->Node_Element[m_Current_Node->Element_Count-1].Offset;
	if(Get_Parent_Node(m_Parent_Insert_Key, Current_Node_Offset) != -1)
	{
		bool m_Is_Find = false;
		int m_Index = FindInNode(Current_Node, m_Parent_Insert_Key, m_Is_Find);
		
		for(int i=0; i<Current_Node->Element_Count; i++)
		{
			if(m_Parent_Insert_Key <= Current_Node->Node_Element[i].Key || Current_Node->Node_Element[i].Key == (Type)-1)                
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
		Tree_File.write((char*)New_Root_Node, sizeof(Node<Type>));
		Tree_File.seekp(0, ios::beg);
		Tree_File.write((char*)&Root_Node_Offset, sizeof(long));
		delete New_Root_Node; 
	}	
}

// Current_Node�� Parent_Node�� Set.
template <class Type>
int B_Plus_Tree<Type>::Get_Parent_Node(Type m_Inserted_Key, long m_Current_Node_Offset)
{	
	long Temp_P_Node_Offset = -1;
	// Root Node�� �о�´�.
	Tree_File.seekg(0, ios::beg);
	Tree_File.read((char*)&Temp_P_Node_Offset, sizeof(long));

	for(int i=0; i<Node_Count_In_File; i++, Temp_P_Node_Offset += sizeof(Node<Type>))
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

// m_Deleted_Key ���� ������, m_Deleted_Offset Block�� ����� �ϳ��� Element�� �����Ѵ�.
// �� �� External Node�� Delete�ϰ�, Internal Node & Root Node�� �����ϴ� Element�� TCP Element�� Ȱ���Ѵ�. 
// Return ������ Delete Success ���� �Ǵ�.
template <class Type>
bool B_Plus_Tree<Type>::Delete(Type m_Deleted_Key, long m_Deleted_Offset)           
{	
	bool m_Is_Delete;
	int m_Deleted_Index = -1;

	// Delete�Ǵ� Key���� ������ Key�� ������ Element�� �� �� ó�� ���� �Ϳ� ���� Information.�� �˷��ش�.
	Find(m_Deleted_Key, m_Deleted_Index, m_Is_Delete);
	int i=m_Deleted_Index;

	while(1)
	{
		for(i; i<Current_Node->Element_Count; i++)
		{
			if(Current_Node->Node_Element[i].Key    == m_Deleted_Key &&
			   Current_Node->Node_Element[i].Offset == m_Deleted_Offset)
			{
				for(int j=i+1; j<Current_Node->Element_Count; j++)
				{
					Current_Node->Node_Element[j-1].Key    = Current_Node->Node_Element[j].Key;
					Current_Node->Node_Element[j-1].Offset = Current_Node->Node_Element[j].Offset;
				}
			
				if(Current_Node->Element_Count == Element_In_Node)
				{
					Current_Node->Node_Element[j-1].Key = -1;
					Current_Node->Node_Element[j-1].Offset = Current_Node->Last_Offset;
					Current_Node->Last_Offset = -1;
				}
				else
				{		
					Current_Node->Node_Element[j-1].Key = -1;
					Current_Node->Node_Element[j-1].Offset = Current_Node->Node_Element[j].Offset;
				}
				Current_Node->Element_Count--;
				Dirty = true;
				return true;                                         // Delete�� ���������Ƿ� True�� Return.
			}		
		}
	
		// �ϳ��� Node���� ��� Elements�� ���� �ʰ�, ������ Element�� Key���� ������ Element�� Key�� ���� ���,
		// Current_Node->Next_Node_Offset���� ���� Node�� �̵��Ѵ�. 
		if(i == Current_Node->Element_Count && Current_Node->Node_Element[i-1].Key == m_Deleted_Key)
		{
			GetNode(Current_Node->Next_Node_Offset);
			i = 0;
		}
		else
			return false;
	}	
}


// Parameter�� �ش� Key�� �����ͼ� Msearch�� �Ѵ�.
template <class Type>
void B_Plus_Tree<Type>::Msearch(Type m_MSearch_Key, int* Result)
{
	int m_Index, Result_Count = 0;
	bool m_Is_Find;
	Find(m_MSearch_Key, m_Index, m_Is_Find);
	int i = m_Index;
	if(m_Is_Find == false)
		return;
	
	while(1)
	{
		// �ش� Block�� ������ Element�� Key���� ����, ���ϴ� Element�� ��ã�� ���. ���� Block���� �̵�.
		if(i == (Current_Node->Element_Count-1) && Current_Node->Node_Element[i].Key ==  m_MSearch_Key)
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

		// Key ���� �߰��� ���.
		if(Current_Node->Node_Element[i].Key == m_MSearch_Key)
			Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
		if(m_MSearch_Key < Current_Node->Node_Element[i].Key)
			return;
		i++;
	}
	return;
}

// Parameter�� �ش� Key [Min..Max]�� �����ͼ� Rsearch�� �Ѵ�.
template <class Type>
void B_Plus_Tree<Type>::Rsearch(Type m_Small_Key, Type m_Big_Key, int* Result)
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





































/*
// Hash Function�� ���� Split�� ��, Key�� ����� Block Offset�� ����.
template <class Type>
bool B_Plus_Tree<Type>::Change_Block(Type m_Change_Key, long Old_Offset, long New_Offset)
{
	int  m_Start_Index = -1;
	bool m_Is_Find = false;
    Find(m_Change_Key, m_Start_Index, m_Is_Find);
	int i = m_Start_Index; 
	if(m_Is_Find == true)
	{
		while(1)
		{
			// �ش� Block�� ������ Element�� Key���� ����, ���ϴ� Element�� ��ã�� ���. ���� Block���� �̵�.
			if(i == Current_Node->Element_Count && Current_Node->Node_Element[i].Key ==  m_Change_Key)
			{
				GetNode(Current_Node->Last_Offset);
				i = 0;
			}

			// Change�Ǵ� Key ���� �߰��� ���.
			if(Current_Node->Node_Element[i].Key == m_Change_Key && 
			   Current_Node->Node_Element[i].Offset == Old_Offset)
			{	
				Current_Node->Node_Element[i].Offset = New_Offset;
				Dirty = true;
				return true;
			}
			if(m_Change_Key < Current_Node->Node_Element[i].Key)
				break;
			i++;
		}
	}
	else
	{
		cout << "[MSG] Can`t Change The [ " << m_Change_Key << " ] Key Element..." << endl;
		return false;
	}
}
*/

			