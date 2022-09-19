#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define INP_MAX  100 //max chars in whole input including \0
#define COMM_MAX 24  //max chars in command
#define SOURCE_MAX 200

#ifdef _WIN32
#define CLSC "cls"
#include <Windows.h> //this is for wait (sleep) but it doesnt work on windows for some reason, whatever
#else
#define CLSC "clear"
#include <unistd.h>
#endif

#define STR -1
#define CHAR 0
#define INT 1
#define REAL 2

typedef struct{
	int argc;
	char **argv;
	int *poses; //starting position of each argument
	char *all;
} args;

typedef struct{
	char *name;
	void *value;
	int varSize; // never used but would be wasted any way 'cause of padding
	int type; // 1 - Int; 2 - Real; 0 - Char; -1 - String; (can be interpreted as any);
} var;

typedef struct{
	var *elems;
	int allocSize;
	int logSize;
} vars;

int strsame(char *, char *);
int strtoi(char *);

int type(char *); // guess type
char *stype(int); // conver number of type to name (1=>int)

void init_vars(vars *s);
void add_var(vars *s, int elemSize, void *value, char *name, int type);

void parseargs(args *arg);
int check_args(args *, int min_count,...);
var *get_var(vars *varlist,char *name); //search for var struct
void get_str_value(void *addr,char **str, int type); //of a var
var *getvarue(char *value, vars *varlist);

//void ioper(void *x,void *y,void *(*op)()){op( *((int *)x), *((int *)y));}

void isum (int *x, int *y){*x+=*y;}
void isub (int *x, int *y){*x-=*y;}
void idivi(int *x, int *y){*x/=*y;}
void iprod(int *x, int *y){*x*=*y;}
void iswap(int *x, int *y){*x=*y;}
void fsum (float *x, float *y){*x+=*y;}
void fsub (float *x, float *y){*x-=*y;}
void fprod(float *x, float *y){*x*=*y;}
void fdivi(float *x, float *y){*x/=*y;}
void fswap(float *x, float *y){*x=*y;}
//previous string is unfreed, floating //somewhere in the heep :(
void ssum (char **x, char **y){char *buff=strdup(strcat(*x,*y));*x=buff;}				   
void sswap(char **x, char **y){*x=strdup(*y);} //same thing :(

void (*iop[5])(int *,int *)     = {isum,iswap,isub,idivi,iprod}; //integer operations and so on
void (*fop[5])(float *,float *) = {fsum,fswap,fsub,fdivi,fprod}; //so 0 sum 1 swap 2 sub 3 divi 4 prod
void (*sop[2])(char **,char **) = {ssum,sswap};

void operation(args *arg, vars *varlist, int op); //void (**op)()); <-- there was buch of cringe (gone now)
void chvar(var *value1, var *value2, int op); //void (**change)(void *val1,void *val2));
void exec(int comm, args *arg, vars *varlist, char **source, int *finger); // <--- whole action is here

int compare(void *val1, void *val2, int type); // 0 - equel; <0 - less; >0 - more
int get_comm(char *, int *); //return position where args start, int * changed to position of args

int main(int argc, char **argv)
{
	int c,comm,slen=0,finger=0;
	char b1; //bx - buffer
	time_t ti;
	FILE *file;
	char **source;
	vars varlist; // now that i think, some of these could be global
	args arg;     // especially arg and varlist
	
	setbuf(stdout, NULL); //without this, buffer flushes only with \n (bad for wait command)
	if(argc>1)
	{
		if((file=fopen(argv[1],"r"))==NULL)
		{printf("File %s does not exist!\n",argv[1]);return 1;}
	}
	else {printf("No input file!\n");return 1;}
	source=malloc(sizeof(char *)*SOURCE_MAX); //max strings in source file
	source[0]=malloc(INP_MAX);
	while(fgets(source[slen++],INP_MAX,file)!=NULL)source[slen]=malloc(INP_MAX); //this is dumb!
	fclose(file);								       //I need to get rid of emty lines at
										       //THIS STAGE!!!!!
	srand(time(&ti));

	init_vars(&varlist);
	arg.all=malloc(60);
	arg.argv=malloc(sizeof(char *)*8);
	arg.poses=malloc(sizeof(int)*8);

	char *input=malloc(INP_MAX); //free at the end!

	//for(c=0;c<argc;c++) printf("%d) %s\n",c,argv[c]);

	do {
		//printf("->: "); //invitation

		//for(c=0; (b1=getchar())!='\n'; input[c++] = b1);
		for(c=0; (b1=source[finger][c])!='\n'; input[c++]=b1); // doin' in manual cause just copypaste of interactive
		input[c]=0; // also replaces \n
		//printf("slen - %d\n",slen);

		comm=get_comm(input,&c);
		if(comm==0) continue;

		if(c!=-1)
		{
			arg.all=input+c;
			parseargs(&arg); //if some chars after command
		}
		else arg.argc=0;

		exec(comm,&arg,&varlist,source,&finger);
	} while(finger++<slen-2); //quit
	
	//printf("Goodbye, roma.\n");
	free(input);
	return 0;
}

