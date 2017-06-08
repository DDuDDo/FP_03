#include <iostream>
#include <fstream>
#include <string>

#define BLOCKSIZE 4096                                               // Block Size = 4K.
#pragma pack(1)

using namespace std;

// ************************************************************************************************
// ************************* Element Class : Node에 삽입되어지는 Element. *************************
// ************************************************************************************************ 
template <class Type>
class Element{
public : 
	long  Offset;													 // External Node인 경우 DB File내의 Offset.(ID를 기억한다.) 
																	 // Internal Node, Root Node인 경우는 하위 Node를 가리킨다.
	                                                                 // 이 때 Point되는 Node는 이 Element의 Key보다 작은 값들이 모인 Node.  
	Type  Key;                                                       // Key(Value).

	Element();
};

// Constructor : Offset과 Key가 -1을 나타내면 Invalid하다.
template <class Type>
Element<Type>::Element()
{
	Offset = -1;
	Key    = (Type)-1;
}

// ************************************************************************************************
// **************************** Node Class : B+Tree에서의 하나의 Node. ****************************
// ************************************************************************************************ 
template <class Type>
class Node{
public :
	Element<Type> Node_Element[BLOCKSIZE/sizeof(Element<Type>)-2];   // 하나의 Node에 들어가는 Elements.(510)
	long    Last_Offset;                                             // 하나의 Node에서 Last Fan-Out.
	long    Next_Node_Offset;                                        // External Node인 경우. 다음 Node의 Offset. -1인 경우 Invalid.
	int     Element_Count;                                           // Node에서 채워진 Element 갯수.
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
	int      Node_Count_In_File;                                     // Tree내 Node의 갯수.
	long     Root_Node_Offset;                                       // Root Node Offset.
	Node<Type>* Current_Node;                                        // 현재 Node.
	long     Current_Node_Offset;                                    // 현재 Node가 저장된 File의 Offset.
	bool     Dirty;                                                  // 현재 Node의 Update 유무.
	int      Element_In_Node;                                        // 한 Node 안의 Element 수.
	long     Small_Pointer;                                          // Split될 때 왼쪽   Node.
	long     Big_Pointer;                                            // Split될 때 오른쪽 Node.
	fstream  Tree_File;                                              // Tree가 저장된 File.
	//ofstream  fout;/////
	void Init(char* Filename);                                       // 생성자에서 Call하는 Initialization Procedure.
	void GetNode(long m_Offset);				                     // Param의 Offset에 위치한 Node를 Current_Node로 Set.
	int  FindInNode(Node<Type>* m_Current_Node, Type m_Find_Key, bool& m_Is_Find);       // m_Current_Node에서 Key를 찾는다.
	void Insert_Element(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index);
	void JustInsert(Type m_Inserted_Key,  long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index);
	void SplitInsert(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index);
	int  Get_Parent_Node(Type m_Inserted_Key, long m_Current_Node_Offset);               // Parent Node 얻는다.

public :
	B_Plus_Tree(char* Filename);
	~B_Plus_Tree();
	long Find(Type m_Find_Key, int& m_Index, bool& m_Is_Find);       // Key 값을 가지고 Tree에서 존재하는지 여부를 Check.															 
	bool Insert(Type m_Inserted_Key, long m_Inserted_Offset);        // 하나의 Element를 Insert.
	bool Delete(Type m_Deleted_Key,  long m_Deleted_Offset);         // 하나의 Element를 Delete.
	bool Change_Block(Type m_Change_Key, long Old_Offset, long New_Offset);    // Hash Function에 의해 Split될 때, Key가 저장된 Block Offset을 변경.
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
	Element_In_Node = BLOCKSIZE/sizeof(Element<Type>)-2;             // 여기서는 510이다.
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
	// B+ Tree File을 Open한다. 존재하는 경우 File로부터 Root_Node_Offset을 얻는다. 
	// File이 존재하지 않으면 Root Node를 Create한다.
	Tree_File.open(Filename, ios::in | ios::out | ios::binary);
	if(!Tree_File)                                                   // File Open에 실패한 경우.
	{
		Tree_File.clear();                                           // Open시 설정된 Error Bit를 모두 Clear.
		Tree_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
		
		// Root Node를 생성하여 Tree_File의 Offset 4에 저장한다. Offset[0..4]는 Root Node를 가리키는 Pointer이다. 
		Node<Type>* Root = new Node<Type>;                                      
		Root->Node_Mode = 4;                                         // Root & External.
		Tree_File.seekp(sizeof(long), ios::beg);
		Root_Node_Offset = Tree_File.tellp();
		Tree_File.write((char*)Root, sizeof(Node<Type>));            // Root Node 저장.
		Tree_File.seekp(0, ios::beg);
		Tree_File.write((char*)&Root_Node_Offset, sizeof(long));     // Root Node Offset 저장.
		delete Root;
	}
	else                                                             // Tree_File이 이미 존재하는 경우.
	{
		Tree_File.seekg(0, ios::beg);        
		Tree_File.read((char*)&Root_Node_Offset, sizeof(long));      // Root_Node_Offset Get.     
	}

