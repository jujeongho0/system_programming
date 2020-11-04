#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct _sym_table {
    char symbol[20];
    int loc;
};

struct _op_table {
    char op[20];
    int opcode;
    int oplength;
    char sicxe;
};

struct _op_table op_table[] =
{
    {"ADD",0x18,3,0}, {"ADDF",0x58,3,1}, {"ADDR",0x90,2,1}, {"AND",0x40,3,0},
    {"CLEAR",0xB4,2,1}, {"COMP",0x28,3,0}, {"COMPF",0x88,3,1}, {"COMPR",0xA0,2,1},
    {"DIV",0x24,3,0}, {"DIVF",0x64,3,1}, {"DIVR",0x9C,2,1}, {"FIX",0xC4,1,1},
    {"FLOAT",0xC0,1,1}, {"HIO",0xF4,1,1}, {"J",0x3C,3,0}, {"JEQ",0x30,3,1},
    {"JGT",0x34,3,0}, {"JLT",0x38,3,0}, {"JSUB",0x48,3,0}, {"LDA",0x00,3,0},
    {"LDB",0x68,3,1}, {"LDCH",0x50,3,0}, {"LDF",0x70,3,1}, {"LDL",0x08,3,0},
    {"LDS",0x6C,3,1}, {"LDT",0x74,3,1}, {"LDX",0x04,3,0}, {"LPS",0xD0,3,0},
    {"MUL",0x20,3,0}, {"MULF",0x60,3,1}, {"MULR",0x98,2,1}, {"NORM",0xC8,1,1},
    {"OR",0x44,3,0}, {"RD",0xD8,3,0}, {"RMO",0xAC,2,1}, {"RSUB",0x4C,3,0},
    {"SHIFTL",0xA4,2,1}, {"SHIFTR",0xA8,2,1}, {"SIO",0xF0,1,1}, {"SSK",0xEC,3,1},
    {"STA",0x0C,3,0}, {"STB",0x78,3,1}, {"STCH",0x54,3,0}, {"STF",0x80,3,1},
    {"STI",0xD4,3,1}, {"STL",0x14,3,0}, {"STS",0x7C,3,1}, {"STSW",0xE8,3,0},
    {"STT",0x84,3,1}, {"STX",0x10,3,0}, {"SUB",0x1C,3,0}, {"SUAF",0x5C,3,1},
    {"SUBR",0x94,2,1}, {"SVC",0xB0,2,1}, {"TD",0xE0,3,0}, {"TIO",0xF8,1,1},
    {"TIX",0x2c,3,0}, {"TIXR",0xB8,2,1}, {"WD",0xDC,3,0},
    
    {"BASE",0,0,0},{"NOBASE",0,0,0}, {"BYTE",1,0,0}, {"END",2,0,0},
    {"EQU",7,0,0}, {"LTORG",8,0,0}, {"RESB",3,0,0}, {"RESW",4,0,0},
    {"START",5,0,0}, {"WORD",6,0,0}, {"USE",9,0,0}, {"CSECT",10,0,0},
    {"EXTREF",11,0,0}, {"EXTDEF",12,0,0}
};

char source[20],object[20];
int length;
FILE* f_src, *f_obj, *f_lst, *f_itr;

char HEXTAB[]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

int hexstr2dec(char str)
{
    int i;
    for(i=0;i<=15;i++)
    {
        if(HEXTAB[i] == str)
            return i;
    }
    return (-1);
}

char objcode[100],text_record[100];
int record_start,text_count=0;
int modcode[100],mod_entity=0;
void writeTEXT(void)
{
    fprintf(f_obj,"T%06X%02lX%s\n",record_start,strlen(text_record)/2,text_record);
    strcpy(text_record,"");
    text_count=0;
}