void exec(int comm, args *arg, vars *varlist, char **source, int *finger)
{
	int c,min,max,ivarb, newcomm,jepp;
	float fvarb; // buffer for float var
	char gch; // buffer for getchar
	var *pi, *pi2; //initially was supposed to point to var type (point to int)
	char *sbuf, *sbuf2; //for creating string and for doif if both are not variables (second one is also for anything else)
	args argif,argo; // <-- Example of a bad code design - a variable in case i do doif and argo

	switch(comm) //is it even ok for a switch to be so long?
	{
		default:
		printf("Unknown command: [%d]\n", comm);
		break;

		case 1953068401:break;//quit

		case 1667594341: //exec
		system(arg->all);
		break;

		case 1852404336: //prin
		for(c=0;c<arg->argc;c++)
		{
			switch(arg->argv[c][0])
			{
				case '$':
				pi=get_var(varlist,arg->argv[c]+1);
				if(pi!=NULL)
				{
					sbuf2=malloc(sizeof(void *));
					get_str_value(pi->value,&sbuf2,pi->type);
					printf("%s",sbuf2);
					free(sbuf2);
					continue;
				} else printf("%s",arg->argv[c]);
				break;
				case '`':
				printf("\n");
				break;
				default:
				printf("%s",arg->argv[c]);
				break;
			}
		}
		break;

		case 1701869940: //type
		printf("%s => %s\n",arg->argv[0],stype(type(arg->argv[0])));
		break;

		case 7630441: //int
		if(!check_args(arg,2,0,0))break;
		ivarb = atoi(arg->argv[1]);
		add_var(varlist,sizeof(int),&ivarb,arg->argv[0],1); //vars size value name type
		break;

		case 1818322290: //real
		if(!check_args(arg,2,0,0))break;
		fvarb = atof(arg->argv[1]);
		add_var(varlist,sizeof(float),&fvarb,arg->argv[0],2); //vars size value name type
		break;

		case 1918986339: //char
		if(!check_args(arg,2,0,0))break;
		add_var(varlist,sizeof(char),arg->argv[1],arg->argv[0],0);
		break;

		case 7500915: //str
		if(!check_args(arg,2,0,0))break;
		sbuf=malloc(sizeof(char *));
		sbuf=strdup(arg->argv[1]);
		add_var(varlist,sizeof(char *),&sbuf,arg->argv[0],-1); //vars size value name type
		break;

		case 1885435763: //swap //so 0 sum 1 swap 2 sub 3 divi 4 prod
		operation(arg,varlist,1);
		break;
		case 7173491://sum  for the time you can only add real to real and int to int
		operation(arg,varlist,0);
		break;
		case 6452595://sub
		operation(arg,varlist,2);
		break;
		case 7760228://div
		operation(arg,varlist,3);
		break;
		case 1685025392: //prod(uct)
		operation(arg,varlist,4);
                break;

		case 1953720684: //list list all variables and info about them
		for(pi = varlist->elems;pi<varlist->elems+varlist->logSize;pi++)
		{
			printf("#%x -> %s (%s) [",pi,pi->name,stype(pi->type));
			sbuf2=malloc(sizeof(void *));
			get_str_value(pi->value,&sbuf2,pi->type);
			printf("%s",sbuf2);
			printf("]\n");
		}
		break;

		case 1634036835://clea to clear screen (define for win and linux)
		system(CLSC);
		break;

		case 1718185828://doif
		//doif a[0] =[1] b[2] command blah blah[3]
		pi=getvarue(arg->argv[0],varlist); //dont forget to free var
		pi2=getvarue(arg->argv[2],varlist);
		//printf("%c %c %c\n",*((char *)pi->value),*arg->argv[1],*((char *)pi2->value));

		jepp = 0;
		switch(arg->argv[1][0])
		{
			case '=':
			if(compare(pi->value,pi2->value,pi->type)==0) jepp = 1;
			break;
			case '>':
			if(compare(pi->value,pi2->value,pi->type)>0) jepp = 1;
			break;
			case '<':
			if(compare(pi->value,pi2->value,pi->type)<0) jepp = 1;
			break;
		}
		if(jepp==1)
		{
			newcomm=get_comm((arg->all)+(arg->poses[3]),&c);
			argif.all=arg->all+arg->poses[3]+c;
			argif.argv=malloc(sizeof(char *)*8);
			argif.poses=malloc(sizeof(int)*8);
			parseargs(&argif); //<-- freeeee
			exec(newcomm,&argif,varlist,source,finger);
			free(argif.argv);
			free(argif.poses);
		}
		if(pi->name==0) free(pi->value);
		if(pi2->name==0) free(pi2->value);
		break;

		case 1869901671://goto
		if(!check_args(arg,1,0))break;
		pi=getvarue(arg->argv[0],varlist);
		*finger=*((int *)pi->value)-2; //it sould be - 1 couse of numeration from 0, but since i execute
//					     // one of them, it aint nessesery so
//		//ok, after testing, comes out i was wrong, i do need to do - 1 (-2 even)
		//there was a bunch of code here that also executed the command (stupid!)
		break;

		case 1684955506://rand
		if(!check_args(arg,2,1,1))break;
		min=atoi(arg->argv[0]);
		max=atoi(arg->argv[1]);
		c=rand()%(max-min+1)+min;
		if(arg->argc==2){printf("%d\n",c);break;}
		pi=get_var(varlist,arg->argv[2]);
		if(pi==NULL){printf("rand: var not found!\n");break;}
		if(pi->type==2)
			*((float *)pi->value)=(float)c;
		else
			*((int *)pi->value)=c;
		break;

		case 1851876211://scan
		if(arg->argc==0){getchar();break;}
		pi=get_var(varlist,arg->argv[0]);
		if(pi==NULL){printf("scan: var not found!\n");break;}
		sbuf2=malloc(80);
		c=0;while((sbuf2[c++]=getchar())!='\n');
		sbuf2[c-1]=0;
		switch(pi->type)
		{
			case STR:*((char **)(pi->value))=strdup(sbuf2);break;
			case CHAR:*((char *)(pi->value))=sbuf2[0];break;
			case INT:*((int *)(pi->value))=atoi(sbuf2);break;
			case REAL:*((float *)(pi->value))=atof(strdup(sbuf2));break;
		}
		break;
		
		case 1953063287: //wait
		sleep(atoi(arg->argv[0])); //usleep take microseconds
		break;
	}
}