	Tree_File.seekg(0, ios::end);        
	Node_Count_In_File = (Tree_File.tellg()-(long)4)/(sizeof(Node<Type>));
}

// Find Function으로 주어진 Key값을 가지고 B_Plus_Tree에서 찾는다.
// 이때 Range Search를 가능하게 하기위하여 동일한 값이 존재하는 경우 맨 처음의 Key에 대한 DB_FIle Offset을 반환한다.
// 그러므로 뒤에 연속적으로 존재하는 동일한 Key Element들을 모두 접근 할 수 있다.
// 1st Param : Find하게될 Key. 2nd Param : 찾은 Key값의 Index or Insert될 Index 저장. 3th Param : 존재하는 경우 True, 아니면 False.  
// Current_Node : Key가 존재하는 경우 그 Key가 존재하는 Node가, 아닌 경우 그 key값이 Insert되야하는 Node가 지정된다.
// Return Value : Key가 존재하는 경우 그 Key의 DB_file 내 Block Offset을 Return, 아닌 경우 -1 Return.  
template <class Type>
long B_Plus_Tree<Type>::Find(Type m_Find_Key, int& m_Index, bool& m_Is_Find)
{
	m_Is_Find = false;
	m_Index = 0;
	long m_Node_Offset = Root_Node_Offset;                           // Root Node에서 Find 시작.
	
	while(1)
	{	
		GetNode(m_Node_Offset);                                      // m_Node_Offset을 가지고 해당 Node를 Current_Node에 Set.
		
		// External Node인 경우 Break.
		if(Current_Node->Node_Mode == 3 || Current_Node->Node_Mode == 4)
			break;
		else
		{
			for(int i=0; i<Current_Node->Element_Count; i++)         // 각 Node의 Valid한 Element 수 만큼 Loop.
			{
				if(m_Find_Key <= Current_Node->Node_Element[i].Key)  // m_Find_Key가 비교하는 Key보다 작거나 같으면 하위 Node로 이동.
				{
					m_Node_Offset = Current_Node->Node_Element[i].Offset;
					break;
				}
			}
			if(i == Current_Node->Element_Count)					 // m_Find_Key가 하나의 Node의 모든 Element들의 Key보다 큰 경우.
			{
				if(Current_Node->Element_Count == Element_In_Node)   // 비교한 Node에 Element들이 꽉 차있는 경우.  
					m_Node_Offset = Current_Node->Last_Offset;
				else
					m_Node_Offset = Current_Node->Node_Element[i].Offset;
			}
		}
	}
	
	// Statement : 위의 While Loop로 인해 Current Node에 Key값이 존재할 가능성이 있는 Node가 저장되어있고 이는 External Node이다.  
	m_Index = FindInNode(Current_Node, m_Find_Key, m_Is_Find);
	
	if(m_Is_Find == true)                                            
		return Current_Node->Node_Element[m_Index].Offset;
	else
		return -1;
}

