#include <iostream>
#include <fstream>
#include <iomanip>
#include "Record_Block.h"
#include "Dynamic_Hash.h"

#define BLOCKSIZE  4096
#define BLOCKCOUNT 12000                                             
#define IN_BLOCK_MAX  BLOCKSIZE/sizeof(Student)                      // 95

#pragma pack(1)

using namespace std;

// Constructor
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

class JJun_DB{
private :
	fstream DBFile;                                                  // Database File.                                                
	string Operation;                                                // Current Operation.
	string Data[10];                                                 // Command���� Operation�� ������ ������ �κ��� Tokenizer�Ͽ� ����.
	Block DB_Buffer;                                                 // ������� ���� Buffer.
	Dynamic_Hash* H;                                                 // Hash Class.
						 
	char Dept[10];                                                   // Console Output�� ���� Variable.
	void Open();                                                     
	bool Insert();
	void Search();
	void List();
	int  Command_Tokenizer(string Command_Buffer);                   
	void Manual();
	void Interface();

public :
	friend class Dynamic_Hash;
	JJun_DB();
	~JJun_DB();
	void Command();
};

// Constructor
JJun_DB::JJun_DB()
{
	Operation = "";
	for(int i=0; i<4; i++)
		Data[i] = ""; 
	Manual();
}

// Destructor
JJun_DB::~JJun_DB()
{
	DBFile.clear();
	DBFile.close();
}

// Manual
void JJun_DB::Manual()
{
	printf("[ ******************** JJun Database Guide Line ******************** ]\n");
	printf("[ Student(Name(20), ID(4), Score(4), Dept(10)) ]\n");
	printf("[ binsert filename ] : Bulk Insertion.\n");
	printf("[ open filename    ] : DB File Load.\n");
	printf("[ insert (name, ID, score, dept) ] : Single Insetion.\n");
    printf("[ search attribute = value         ] : Value Search.\n");
	printf("[ exit ] : Program Termination & Storing.\n");
	printf("[ list ] : Listing\n");
}

// DB File�� Open�Ѵ�. 
void JJun_DB::Open()
{
	char Filename[80] = "";
	sprintf(Filename, "%s.Data", Data[0].c_str());
	// DataBase File�� Open�Ѵ�. File�� �������� ������ Create.
	DBFile.open(Filename, ios::in | ios::out | ios::binary);               
	if(!DBFile)
	{
		DBFile.clear();
		DBFile.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
		DBFile.seekp(0, ios::beg);                                        
		DBFile.write((char*)&DB_Buffer, sizeof(Block));
	}

	strcpy(Filename,""); 
	sprintf(Filename, "%s.Hash", Data[0].c_str());
	H = new Dynamic_Hash(Filename);
	
}

// Shell���� Command�� �Է¹޴´�.
void JJun_DB::Command()
{
	string Command_Buffer;
	char Buffer[256];
	cout << "[Command] >>";
	int count = 1;
	while(1)
	{	
		strcpy(Buffer,"");
		cin.getline(Buffer, 256);                                    // Shell���� Command �Է�.
		if(!strcmp(Buffer,""))
		{
			cout << "[Command] >>";	
			continue;
		}
		
		Command_Buffer = Buffer;
		if(Command_Tokenizer(Command_Buffer) == 0)                   // Return Value : 0�̸� Exit; 
			break;
		cout << "[Command] >>";
	}
	cout << "<< [MSG] Database Files are stored. Thank U for Yourself~ >>" << endl;	
}