int strsame(char *str1, char *str2)
{
	int i;
	for(i=0;str1[i]!='\0' && str2[i] !='\0';i++)
		if(str1[i]!=str2[i]) return 0;
	return 1;
}

int strtoi(char *str)
{
	int i,istr=0;
	for(i=0;i<4;i++)
	{
	if(str[i]==' ')break;
	*(((char *)&istr)+i)=str[i];
	}

	return istr;
}

void parseargs(args *arg)
{
	int i=0,c=0,n=0;
	char in=0; // in double quotes?
	arg->argc=0;
	char *buff=malloc(80); //buffpup lol
	arg->poses[n]=i;

	while(arg->all[i]!=0)
	{
		switch(arg->all[i])
		{
			case ' ':
			if(in){buff[c++]=arg->all[i];break;}
			buff[c]='\0';
			c=0;
			arg->argv[n++]=strdup(buff);
			arg->argc++;
			while(arg->all[i+1]==' ')i++;
			arg->poses[n]=i+1;
			//printf("arg no.%d starts at pos %d\n",n,arg->poses[n]);
			break;

			case '"':
			in=(in)?0:1;
			break;

			default:
			buff[c++]=arg->all[i];
			break;
		}
		//printf("processed #%d [%c]\n",i,arg->all[i]);
		i++;
	}
	if(in)c--;
	buff[c]='\0';
	arg->argv[n]=strdup(buff);
	arg->argc++;
	free(buff);
}

void init_vars(vars *s)
{
	s -> allocSize = 4;
	s -> logSize = 0;
	//printf("initial varlist allocation -> allocating [%d] bytes\n",sizeof(var) * (s -> allocSize));
	s -> elems = malloc(sizeof(var) * (s -> allocSize));
	assert(s -> elems != NULL);
}

void add_var(vars *s, int elemSize, void *value, char *name, int type)
{
	if(s -> allocSize == s -> logSize) //reallocation if out of memory
	{
		s -> allocSize += 4;
		s -> elems = realloc(s -> elems, (s -> allocSize)*sizeof(var));
		assert(s -> elems != NULL);
	}

	var *new = s -> elems + s -> logSize;
	new -> varSize = elemSize;
	new -> name = strdup(name);
	new -> value = malloc(elemSize);
	new -> type = type;
	assert(new -> value != NULL);
	memcpy(new -> value, value, elemSize);
	
	s -> logSize++;
}

int type(char *str) //guess type by string
{
	int i=0,dot=0,chars=0;
	if(str[i]=='-')
		if(str[i+1]==0)return 0; else i++; // - sign

	for(;str[i]!='\0';i++)
	{
		if(str[i]<48 || str[i]>57)
			if(str[i]==46) //.
			{
				if(dot++==2)break;
			}
			else
				if(chars++==2)break;
	}
	if(!chars)
		switch(dot)
		{
			case 0: return 1; //int
			case 1: return 2; //real
		}
	else
		if(chars==1)
			return 0;
		else
			return -1;
}