void pass1(void)
{
    f_itr = fopen("intermediate_file.itr","w");

    struct _sym_table sym_table1[100];
    int  LOCCTR=0,start_address=0,sym_entity=0,lt_entity;
    char temp[255],token[20],label[20],opcode[20],op[40];
    while(fgets(temp,255,f_src))//src -> itr(code(+error_flag) + sym_table)
    {   
        int idx1=0 ,idx2=0,start_flag=0,sym_flag=0,resw_flag=0,resb_flag=0,byte_flag=0,end_flag=0,lt_flag=0;
        int error_flag1=0;//duplicate symbol
        int error_flag2=0;//invalid operation

        //get first token
        while(temp[idx1]==' ' || temp[idx1]=='\t')
            idx1++;
        if(temp[idx1]=='.' || temp[idx1]=='\n' || temp[idx1]=='\r')
        {
            fprintf(f_itr,"6\t%s",temp);        
            continue;//line : comment or space
        }        
        if(idx1 == 0)//first token is label
        {
            while(temp[idx1]!=' ' && temp[idx1]!='\t' && temp[idx1]!='.' && temp[idx1]!='\n' && temp[idx1]!='\r')
            {
                token[idx2] = temp[idx1];
                idx1++;
                idx2++;
            }
            token[idx2]='\0';
            strcpy(label,token);
            for(int i=0;i<sym_entity;i++)//check label is in symbol table
            {
                if(strcmp(sym_table1[i].symbol,label)==0)
                {               
                    sym_flag=1;
                    break;                
                }            
            }
            if(sym_flag==1)
                error_flag1=1;//duplicate symbol
            else
            {
                strcpy(sym_table1[sym_entity].symbol,label);
                sym_table1[sym_entity].loc = start_address + LOCCTR;
                sym_entity++;
            }

            //get second token          
            idx2=0;
            while(temp[idx1]==' ' || temp[idx1]=='\t')
                idx1++;
            if(temp[idx1]=='.' || temp[idx1]=='\n' || temp[idx1]=='\r')
            {
                fprintf(f_itr,"3\t%s\t\t\t%d%d\n",label,error_flag1,error_flag2);            
                continue;//line : label
            }
            while(temp[idx1]!=' ' && temp[idx1]!='\t' && temp[idx1]!='.' && temp[idx1]!='\n' && temp[idx1]!='\r')
            {
                token[idx2] = temp[idx1];
                idx1++;
                idx2++;
            }
            token[idx2]='\0';
            strcpy(opcode,token);
            if(opcode[0]=='+')
            {               
                error_flag2=1;
                for(int i=0;i<(sizeof(op_table)/sizeof(struct _op_table));i++)
                {
                    if(strcmp(op_table[i].op,opcode+1)==0)
                    {   
                        error_flag2=0;
                        LOCCTR = LOCCTR + 4;                        
                        break;
                    }
                }                
            }
            else
            {
                if(strcmp(opcode,"START")==0)
                    start_flag=1;         
                else if(strcmp(opcode,"WORD")==0)
                    LOCCTR = LOCCTR + 3;                
                else if(strcmp(opcode,"RESW")==0)
                    resw_flag=1;
                else if(strcmp(opcode,"RESB")==0)
                    resb_flag=1;
                else if(strcmp(opcode,"BYTE")==0)
                    byte_flag=1;                
                else if(strcmp(opcode,"END")==0)
                    end_flag=1;
                else
                {
                    error_flag2=1;
                    for(int i=0;i<(sizeof(op_table)/sizeof(struct _op_table));i++)//check second token is in op table
                    {
                        if(strcmp(op_table[i].op,opcode)==0)
                        {         
                            error_flag2=0;      
                            LOCCTR = LOCCTR + op_table[i].oplength;
                            break;                
                        }                               
                    }
                }                
            }

            //get third token
            idx2=0;
            while(temp[idx1]==' ' || temp[idx1]=='\t')
                idx1++;
            if(temp[idx1]=='.' || temp[idx1]=='\n' || temp[idx1]=='\r')
            {
                fprintf(f_itr,"2\t%s\t%s\t\t%d%d\n",label,opcode,error_flag1,error_flag2);            
                continue;//line : label opcode
            }
            while(temp[idx1]!=' ' && temp[idx1]!='\t' && temp[idx1]!='.' && temp[idx1]!='\n' && temp[idx1]!='\r')
            {
                token[idx2] = temp[idx1];
                idx1++;
                idx2++;
            }
            token[idx2]='\0';
            strcpy(op,token);            
            if(start_flag==1)
                start_address = atoi(op);
            else if(resw_flag==1)
                LOCCTR = LOCCTR + 3*atoi(op);
            else if(resb_flag==1)
                LOCCTR = LOCCTR + atoi(op);
            else if(byte_flag==1)
            {
                int constant=0,idx3=0,idx4=0;
                while(op[idx3]!='\'')
                    idx3++;
                idx4 = idx3 +1;
                while(op[idx4]!='\'')
                    idx4++;
                constant = idx4 - idx3 -1;                
                LOCCTR = LOCCTR + constant;
            }
            fprintf(f_itr,"1\t%s\t%s\t%s\t%d%d\n",label,opcode,op,error_flag1,error_flag2);
            if(end_flag==1)
                break;//line : label END op
            continue;//line : label opcode op            
        }
        else//first token is opcode
        {
           while(temp[idx1]!=' ' && temp[idx1]!='\t' && temp[idx1]!='.' && temp[idx1]!='\n' && temp[idx1]!='\r')
            {
                token[idx2] = temp[idx1];
                idx1++;
                idx2++;
            }
            token[idx2]='\0';            
            strcpy(opcode,token);
            if(opcode[0]=='+')
            {               
                error_flag2=1;
                for(int i=0;i<(sizeof(op_table)/sizeof(struct _op_table));i++)
                {
                    if(strcmp(op_table[i].op,opcode+1)==0)
                    {   
                        error_flag2=0;
                        LOCCTR = LOCCTR + 4;                        
                        break;
                    }
                }                
            }
            else
            {
                if(strcmp(opcode,"START")==0)
                    start_flag=1;         
                else if(strcmp(opcode,"WORD")==0)
                    LOCCTR = LOCCTR + 3;                
                else if(strcmp(opcode,"RESW")==0)
                    resw_flag=1;
                else if(strcmp(opcode,"RESB")==0)
                    resb_flag=1;
                else if(strcmp(opcode,"BYTE")==0)
                    byte_flag=1;                
                else if(strcmp(opcode,"END")==0)
                    end_flag=1;
                else
                {
                    error_flag2=1;
                    for(int i=0;i<(sizeof(op_table)/sizeof(struct _op_table));i++)//check first token is in op table
                    {
                        if(strcmp(op_table[i].op,opcode)==0)
                        {         
                            error_flag2=0;      
                            LOCCTR = LOCCTR + op_table[i].oplength;
                            break;                
                        }                               
                    }
                }                
            }

            //get second token
            idx2=0;
            while(temp[idx1]==' ' || temp[idx1]=='\t')
                idx1++;
            if(temp[idx1]=='.' || temp[idx1]=='\n' || temp[idx1]=='\r')
            {
                fprintf(f_itr,"5\t\t%s\t\t%d%d\n",opcode,error_flag1,error_flag2);            
                continue;//line : opcode
            }
            while(temp[idx1]!=' ' && temp[idx1]!='\t' && temp[idx1]!='.' && temp[idx1]!='\n' && temp[idx1]!='\r')
            {
                token[idx2] = temp[idx1];
                idx1++;
                idx2++;
            }
            token[idx2]='\0';
            strcpy(op,token);            
            if(start_flag==1)
                start_address = atoi(op);
            else if(resw_flag==1)
                LOCCTR = LOCCTR + 3*atoi(op);
            else if(resb_flag==1)
                LOCCTR = LOCCTR + atoi(op);
            else if(byte_flag==1)
            {
                int constant=0,idx3=0,idx4=0;
                while(op[idx3]!='\'')
                    idx3++;
                idx4 = idx3 +1;
                while(op[idx4]!='\'')
                    idx4++;
                constant = idx4 - idx3 -1;                
                LOCCTR = LOCCTR + constant;
            }
            fprintf(f_itr,"4\t\t%s\t%s\t%d%d\t\n",opcode,op,error_flag1,error_flag2);
            if(end_flag==1)
                break;//line : END op
            continue;//line : opcode op 
        }      
    }  

    length = LOCCTR;

    fprintf(f_itr,"%d\n",sym_entity);
    //print sym_table
    for(int i=0;i<sym_entity;i++)
    {
        char loc_16[10];
        sprintf(loc_16,"%x",sym_table1[i].loc);
        fprintf(f_itr,"%s\t%s\n",sym_table1[i].symbol,loc_16);   
    }       

    fclose(f_itr);  
    fclose(f_src);
}