// m_Node_Offset을 가지고 해당 Node를 Current_Node에 Set.
template <class Type>
void B_Plus_Tree<Type>::GetNode(long m_Offset)
{
	if(Current_Node_Offset == m_Offset)                              // Current_Node인 경우. 그냥 바로 Return;
	{
		Dirty = true;
		return;
	}
	if(Dirty)                                                        // Current_Node를 교체하기 전에 Dirty Bit를 Check.    
	{
		Tree_File.seekp(Current_Node_Offset, ios::beg);              // Current_Node가 저장된 File의 Offset으로 간다.
		Tree_File.write((char*)Current_Node, sizeof(Node<Type>));    // Current_Node Update.
		Dirty = false;                                               // Dirty Bit Release.
	}

	Current_Node_Offset = m_Offset;
	Tree_File.seekg(Current_Node_Offset, ios::beg);                  // Current_Node_Offset으로 Read File Pointer Set. 
	Tree_File.read((char*)Current_Node, sizeof(Node<Type>));         // Current_Node로 읽어온다. 
}

// m_Current_Node에서 m_Find_Key가 존재하는지 Check하여 존재한다면 m_Is_Find를 True로 Set하고 Index를 Return, 
// 존재하지 않은 경우는 m_Is_Find를 False로 Set하고 Insert될 Index를 Return.
template <class Type>
int B_Plus_Tree<Type>::FindInNode(Node<Type>* m_Current_Node, Type m_Find_Key, bool& m_Is_Find)
{                                                                    
 	for(int i=0; i<m_Current_Node->Element_Count; i++)
	{
		if(m_Current_Node->Node_Element[i].Key == m_Find_Key)        // Case : 같은 Key 값 발견.          
		{
			m_Is_Find = true;
			return i;
		}
		if(m_Find_Key < m_Current_Node->Node_Element[i].Key)         // Case : 발견하지 못한 경우.
		{
			m_Is_Find = false;
			return i;
		}
	}
	// Statement : 현재 Node에 존재하는 모든 Element들의 Key값보다 큰 경우.
	m_Is_Find = false;
	return m_Current_Node->Element_Count;
}

// Insert Function. 1st Param : Tree에 Insert될 Key, 2nd Param : 그 Key를 가진 Record의 DB_File 내 Block Offset.        
template <class Type>
bool B_Plus_Tree<Type>::Insert(Type m_Inserted_Key, long m_Inserted_Offset)
{
	bool m_Is_Find = false;                                                  
	int m_Insert_Index = -1;
		
	Find(m_Inserted_Key, m_Insert_Index, m_Is_Find);    
	
	// Statement 
	// m_Is_Find가 True인 경우 ==> 같은 Key값을 가진 Elements들의 맨 처음 Index를 가지고 Return.
	// 이 때 동일한 Key를 가진 Element가 여러 개 있는 경우. 모두 검색하여 Key 값도 같고 가리키는 Block Offset도 
	// 같은 경우 Insert하지 않는다. 
	// m_Is_Find가 False인 경우 ==> Current_Node의 m_Insert_Index에 Insert한다.

	Insert_Element(m_Inserted_Key, m_Inserted_Offset, Current_Node, m_Insert_Index);
	return true;
}

template <class Type>
void B_Plus_Tree<Type>::Insert_Element(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index)
{
	if(m_Current_Node->Element_Count != Element_In_Node)             // Case : Node가 꽉차지 않아서 Split이 필요하지 않은 경우.
		JustInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);
	else															 // Case : Node가 꽉차서 Split이 필요한 경우.
		SplitInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);               
}