// Command�� Tokenize�ϰ�, �ش� Procedure Call.
int JJun_DB::Command_Tokenizer(string Command_Buffer)
{
	char*  c_token;													 // Token.	
	char Token_Buffer[256] = "";
	string s_token;
	strcpy(Token_Buffer, Command_Buffer.c_str());
	
	c_token = strtok(Token_Buffer, " ");
	Operation = c_token;

	if(Operation == "open")                                          // Open Operation�� ��� Data[0]�� ��ȿ. 
	{
		c_token = strtok(NULL, " ");
		Data[0] = c_token;
		Open();
		cout << "[ " << Data[0] << " Database Open ]" << endl;
	}
	else if(Operation == "insert")                                   // Insertion. Data[0..5] Valid.
	{                                  
		c_token = strtok(NULL, "(, ");
		Data[0] = c_token;
		for(int i=1; i<3; i++)
		{
			c_token = strtok(NULL, " ,");
			Data[i] = c_token;
		}

		c_token = strtok(NULL, ") ");
		Data[3] = c_token;
		
		if(Insert())
			cout << "[MSG] << " << Data[0] << " " << Data[1] << " " << Data[2] << " " << Data[3] << " >> Record was inserted ]" << endl;
		else
			cout << "[ ID Collision... So Insertion Failed.. ]" << endl;
	}
	else if(Operation == "binsert")                                  // File�� ���ؼ� �ѹ��� Insert.
	{
		char Line[80];
		int m_True_Count = 0, m_False_Count = 0;
		ifstream Insert_File;
		c_token = strtok(NULL, " ");
		Insert_File.open(c_token);
		cout << "[MSG] Inserting Process... Please Wait.." << endl;
		
		while(1)                                                     // File�� ������ ��� �д´�.
		{
			Insert_File.getline(Line,80);
			if(Insert_File.fail())
				break;
			c_token = strtok(Line, ",");
			Data[0] = c_token;
			for(int i=1; i<4; i++)
			{
				c_token = strtok(NULL, ", ");
				Data[i] = c_token;
			}
			if(Insert())
			{	
				m_True_Count++;
			}
			else
			{
				m_False_Count++;
			}
			if(m_True_Count%1000 == 0)
				cout << "[ " << m_True_Count << " ] Records were Inserted... Please Wait." << endl;
		}
		
		cout << "[ " << m_True_Count << " Records was inserted ]" << endl;
		if(m_False_Count)
			cout << "[ " << m_False_Count << " Records was inserted ]" << endl;
		
		Insert_File.close();
	}
	
	else if(Operation == "search")
	{
		for(int i=0; i<2; i++)
		{
			c_token = strtok(NULL, " ");
			Data[i] = c_token;
			c_token = strtok(NULL, " ");                                     
		}	
		Search();
	}
	else if(Operation == "list")
	{
		List();
	}
	else if(Operation == "exit")
	{
		return 0;
	}
	else
		return 1;
	
	return 1;
}

// �ϳ��� Record�� Insert�Ѵ�.
bool JJun_DB::Insert()
{	
	bool Is_Insert = false;
	///// Dynamic Hash�� ����Ͽ� Database�� �����Ѵ�.
	int DB_File_Offset = H->Get_Hash_Offset(Data[1]);                // �ش� ID Record�� �����ؾ� �� Block Offset�� Return.
	
	DBFile.seekg(DB_File_Offset, ios::beg);                          // �ش� Block�� �о� �´�.      
	DBFile.read((char*)&DB_Buffer, sizeof(Block));
	
	// Insert�Ǵ� Record�� ID�� ���� Record�� �̹� �����ϴ� ��츦 Check. 
	for(int i=0; i<DB_Buffer.Record_Count; i++)			           
		if(DB_Buffer.Record[i].ID == atoi(Data[1].c_str()))		
			return false;
	
	if(DB_Buffer.Record_Count < IN_BLOCK_MAX)                        // Buffer Block�� �� �������� ���.
	{
		// Record Insertion.
		strcpy(DB_Buffer.Record[DB_Buffer.Record_Count].Name, Data[0].c_str());			
		DB_Buffer.Record[DB_Buffer.Record_Count].ID = atoi(Data[1].c_str());
		DB_Buffer.Record[DB_Buffer.Record_Count].Score = atof(Data[2].c_str());
		strncpy(DB_Buffer.Record[DB_Buffer.Record_Count].Dept, Data[3].c_str(),10);

		
		DB_Buffer.Record_Count++;
		// Modified�� Buffer�� Database�� ���ش�.
		DBFile.seekp(DB_File_Offset, ios::beg);      
		DBFile.write((char*)&DB_Buffer, sizeof(Block));
		Is_Insert = true;
	}
	else                                                             // Buffer Block�� ���� ���.
	{															
		H->Block_Full(Data[1].c_str(), DB_Buffer.Bit_Num, DBFile);
		Insert();
	}

	return true;
}