char *stype(int type)
{
	switch(type)
	{
		case STR: return "str";
		case CHAR: return "char";
		case INT: return "int";
		case REAL: return "real";
	}
}

int check_args(args *arg, int min, ...) // 1 - same 0 - any
{
	int i;
	va_list types;
	va_start(types, min);
	int same; //type that all args sould be

	if(arg->argc < min)
	{
		printf("Not enough arguments!\n");
		va_end(types);
		return 0;
	}
	
	for(i=0;(same=va_arg(types,int))==0;i++) //skip any at the beginning
		if(i<=arg->argc)return 1;
	same=type(arg->argv[i++]);
	for(;i<=arg->argc;i++)
		if(va_arg(types,int)==1)
			if(same!=type(arg->argv[i]))
			{
				printf("Wrong argument type!\n");
				va_end(types);
				return 0;
			}
			else
				same=type(arg->argv[i]);

	va_end(types);
	return 1;
}

var *get_var(vars *varlist,char *name) //search for var struct
{
	for(int i=0;i<varlist->logSize;i++)
		if(!strcmp(name,varlist->elems[i].name))
			return &varlist->elems[i];
	return NULL;
}

void get_str_value(void *addr,char **str, int type) //covert value of a var to string
{
	switch(type)
	{
		case STR: *str=*((char **)addr);break;
		case CHAR: sprintf(*str,"%c",*((char *)(addr)));break;
		case INT: sprintf(*str,"%d",*((int *)(addr)));break;
		case REAL: sprintf(*str,"%.3f",*((float *)(addr)));break;
	}
}

void chvar(var *value1, var *value2,int op) //void (**change)(void *val1,void *val2))
{
	switch(value1->type)
	{
		case INT:
		(iop)[op](value1->value,value2->value);
		break;

		case REAL:
		(fop)[op](value1->value,value2->value);
		break;

		case STR: //str, sum etc.
		(sop)[op](value1->value,value2->value);
		return;

		default:
		printf("not implemented\n");
		return;
	}
}

int compare(void *val1, void *val2, int type) // 0 - equel; <0 - less; >0 - more
{
	float a;
	switch(type)
	{
		case INT:return *((int*)val1)-*((int*)val2);
		case REAL:a=*((float*)val1)-*((float*)val2); //cant do subtraction 'cause it will convert to int and cut right part
		if(a==0)return 0;				   //i could make function float but would that be better?
		else if(a<0)return -1;
		else return 1;
		case STR:return strcmp((char*)val1,(char*)val2);
		case CHAR:return *((char*)val1)-*((char*)val2);
	}
}

int get_comm(char *input, int *c)
{
	int s=0, b=0, comm; //str counter and buff counter
	char *buff=malloc(COMM_MAX);

	if(*input==0)return 0;
	while(input[s]==' ')s++; //skip spaces

	while(input[s]!=' ' && input[s]!=0) //FOR FOR FOR?
	{
		buff[b]=input[s];
		b++;s++;
	}

	buff[b]=0;
	comm = strtoi(buff);
	free(buff);

	while(input[s]==' ')s++;
	if(input[s]==0) 
		*c = -1; //no args
	else
		*c = s; //beginning of args

	return comm;
}

var *getvarue(char *value, vars *varlist) //if var (starts with $), returns it, esle creates and returns var struct with guessed value
{
	var *pi;
	void *buff;
	void *str;
	if(value[0]=='$')
		if((pi=get_var(varlist,value+1))!=0) return pi;

	pi=malloc(sizeof(pi));
	pi->type=type(value);

	switch(pi->type)
	{
		case INT:buff=malloc(sizeof(int));*((int*)buff) = atoi(value);break;
		case REAL:buff=malloc(sizeof(float));*((float*)buff) = atof(value);break;
		case CHAR:buff=malloc(sizeof(char));*((char*)buff) = value[0];break;
		case STR:str=strdup(value);buff=&str;break;
		default: printf("error getting value\n");
	}
	pi->name=0;
	pi->value=buff;
	return pi;
}

void operation(args *arg, vars *varlist, int op) //void (**op)())
{
	var *pi,*pi2;
	if(!check_args(arg,2,0,0))return;
        pi=get_var(varlist,arg->argv[0]);
	pi2=getvarue(arg->argv[1],varlist);
        if(pi==NULL){printf("operation: var not found!\n");return;}
        chvar(pi,pi2,op); //op);
	if(pi2->name==0 && pi->type!=-1) free(pi2->value);
}