void pass2(void)
{   
    
    f_itr = fopen("intermediate_file.itr","r");

    struct _source_code {
    int type;    
    //1 :label opcode op error
    //2 : label opcode error
    //3 : label error
    //4 :       opcode op error
    //5 :       opcode error
    //6 : comment or space
    char label[20];
    char opcode[20];
    char op1[20];
    char op2[20];
    char error[3];
    //00 : normal
    //10 : duplicate symbol
    //01 : invalid operation
    char temp[255];
    int obj_code[20];
    };

    struct _source_code source_code[100];
    struct _sym_table sym_table2[100];
    int sym_entity=0,source_entity=0,lt_entity=0;
    char temp[255];
    
    while(fgets(temp,255,f_itr))//itr(code) -> source_code struct
    {           
        int double_flag=0, pivot=0;
        if(temp[0]=='1')//label opcode op error
        {
            source_code[source_entity].type=1;
            char* token = strtok(temp,"\t");
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].label,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].opcode,token);
            token = strtok(NULL,"\t");
            for(int i=0;i<strlen(token);i++)
            {
                if(token[i]==',')
                {
                    double_flag=1;
                    pivot=i;
                    break;
                }
            }
            if(double_flag==1)
            {
                strncpy(source_code[source_entity].op1,token,pivot);
                strcpy(source_code[source_entity].op2,token+pivot);
            }
            else
                strcpy(source_code[source_entity].op1,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].error,token);
            source_entity++;            
        }
        else if(temp[0]=='2')//label opcode
        {
            source_code[source_entity].op1[0] = '\0';
            source_code[source_entity].op2[0] = '\0';
            source_code[source_entity].type=2;
            char* token = strtok(temp,"\t");
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].label,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].opcode,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].error,token);
            source_entity++;           
        }
        else if(temp[0]=='3')//label
        {
            source_code[source_entity].opcode[0] = '\0';
            source_code[source_entity].op1[0] = '\0';
            source_code[source_entity].op2[0] = '\0';
            source_code[source_entity].type=3;
            char* token = strtok(temp,"\t");
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].label,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].error,token);
            source_entity++;            
        }        
        else if(temp[0]=='4')//opcode op
        {
            source_code[source_entity].label[0] = '\0';
            source_code[source_entity].type=4;
            char* token = strtok(temp,"\t");
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].opcode,token);
            token = strtok(NULL,"\t");
            for(int i=0;i<strlen(token);i++)
            {
                if(token[i]==',')
                {
                    double_flag=1;
                    pivot=i;
                    break;
                }
            }
            if(double_flag==1)
            {
                strncpy(source_code[source_entity].op1,token,pivot);
                strcpy(source_code[source_entity].op2,token+pivot);
            }
            else
                strcpy(source_code[source_entity].op1,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].error,token);
            source_entity++;           
        }
        else if(temp[0]=='5')//opcode
        {
            source_code[source_entity].label[0] = '\0';
            source_code[source_entity].op1[0] = '\0';
            source_code[source_entity].op2[0] = '\0';
            source_code[source_entity].type=5;
            char* token = strtok(temp,"\t");
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].opcode,token);
            token = strtok(NULL,"\t");
            strcpy(source_code[source_entity].error,token);
            source_entity++;           
        }
        else if(temp[0]=='6')//comment or space
        {
            source_code[source_entity].type=6;            
            strcpy(source_code[source_entity].temp,temp+2);            
            source_entity++;            
        }

        if(strcmp(source_code[source_entity-1].opcode,"END")==0)
            break;            
    }        

    
    //itr(sym_table) -> sym_table2
    fgets(temp,255,f_itr);
    sym_entity = atoi(temp);
    for(int i=0;i<sym_entity;i++)
    {
        fgets(temp,255,f_itr);
        char* token = strtok(temp,"\t");
        strcpy(sym_table2[i].symbol,token);
        token = strtok(NULL,"\t");
        sym_table2[i].loc=atoi(token);           
    }
    
    //write list file
    fclose(f_itr);   
    f_lst = fopen("assembly_listing_file.lst","w");
    f_obj = fopen(object,"w");
    strcpy(text_record,"");
    fprintf(f_lst,"LOC\tSource Statement     \tObject code\tError\n\n");
    char start_label[20];
    int LOCCTR,start_address=0;
    for(int i=0;i<source_entity;i++)
    {
        if(strcmp(source_code[i].opcode,"START")==0)
        {
            strcpy(start_label,source_code[i].label);
            start_address = atoi(source_code[i].op1);
            break;
        }
    }    
    fprintf(f_obj,"H%s\t%06X%06X\n",start_label,start_address,length);
    LOCCTR = start_address;
    int base=-1;
    for(int i=0;i<source_entity;i++)
    {        
        if(strncmp(source_code[i].error,"10",2)==0)//duplicate symbol
        {
            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tDuplicate Symbol\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
        }
        else if(strncmp(source_code[i].error,"01",2)==0)//invalid operation
        {
            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tInvalid Operation\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
        }
        else if(strncmp(source_code[i].error,"11",2)==0)
        {
            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tDuplicate Symbol & Invalid Operation\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
        }
        else
        {     
            int addr=-1;
            int disp;            
            int opc,format=-1;
            if(source_code[i].opcode[0]=='+')
            {
                for(int j=0;j<(sizeof(op_table)/sizeof(struct _op_table));j++)
                {
                    if(strcmp(source_code[i].opcode+1,op_table[j].op)==0)
                    {                         
                        opc = op_table[j].opcode;      
                        format = op_table[j].oplength;                 
                        break;
                    }
                } 
            }
            else
            {
                for(int j=0;j<(sizeof(op_table)/sizeof(struct _op_table));j++)
                {
                    if(strcmp(source_code[i].opcode,op_table[j].op)==0)
                    {                         
                        opc = op_table[j].opcode;      
                        format = op_table[j].oplength;                 
                        break;
                    }
                } 
            }
            
            
            if(format==0)//directive
            {
                if(strcmp(source_code[i].opcode,"BASE")==0)
                {
                    fprintf(f_lst,"\t%s\t%s\t%s%s\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                    for(int j=0;j<sym_entity;j++)//check label is in symbol table
                    {
                        if(strcmp(sym_table2[j].symbol,source_code[i].op1)==0)
                        {               
                            base = sym_table2[j].loc;
                            break;                
                        }            
                    }
                    if(base<0)
                        base = 0;
                    continue;
                }
                else if(strcmp(source_code[i].opcode,"NOBASE")==0)
                {
                    fprintf(f_lst,"\t%s\t%s\t%s%s\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                    base=0;
                    continue;
                }
                else if(strcmp(source_code[i].opcode,"WORD")==0)
                {
                    LOCCTR += 3;
                    if(source_code[i].op1[0]>'A')
                    {
                        for(int j=0;j<sym_entity;j++)//check label is in symbol table
                        {
                            if(strcmp(sym_table2[j].symbol,source_code[j].op1)==0)
                            {               
                                addr = sym_table2[j].loc;
                                break;                
                            }            
                        }
                        if(addr==-1)
                        {
                            LOCCTR -= 3;
                            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tUndefined Symbol\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                            continue;
                        }
                        source_code[i].obj_code[0] = (addr >> 16) & 255;
                        source_code[i].obj_code[1] = (addr >> 8) & 255;
                        source_code[i].obj_code[2] = addr & 255;
                    }
                    else
                    {
                        source_code[i].obj_code[0] = 0;
                        source_code[i].obj_code[1] = (atoi(source_code[i].op1) >> 8) & 255;
                        source_code[i].obj_code[2] = atoi(source_code[i].op1) & 255;
                    }
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X\n",LOCCTR-3,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2]);
                    continue;       
                }
                else if(strcmp(source_code[i].opcode,"RESW")==0)
                {
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\n",LOCCTR,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                    LOCCTR = LOCCTR + (3*atoi(source_code[i].op1));
                    if(text_count>0)
                        writeTEXT();
                    record_start=LOCCTR;
                    continue;
                }    
                else if(strcmp(source_code[i].opcode,"RESB")==0)
                {
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\n",LOCCTR,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                    LOCCTR = LOCCTR + atoi(source_code[i].op1);
                    if(text_count>0)
                        writeTEXT();
                    record_start=LOCCTR;
                    continue;
                } 
                else if(strcmp(source_code[i].opcode,"BYTE")==0)
                {
                    int n;
                    if(source_code[i].op1[0]=='C')
                    {
                        n=0;
                        while(source_code[i].op1[n+2]!='\'')
                        {
                            source_code[i].obj_code[n]=source_code[i].op1[n+2];
                            n++;
                        }
                        source_code[i].obj_code[n] ='\0';
                        LOCCTR += n;
                        if(n==1)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0]);
                        else if(n==2)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X\n",LOCCTR-2,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1]);
                        else if(n==3)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X\n",LOCCTR-3,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2]);
                        else if(n==4)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X%02X\n",LOCCTR-4,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2],source_code[i].obj_code[3]);
                        continue;  
                    }
                    else if(source_code[i].op1[0]=='X')
                    {
                        int len=0,n=0;
                        while(source_code[i].op1[n+2]!='\'')
                        {
                            source_code[i].obj_code[len]=hexstr2dec(source_code[i].op1[n+2])*16 + hexstr2dec(source_code[i].op1[n+3]);
                            n += 2;
                            len++;
                        }
                        source_code[i].obj_code[len] = '\0';
                        LOCCTR += len;
                        if(len==1)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0]);
                        else if(len==2)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X\n",LOCCTR-2,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1]);
                        else if(len==3)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X\n",LOCCTR-3,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2]);
                        else if(len==4)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X%02X\n",LOCCTR-4,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2],source_code[i].obj_code[3]);
                        continue;  
                    }
                    else
                    {
                        source_code[i].obj_code[0]=atoi(source_code[i].op1);
                        LOCCTR++;
                        if(atoi(source_code[i].op1)==1)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0]);
                        else if(atoi(source_code[i].op1)==2)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1]);
                        else if(atoi(source_code[i].op1)==3)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2]);
                        else if(atoi(source_code[i].op1)==4)
                            fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2],source_code[i].obj_code[3]);
                        continue; 
                    }                    
                } 
                else
                {
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\n",LOCCTR,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                    continue;
                }
                
            }

            //4 format
            if(source_code[i].opcode[0]=='+')
            {               
                LOCCTR += 4;                
                if(source_code[i].op1[0]=='#')
                { 
                    source_code[i].obj_code[0] = opc+1;
                    if(source_code[i].op1[1]>='A')
                    {
                        for(int j=0;j<sym_entity;j++)//check label is in symbol table
                        {
                            if(strcmp(sym_table2[j].symbol,source_code[i].op1+1)==0)
                            {               
                                addr = sym_table2[j].loc;
                                break;                
                            }            
                        }
                        if(addr==-1)
                        {
                            LOCCTR -=4;
                            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tUndefined Symbol\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                            continue;
                        }
                        addr -= LOCCTR;
                    }
                    else
                    {
                        addr = atoi(source_code[i].op1+1);
                    }
                    source_code[i].obj_code[1] = 16;
                    source_code[i].obj_code[2] = (addr >> 8) & 255;
                    source_code[i].obj_code[3] = addr & 255;
                }
                else
                {
                    if(source_code[i].op1[0]=='@')
                    {
                        source_code[i].obj_code[0] = opc+2;
                        for(int j=0;j<sym_entity;j++)//check label is in symbol table
                        {
                            if(strcmp(sym_table2[j].symbol,source_code[i].op1+1)==0)
                            {               
                                addr = sym_table2[j].loc;
                                break;                
                            }            
                        }
                    }
                    else
                    {
                        source_code[i].obj_code[0] = opc+3;
                        for(int j=0;j<sym_entity;j++)//check label is in symbol table
                        {
                            if(strcmp(sym_table2[j].symbol,source_code[i].op1)==0)
                            {               
                                addr = sym_table2[j].loc;
                                break;                
                            }            
                        }
                        if(addr==-1)
                        {
                            LOCCTR -=4;
                            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tUndefined Symbol\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                            continue;
                        }
                    }                    
                    if(addr>=0)
                    {
                        source_code[i].obj_code[1] = 16;
                        source_code[i].obj_code[2] = (addr >> 8);
                        source_code[i].obj_code[3] = addr & 255;
                    }
                    else
                    {
                        int tmp;
                        if(source_code[i].op1[0] == '@')
                            tmp = atoi(source_code[i].op1+1);
                        else
                            tmp = atoi(source_code[i].op1);
                        source_code[i].obj_code[1] = 16;
                        source_code[i].obj_code[2] = (tmp >> 8);
                        source_code[i].obj_code[3] = tmp & 255;                        
                    }
                    if(source_code[i].op2[1]=='X')
                        source_code[i].obj_code[1] += 128;                   
                }
                fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X%02X\n",LOCCTR-4,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2],source_code[i].obj_code[3]);
                sprintf(objcode,"%02X%02X%02X%02X",source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2],source_code[i].obj_code[3]);
                strcat(text_record,objcode);
                modcode[mod_entity] = LOCCTR-4+1;
                mod_entity++;
                text_count++;
                if(text_count==10)
                {
                    writeTEXT();
                    record_start=LOCCTR;
                }
            }
            else//1,2,3 format
            {
                if(format==1)//format 1
                {                  
                    LOCCTR += 1;
                    source_code[i].obj_code[0] = opc;                    
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X\n",LOCCTR-1,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0]);
                    sprintf(objcode,"%02X",source_code[i].obj_code[0]);
                    strcat(text_record,objcode);
                    text_count++;
                    if(text_count==10)
                        writeTEXT();
                }
                else if(format==2)//format 2
                {   
                    LOCCTR += 2;
                    source_code[i].obj_code[0] = opc;
                    if(source_code[i].op1[0] < 'A')
                        source_code[i].obj_code[1] = atoi(source_code[i].op1) << 4;
                    else
                    {
                        int reg=-1;
                        if(source_code[i].op1[0]=='A')
                            reg=0;
                        else if(source_code[i].op1[0]=='X')
                            reg=1;
                        else if(source_code[i].op1[0]=='L')
                            reg=2;
                        else if(source_code[i].op1[0]=='B')
                            reg=3;
                        else if(source_code[i].op1[0]=='S')
                            reg=4;
                        else if(source_code[i].op1[0]=='T')
                            reg=5;
                        else if(source_code[i].op1[0]=='F')
                            reg=6;
                        source_code[i].obj_code[1] = reg << 4;
                    }
                    if(source_code[i].op2[0]==',')
                    {
                        if(source_code[i].op2[1] < 'A')
                            source_code[i].obj_code[1] = source_code[i].obj_code[1] | atoi(source_code[i].op2+1);
                        else
                        {
                            int reg=-1;
                            if(source_code[i].op2[1]=='A')
                                reg=0;
                            else if(source_code[i].op2[1]=='X')
                                reg=1;
                            else if(source_code[i].op2[1]=='L')
                                reg=2;
                            else if(source_code[i].op2[1]=='B')
                                reg=3;
                            else if(source_code[i].op2[1]=='S')
                                reg=4;
                            else if(source_code[i].op2[1]=='T')
                                reg=5;
                            else if(source_code[i].op2[1]=='F')
                                reg=6;
                            source_code[i].obj_code[1] = source_code[i].obj_code[1] | reg;
                        }                        
                    }
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X\n",LOCCTR-2,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1]);
                    sprintf(objcode,"%02X%02X",source_code[i].obj_code[0],source_code[i].obj_code[1]);
                    strcat(text_record,objcode);
                    text_count++;
                    if(text_count==10)
                    {
                        writeTEXT();
                        record_start=LOCCTR;
                    }
                }
                else if(format==3)//format 3
                {                    
                    LOCCTR += 3;
                    if(source_code[i].op1[0]=='#')
                    {
                        source_code[i].obj_code[0] = opc+1;
                        if(source_code[i].op1[1] >= 'A')
                        {
                            for(int j=0;j<sym_entity;j++)//check label is in symbol table
                            {
                                
                                if(strcmp(sym_table2[j].symbol,source_code[i].op1+1)==0)
                                {   
                                    addr = sym_table2[j].loc;
                                    break;                
                                }            
                            }
                            if(addr==-1)
                            {
                                LOCCTR -=3;
                                fprintf(f_lst,"\t%s\t%s\t%s%s\t\tUndefined Symbol\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                                continue;
                            }                            
                            addr -= LOCCTR;
                            source_code[i].obj_code[1] = (addr >> 8) & 15;
                            source_code[i].obj_code[1] = source_code[i].obj_code[1] | 32;
                            source_code[i].obj_code[2] = addr & 255;
                        }
                        else
                        {
                            source_code[i].obj_code[1] = (atoi(source_code[i].op1+1) >> 8) & 15;
                            source_code[i].obj_code[2] = atoi(source_code[i].op1+1) & 255;
                        }                        
                    }
                    else if(source_code[i].op1[0]=='\0')
                    {
                        source_code[i].obj_code[0] = opc +3;
                        source_code[i].obj_code[1] = 0;
                        source_code[i].obj_code[2] = 0;
                    }
                    else
                    {
                        if(source_code[i].op1[0]=='@')
                        {
                            source_code[i].obj_code[0] = opc +2;                        
                            for(int j=0;j<sym_entity;j++)//check label is in symbol table
                            {
                                if(strcmp(sym_table2[j].symbol,source_code[i].op1+1)==0)
                                {               
                                    addr = sym_table2[j].loc;
                                    break;                
                                }            
                            }
                            disp = addr - LOCCTR;
                        }                        
                        else
                        {
                            source_code[i].obj_code[0] = opc +3;                        
                            for(int j=0;j<sym_entity;j++)//check label is in symbol table
                            {
                                if(strcmp(sym_table2[j].symbol,source_code[i].op1)==0)
                                {               
                                    
                                    addr = sym_table2[j].loc;
                                    break;                
                                }            
                            }
                            disp = addr - LOCCTR;                            
                        }
                        if(addr==-1)
                        {                           
                            LOCCTR -=3;
                            fprintf(f_lst,"\t%s\t%s\t%s%s\t\tUndefined Symbol\n",source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2);
                            continue;
                        }
                        if((abs(disp)>=4096)&&(addr>=0))
                        {
                            disp = abs(base-addr);
                            source_code[i].obj_code[1] = (disp >> 8) & 15;
                            source_code[i].obj_code[1] = source_code[i].obj_code[1] | 64;
                            source_code[i].obj_code[2] = disp & 255;
                        }
                        else if((disp < 4096) && (addr >=0))
                        {
                            source_code[i].obj_code[1] = (disp >> 8) & 15;
                            source_code[i].obj_code[1] = source_code[i].obj_code[1] | 32;
                            source_code[i].obj_code[2] = disp & 255;
                        }
                        else
                        {
                            int tmp;
                            if(source_code[i].op1[0]=='@')
                                tmp = atoi(source_code[i].op1+1);
                            else
                                tmp = atoi(source_code[i].op1);
                            source_code[i].obj_code[1] = (tmp >> 8);
                            source_code[i].obj_code[2] = tmp & 255;                            
                        }
                        if((source_code[i].op2[1] == 'X') && (source_code[i].op2[0] == ','))
                            source_code[i].obj_code[1] = source_code[i].obj_code[1] + 128;                        
                    }     
                    fprintf(f_lst,"%04X\t%s\t%s\t%s%s\t%02X%02X%02X\n",LOCCTR-3,source_code[i].label,source_code[i].opcode,source_code[i].op1,source_code[i].op2,source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2]);
                    sprintf(objcode,"%02X%02X%02X",source_code[i].obj_code[0],source_code[i].obj_code[1],source_code[i].obj_code[2]);
                    strcat(text_record,objcode);
                    text_count++;
                    if(text_count==10)
                    {
                        writeTEXT();
                        record_start=LOCCTR;
                    }
                }
            }           
        }
    }

    writeTEXT();
    for(int i=0;i<mod_entity;i++)
        fprintf(f_obj,"M%06X05\n",modcode[i]);
    fprintf(f_obj,"E%06X\n",start_address);

    fclose(f_lst);   
    fclose(f_obj);    
}

int main(int argc, char *argv[])
{
    if(argc!=3)
    {
        fprintf(stderr, "Usage: %s source.asm object.obj\n",argv[0]);
        return 0;
    }
    
    strcpy(source,argv[1]);
    strcpy(object,argv[2]);
    
    f_src = fopen(source,"r");
    if(f_src == NULL)
    {
        perror("ERROR");
        return 0;
    }   
    
    pass1();       
    pass2();
   
    return 0;
}