// Node가 꽉차지 않아서 Split 필요없이 Insert가 되는 경우.
template <class Type>
void B_Plus_Tree<Type>::JustInsert(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index)
{ 			
	if(m_Current_Node->Element_Count <= 94)
		m_Current_Node->Node_Element[m_Current_Node->Element_Count+1].Offset = m_Current_Node->Node_Element[m_Current_Node->Element_Count].Offset;
	else
		m_Current_Node->Last_Offset = m_Current_Node->Node_Element[m_Current_Node->Element_Count].Offset;

	for(int j=m_Current_Node->Element_Count; j>m_Insert_Index; j--)  // Insert될 Index 뒤의 모든 Element들을 한 칸씩 이동한다. 
	{
		m_Current_Node->Node_Element[j].Key    = m_Current_Node->Node_Element[j-1].Key;
		m_Current_Node->Node_Element[j].Offset = m_Current_Node->Node_Element[j-1].Offset;
	}

	m_Current_Node->Node_Element[m_Insert_Index].Key    = m_Inserted_Key;
	m_Current_Node->Node_Element[m_Insert_Index].Offset = m_Inserted_Offset;
	m_Current_Node->Element_Count++;                                 // Element Count 증가.
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

// Insertion할 때 Node가 꽉차서 Split이 필요한 경우.
template <class Type>
void B_Plus_Tree<Type>::SplitInsert(Type m_Inserted_Key, long m_Inserted_Offset, Node<Type>* m_Current_Node, int m_Insert_Index)
{
	int s, t;
	Node<Type>* New_Node = new Node<Type>;                           // 새로운 Node를 Create.
	Tree_File.seekg(0, ios::end);
	long  New_Node_Offset = Tree_File.tellg();   
	
	// New_Node Mode Setting.
	if(m_Current_Node->Node_Mode == 1 || m_Current_Node->Node_Mode == 2)       // Root Node가 Split.		
		New_Node->Node_Mode = 2;	
	else if(m_Current_Node->Node_Mode == 3 || m_Current_Node->Node_Mode == 4)  
	{
		New_Node->Node_Mode = 3;	
		New_Node->Next_Node_Offset = m_Current_Node->Next_Node_Offset;
		m_Current_Node->Next_Node_Offset = New_Node_Offset;
	}
	Tree_File.seekp(0, ios::end);
	New_Node_Offset = Tree_File.tellp();                             // New_Node의 DB_File 내 Offset을 가져온다.
	Tree_File.write((char*)New_Node, sizeof(Node<Type>));
	Node_Count_In_File++;

	// m_Current_Node의 [0..N/2-1]까지는 그대로 남겨두고, [N/2..N]까지 Element들은 New_Node로 이동시킨다.
	for(s=Element_In_Node/2, t=0; s<Element_In_Node; s++, t++)
	{
		New_Node->Node_Element[t].Key    = m_Current_Node->Node_Element[s].Key;
		New_Node->Node_Element[t].Offset = m_Current_Node->Node_Element[s].Offset;

		m_Current_Node->Node_Element[s].Key    = (Type)-1;
		m_Current_Node->Node_Element[s].Offset = -1;
	}

	Dirty = true;
	// Node내 Count 설정.
	m_Current_Node->Element_Count = Element_In_Node/2;
	New_Node->Element_Count = Element_In_Node - Element_In_Node/2;
	
	// Node내 Offset 설정.
	New_Node->Node_Element[t].Offset = m_Current_Node->Last_Offset;
	m_Current_Node->Last_Offset = -1;
	
 	// m_Inserted_Key를 Insert한다.
	if(m_Insert_Index < Element_In_Node/2)                           // Case : 추가될 Element가 Current_Node에 삽입되는 경우.
	{	
		JustInsert(m_Inserted_Key, m_Inserted_Offset, m_Current_Node, m_Insert_Index);
	}
	else                                                             // Case : 추가될 Element가 New_Node에 삽입되는 경우.
	{
		JustInsert(m_Inserted_Key, m_Inserted_Offset, New_Node, (m_Insert_Index - Element_In_Node/2));
	}

	// New_Node의 Update된 내용을 DB_File에 Write.
	Tree_File.seekp(New_Node_Offset, ios::beg);
	Tree_File.write((char*)New_Node, sizeof(Node<Type>));
	delete New_Node;

	// Parent_Node를 위하여 Big_Pointer, Small_Pointer를 지정해둔다.
	
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
	else															 // Root Node가 Split.
	{				
		Node<Type>* New_Root_Node = new Node<Type>;	
		Tree_File.seekp(0, ios::end);
		Root_Node_Offset = Tree_File.tellp();                        // New_Node의 DB_File 내 Offset을 가져온다.
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

// Current_Node에 Parent_Node가 Set.
template <class Type>
int B_Plus_Tree<Type>::Get_Parent_Node(Type m_Inserted_Key, long m_Current_Node_Offset)
{	
	long Temp_P_Node_Offset = -1;
	// Root Node를 읽어온다.
	Tree_File.seekg(0, ios::beg);
	Tree_File.read((char*)&Temp_P_Node_Offset, sizeof(long));

	for(int i=0; i<Node_Count_In_File; i++, Temp_P_Node_Offset += sizeof(Node<Type>))
	{
		GetNode(Temp_P_Node_Offset);
		
		if(Current_Node->Node_Mode == 4)
			return -1;
		if(Current_Node->Node_Mode == 3) 
			continue;

		for(int j=0; j<=Current_Node->Element_Count; j++)            // 각 Node의 Valid한 Element 수 만큼 Loop.
		{
			if(m_Current_Node_Offset == Current_Node->Node_Element[j].Offset)   
				return j;
		}
	}
	return -1;
}

// m_Deleted_Key 값을 가지고, m_Deleted_Offset Block에 저장된 하나의 Element를 삭제한다.
// 이 때 External Node만 Delete하고, Internal Node & Root Node에 존재하는 Element는 TCP Element로 활용한다. 
// Return 값으로 Delete Success 여부 판단.
template <class Type>
bool B_Plus_Tree<Type>::Delete(Type m_Deleted_Key, long m_Deleted_Offset)           
{	
	bool m_Is_Delete;
	int m_Deleted_Index = -1;

	// Delete되는 Key값과 동일한 Key를 가지는 Element들 중 맨 처음 만난 것에 대한 Information.을 알려준다.
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
				return true;                                         // Delete를 성공했으므로 True를 Return.
			}		
		}
	
		// 하나의 Node에서 모든 Elements와 같지 않고, 마지막 Element의 Key값과 삭제할 Element의 Key가 같을 경우,
		// Current_Node->Next_Node_Offset으로 다음 Node로 이동한다. 
		if(i == Current_Node->Element_Count && Current_Node->Node_Element[i-1].Key == m_Deleted_Key)
		{
			GetNode(Current_Node->Next_Node_Offset);
			i = 0;
		}
		else
			return false;
	}	
}