// Msearch By Attribute = Value; Attribute�� ���� Value�� Records�� ã�� ȭ�鿡 ����Ѵ�.
// Data[0] = Attribute. Data[1] = Value.
void JJun_DB::Search()
{
	if(Data[0] == "ID")	
	{
	Interface();
	
	long DB_File_Offset = H->Get_Hash_Offset(Data[1]);               // �ش� ID Record�� �����ϴ� Block Offset�� Return.
	
	DBFile.seekg(DB_File_Offset, ios::beg);                          // �ش� Block�� �о� �´�.      
	DBFile.read((char*)&DB_Buffer, sizeof(Block));
	 
	int i;
	for(i=0; i<DB_Buffer.Record_Count; i++)			           
		if(DB_Buffer.Record[i].ID == atoi(Data[1].c_str()))          // Case : ID�� �ش��ϴ� Record�� �����ϴ� ���.		
		{
			strncpy(Dept, DB_Buffer.Record[i].Dept, 10);
			Dept[10] = '\0';
			cout<<"| "<<right<<setw(6)<< "1" << " | " <<left <<setw(12)<< DB_Buffer.Record[i].Name <<"| "<< setw(12) << DB_Buffer.Record[i].ID ;
			cout<<"| "<<right<<setw(5) << DB_Buffer.Record[i].Score << setw(4);
			cout<<"| "<<left <<setw(11)<< Dept <<setw(3) << "| "<< endl;
			cout<<"+----------------------------------------------------------+" << endl;
			return;
		}
		cout << "[MSG] No Record Look Up." << endl;
	}
	else cout << "[MSG] Invalid Command" << endl;
}

// �Էµ� ��� Records�� ����Ѵ�.
void JJun_DB::List()
{
	int m_Total_Count = 0;
	                                          
	Interface();
	ofstream fout;
	fout.open("out.txt", ios::trunc);
	DBFile.seekg(0, ios::beg);
	while(DBFile.read((char*)&DB_Buffer, sizeof(Block)))
	{
		
		for(int i=0; i<DB_Buffer.Record_Count; i++)                  // �ϳ��� Block�� ���Ͽ� Valid�� Record�� Console�� �Ѹ���.
		{
			strncpy(Dept, DB_Buffer.Record[i].Dept, 10);
			Dept[10] = '\0';
			
			cout<<"| "<<right<<setw(6)<< ++m_Total_Count << " | " <<left <<setw(12)<< DB_Buffer.Record[i].Name <<"| "<< setw(12) << DB_Buffer.Record[i].ID ;
			cout<<"| "<<right<<setw(5) << DB_Buffer.Record[i].Score << setw(4);
			cout<<"| "<<left <<setw(11)<< Dept <<setw(3) << "| "<< endl;
			cout<<"+----------------------------------------------------------+" << endl;
		}
		cout << endl << endl;
	}
	DBFile.clear();
	cout << "Total " << m_Total_Count << endl;
};
	 

// ID MSearch.

void JJun_DB::Interface()
{
	cout << endl;
	cout << "+----------------------------------------------------------+" << endl;
	cout << "| No     | name        | ID          | score  | dept.      |" << endl;
	cout << "+----------------------------------------------------------+" << endl;
}

int main(int argc, char* argv[])
{
	JJun_DB JJun;
	JJun.Command();
	return 0;
}