// Parameter로 해당 Key를 가져와서 Msearch를 한다.
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
		// 해당 Block의 마지막 Element와 Key값이 같고, 원하는 Element를 못찾은 경우. 다음 Block으로 이동.
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

		// Key 값을 발견한 경우.
		if(Current_Node->Node_Element[i].Key == m_MSearch_Key)
			Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
		if(m_MSearch_Key < Current_Node->Node_Element[i].Key)
			return;
		i++;
	}
	return;
}

// Parameter로 해당 Key [Min..Max]를 가져와서 Rsearch를 한다.
template <class Type>
void B_Plus_Tree<Type>::Rsearch(Type m_Small_Key, Type m_Big_Key, int* Result)
{
	int m_Index, Result_Count = 0;
	bool m_Is_Find;
	Find(m_Small_Key, m_Index, m_Is_Find);
	int i = m_Index;
	
	while(1)
	{
		// 하나의 block을 다 뒤지고, 원하는 Element가 남아있을 가능성이 있는 경우. 다음 Block으로 이동.
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

		// Change되는 Key 값을 발견한 경우.
		if(m_Small_Key <= Current_Node->Node_Element[i].Key && Current_Node->Node_Element[i].Key <= m_Big_Key)
			Result[Result_Count++] = Current_Node->Node_Element[i].Offset;
		if(m_Big_Key < Current_Node->Node_Element[i].Key)
			return;
		i++;
	}
	return;
}





































/*
// Hash Function에 의해 Split될 때, Key가 저장된 Block Offset을 변경.
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
			// 해당 Block의 마지막 Element와 Key값이 같고, 원하는 Element를 못찾은 경우. 다음 Block으로 이동.
			if(i == Current_Node->Element_Count && Current_Node->Node_Element[i].Key ==  m_Change_Key)
			{
				GetNode(Current_Node->Last_Offset);
				i = 0;
			}

			// Change되는 Key 값을 발견한 경우.
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

			