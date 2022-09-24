/*
MIT License

Copyright © 2022 tzrn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define INP_MAX  100 //max chars in whole input including \0
#define COMM_MAX 24  //max chars in command
#define SOURCE_MAX 200
#define ARG_MAX 8 //max amount of arguments
#define ARG_MCH 80 //max amount of charecters in an argument
#define COMLEN 4
#define LABLEN 16 //label length

#ifdef _WIN32
#define CLSC "cls"
#include <Windows.h> //this is for wait (sleep) but it doesnt work on windows for some reason, whatever
#define sleep(s) Sleep(s)
#else
#define CLSC "clear"
#include <unistd.h>
#endif

typedef enum{
	STR=-1,
	CHAR,
	INT,
	REAL
} type_e;

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
	type_e type; // 1 - Int; 2 - Real; 0 - Char; -1 - String; (can be interpreted as any);
} var;

typedef struct{
	var *elems;
	int allocSize;
	int logSize;
} vars;

typedef struct{ //label for goto to go to
	long int name;
	unsigned int linenum;
} label;

typedef struct{
	int startline;
	int retline; //line to return to after subroutine
	long int name;
	vars *locvars;
	char **argnames;
	int argnum;
	var *retvar; //var to return value to
} subroutine;

typedef struct{
	int logSize;
	int allocSize;
	subroutine **subs;
} substack; //stack for subroutines

void initsubs(substack *sstack);
void disposesubs(substack *sstack);
void pushsub(substack *stack, subroutine *sub); //char *args
subroutine *popsub(substack *stack);

long int strtoi(char *, int len);

int type(char *); // guess type
int typesize(int type);

char *stype(int); // conver number of type to name (1=>int)

void init_vars(vars *s);
void dispose_vars(vars *s);
void add_var(vars *s, int elemSize, void *value, char *name, int type);

void parseargs(args *arg);
int check_args(args *, int min_count,...);
var *get_var(vars *varlist,char *name); //search for var struct
void get_str_value(void *addr, char **str, int type); //of a var
var *getvarue(char *value, vars *varlist);

void isum (int *x, int *y){*x+=*y;}
void isub (int *x, int *y){*x-=*y;}
void idivi(int *x, int *y){*x/=*y;}
void imod (int *x, int *y){*x%=*y;}
void iprod(int *x, int *y){*x*=*y;}
void iswap(int *x, int *y){*x=*y;}
void fsum (float *x, float *y){*x+=*y;}
void fsub (float *x, float *y){*x-=*y;}
void fprod(float *x, float *y){*x*=*y;}
void fdivi(float *x, float *y){*x/=*y;}
void fswap(float *x, float *y){*x=*y;}
void ssum (char **x, char **y){*x=realloc(*x,sizeof(char)*80);strcat(*x,*y);}
void sswap(char **x, char **y){free(*x);*x=strdup(*y);} //same thing :(

void (*iop[6])(int *,int *)     = {isum,iswap,isub,idivi,iprod,imod}; //integer operations and so on
void (*fop[5])(float *,float *) = {fsum,fswap,fsub,fdivi,fprod}; //so 0 sum 1 swap 2 sub 3 divi 4 prod
void (*sop[2])(char **,char **) = {ssum,sswap};

void operation(args *arg, vars *varlist, int op); //void (**op)()); <-- there was buch of cringe (gone now)
void chvar(var *value1, var *value2, int op); //void (**change)(void *val1,void *val2));
void exec(int comm, args *arg, vars *gvarlist, vars **currvarlist,int *finger, label *labels,\
		int leblen, substack *sstack, subroutine *subs, int sublen);
//   ^
//   |--- whole action is here

int compare(void *val1, void *val2, int type); // 0 - equel; <0 - less; >0 - more
int get_comm(char *, int *); //return position where args start, int * changed to position of args

int main(int argc, char **argv)
{
	int c,i,comm,slen=0,finger=0;
	char b1; //bx - buffer
	FILE *file;
	char **source;

	label labels[40]; //max labels (put in define?)
	subroutine subs[40];//reapiting code TT ;;

	//realloc and dispose needed will do later
	for(c=0;c<40;c++)subs[c].argnames=malloc(sizeof(char *) * 8);
	substack sstack; 

	int leblen=0; //number of labels
	int sublen=0; //number of subroutines

	initsubs(&sstack);
	//global
	vars gvarlist; // now that i think, some of these could be global
	vars *currvarlist = &gvarlist;
	args arg;     // especially arg and varlist
	args subargbuff;
	
	setbuf(stdout, NULL); //without this, buffer flushes only with \n (bad for wait command)

	if(argc>1)
	{
		if((file=fopen(argv[1],"r"))==NULL)
		{printf("Could not open %s.\n",argv[1]);return 1;}
	}
	else {printf("No input file!\n");return 1;}

	source=malloc(sizeof(char *)*SOURCE_MAX); //max strings in source file
	source[0]=malloc(INP_MAX);

	subargbuff.argv=malloc(sizeof(char *)*ARG_MAX);
	subargbuff.poses=malloc(sizeof(int)*ARG_MAX);

	while(fgets(source[slen],INP_MAX,file)!=NULL)
	{
		switch(*source[slen])
		{
			case '\n': //empty strings
			break;

			case '@': //comments
			break;
	
			case '!': //labels
			labels[leblen].linenum=slen;
			labels[leblen++].name=strtoi(source[slen]+1,LABLEN);		
			//printf("%d  %s - #%d\n",leblen,source[slen],slen);
			break;
	
			case '%': //subroutines
			subargbuff.all=source[slen]+1;
			subs[sublen].startline=slen;
			subs[sublen].argnum=0;

			parseargs(&subargbuff);
			for(c=1;c<subargbuff.argc;c++)
			{
				//printf("adding arg \"%s\"\n",subargbuff.argv[c]);
				subs[sublen].argnames[c-1]=strdup(subargbuff.argv[c]);
				free(subargbuff.argv[c]);
				subs[sublen].argnum++;
			}

			subs[sublen++].name=strtoi(subargbuff.argv[0],LABLEN);
			break;

			default:
			//printf("%d %s",slen,source[slen]);
			source[++slen]=malloc(INP_MAX); //this is dumb!
			break;
		}
	}
	fclose(file);
	if(slen==0){return 0;}

	srand(time(NULL));

	init_vars(&gvarlist);
	arg.argv=malloc(sizeof(char *)*ARG_MAX);
	arg.poses=malloc(sizeof(int)*ARG_MAX);
	char *input=malloc(INP_MAX); //free at the end!

	//for(c=0;c<argc;c++) printf("%d) %s\n",c,argv[c]);

	do {
		//for(c=0; (b1=getchar())!='\n'; input[c++] = b1);
		//printf("finger is on %d %s",finger, source[finger]);
		for(c=0; (b1=source[finger][c])!='\n'; input[c++]=b1); // doin' in manual cause just copypaste of interactive
		input[c]=0; // also replaces \n
		//printf("input - %s\n",input);
		//printf("slen - %d\n",slen);

		comm=get_comm(input,&c);
		//if(comm==0) continue;

		if(c!=-1)
		{
			arg.all=input+c;
			parseargs(&arg); //if some chars after command
		}
		else arg.argc=0;
		//printf("executing %d @ %s\n",comm,arg.all);
		exec(comm,&arg,&gvarlist,&currvarlist,&finger,labels,leblen,&sstack,subs,sublen);
		for(c=0;c<arg.argc;c++)free(arg.argv[c]);
	} while(++finger<slen && finger>0);
	
	for(c=0;c<40;c++)
	{
		for(i=0;c<sublen&&i<subs[c].argnum;i++)
			free(subs[c].argnames[i]);
		free(subs[c].argnames);
	}

	dispose_vars(&gvarlist); //if global needed
	disposesubs(&sstack);
	free(input);
	//FUNCTION FINCTION FUNTION DISPOSE(ARGV/SUBARGBUFF)
	if(sublen>0)free(subargbuff.argv[0]);
	free(subargbuff.argv);
	free(subargbuff.poses);
	free(arg.poses);
	free(arg.argv);
	for(c=0;c<slen+1;c++)free(source[c]);
	free(source);
	return 0;
}

void exec(int comm, args *arg, vars *gvarlist, vars **currvarlist, int *finger, label *labels,\
	       	int leblen, substack *sstack, subroutine *subs, int sublen)
{
	int c,i,min,max,ivarb, newcomm,jepp;
	long int lgo; //to compare with label name
	float fvarb; // buffer for float var
	var *pi, *pi2; //initially was supposed to point to var type (point to int)
	char *sbuf, *sbuf2; //for creating string and for doif if both are not variables (second one is also for anything else)
	args argif; // <-- Example of a bad code design - a variable in case i do doif
	subroutine *wayback;//帰り

	switch(comm) //is it even ok for a switch to be so long?
	{
		default:
		printf("%d: Unknown command: [%d]\n",*finger,comm);
		break;

		case 1667594341: //exec
		system(arg->all);
		break;

		case 1852404336: //prin
		for(c=0;c<arg->argc;c++)
		{
			switch(arg->argv[c][0])
			{
				case '$':
				pi=get_var(*currvarlist,arg->argv[c]+1);
				if(pi!=NULL)
				{
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
		add_var(*currvarlist,sizeof(int),&ivarb,arg->argv[0],1); //vars size value name type
		break;

		case 1818322290: //real
		if(!check_args(arg,2,0,0))break;
		fvarb = atof(arg->argv[1]);
		add_var(*currvarlist,sizeof(float),&fvarb,arg->argv[0],2); //vars size value name type
		break;

		case 1918986339: //char
		if(!check_args(arg,2,0,0))break;
		add_var(*currvarlist,sizeof(char),arg->argv[1],arg->argv[0],0);
		break;

		case 7500915: //str
		if(!check_args(arg,2,0,0))break;
		sbuf=strdup(arg->argv[1]);
		add_var(*currvarlist,sizeof(char *),&sbuf,arg->argv[0],-1); //vars size value name type
		break;

		case 1885435763: //swap //so 0 sum 1 swap 2 sub 3 divi 4 prod
		operation(arg,*currvarlist,1);
		break;
		case 7173491://sum  for the time you can only add real to real and int to int
		operation(arg,*currvarlist,0);
		break;
		case 6452595://sub
		operation(arg,*currvarlist,2);
		break;
		case 7760228://div
		operation(arg,*currvarlist,3);
		break;
		case 1685025392: //prod(uct)
		operation(arg,*currvarlist,4);
                break;
		case 6582125: //mod
		operation(arg,*currvarlist,5);
                break;

		case 1953720684: //list list all variables in current scope and info about them
		for(pi = (*currvarlist)->elems;pi<(*currvarlist)->elems+(*currvarlist)->logSize;pi++)
		{
			printf("#%p \"%s\" [",pi->value,pi->name);
			get_str_value(pi->value,&sbuf2,pi->type);
			printf("%s",sbuf2);
			printf("]\n");
			free(sbuf2);
		}
		break;

		case 1634036835://clea to clear screen (define for win and linux)
		system(CLSC);
		break;

		case 1718185828://doif
		//doif a[0] =[1] b[2] command blah blah[3]
		pi=getvarue(arg->argv[0],*currvarlist); //dont forget to free var
		pi2=getvarue(arg->argv[2],*currvarlist);
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
			exec(newcomm,&argif,gvarlist,currvarlist,finger,labels,leblen,sstack,subs,sublen);
			for(c=0;c<argif.argc;c++)free(argif.argv[c]);
			free(argif.argv);
			free(argif.poses);
		}
		if(pi->name==0) {free(pi->value);free(pi);}
		if(pi2->name==0) {free(pi2->value);free(pi2);}
		break;

		case 1869901671://goto
		if(!check_args(arg,1,0))break;
		lgo=strtoi(arg->argv[0],LABLEN);
		for(i=0;i<leblen;i++)
		{
			if(lgo==labels[i].name)
			{
			//printf("%li = %li\n",lgo,labels[i].name);
			//printf("found %s, jumping to %d\n",arg->argv[0],*finger+3);
			*finger=labels[i].linenum-1;
			goto fi;
			}
			//printf("%li != %li\n",lgo,labels[i].name);
		}
		printf("%d label \"%s\" not found!\n",*finger,arg->argv[0]);
		fi:
		break;

		case 1970499431://gosu(b)
		if(!check_args(arg,1,0))break;
		lgo=strtoi(arg->argv[0],LABLEN);
		for(i=0;i<sublen;i++)
		{
			if(lgo==subs[i].name)
			{
			//void add_var(vars *s, int elemSize, void *value, char *name, int type)
			pushsub(sstack,subs+i);
			subs[i].retline=*finger;
			*finger=subs[i].startline-1;

			for(c=1; c<arg->argc && arg->argv[c][0]!='>'; c++) //convert arguments to local variables
			{
				pi=getvarue(arg->argv[c],NULL);
				add_var(subs[i].locvars,typesize(pi->type),
					pi->value,subs[i].argnames[c-1],pi->type);
				free(pi->value); //this is dumb 'cause i could just giveaway
				//pointer to new var, but instead i copying and freeing this one
				free(pi);
			}
			
			if(c<arg->argc) //if '>' exists
				subs[i].retvar=get_var(*currvarlist,arg->argv[c+1]);
			else
				subs[i].retvar=NULL;

			*currvarlist=subs[i].locvars;
			goto sfi;
			}
		}

		printf("%d subroutine \"%s\" not found!\n",*finger,arg->argv[0]);
		sfi:
		break;

		case 7628146://ret(urn)
		if(arg->argc>0) //return value to the var
		{
			pi=getvarue(arg->argv[0],*currvarlist);

			if(sstack->subs[sstack->logSize-1]->retvar!=NULL)
			memcpy(sstack->subs[sstack->logSize-1]->retvar->value,\
					pi->value,typesize(pi->type));
			else
				printf("Trying to return value altough it doesn't go anwhere");
		}
		wayback=popsub(sstack);	
		
		if(sstack->logSize==0)
			*currvarlist=gvarlist;
		else
			*currvarlist=sstack->subs[sstack->logSize-1]->locvars;

		*finger=wayback->retline;
		break;

		//glob command to go back to global varlist to be able to goto from gosub to main
		//or to use global vars in sub
		case 1651469415://glob
		*currvarlist=gvarlist;	
		break;

		case 6516588://loc go back to local
		if(sstack->logSize==0)
		{
			fprintf(stderr,"loc: attempt to go back to local when not in function\n");
			break;
		}
		*currvarlist=sstack->subs[sstack->logSize-1]->locvars;
		break;

		case 6581861://end
		*finger=-1;
		break;

		case 1684955506://rand
		if(!check_args(arg,2,1,1))break;
		min=atoi(arg->argv[0]);
		max=atoi(arg->argv[1]);
		c=rand()%(max-min+1)+min;
		if(arg->argc==2){printf("%d\n",c);break;}
		pi=get_var(*currvarlist,arg->argv[2]);
		if(pi==NULL){printf("rand: var not found!\n");break;}
		if(pi->type==REAL)
			*((float *)pi->value)=(float)c;
		else
			*((int *)pi->value)=c;
		break;

		case 1851876211://scan
		if(arg->argc==0){getchar();break;}
		pi=get_var(*currvarlist,arg->argv[0]);
		if(pi==NULL){printf("scan: var not found!\n");break;}
		sbuf2=malloc(80);
		c=0;while((sbuf2[c++]=getchar())!='\n');
		sbuf2[c-1]=0;
		switch(pi->type)
		{
			case STR:*((char **)(pi->value))=strdup(sbuf2);break;
			case CHAR:*((char *)(pi->value))=sbuf2[0];break;
			case INT:*((int *)(pi->value))=atoi(sbuf2);break;
			case REAL:*((float *)(pi->value))=atof(sbuf2);break;
		}
		free(sbuf2);
		break;
		
		case 1953063287: //wait
		sleep(atoi(arg->argv[0]));
		break;
	}
}

long int strtoi(char *str, int len) //len - how many of chars to write, others are ignored
{
	long int i,istr=0;
	//printf("len - %d\n",len);
	for(i=0;i<len;i++)
	{
	if(str[i]==' ' || str[i] == '\n' || str[i]==0)break;
	*(((char *)&istr)+i)=str[i];
	}

	return istr;
}

void parseargs(args *arg)
{
	int i=0,c=0;
	char in=0; // in double quotes?
	char *buff=malloc(80); //buffpup lol

	arg->argc=0;
	arg->poses[arg->argc]=i;

	while(arg->all[i]!=0)
	{
		switch(arg->all[i])
		{
			case ' ':
			if(in){buff[c++]=arg->all[i];break;}
			buff[c]='\0';
			c=0;
			arg->argv[arg->argc++]=strdup(buff);
			while(arg->all[i+1]==' ')i++;
			arg->poses[arg->argc]=i+1;
			//printf("arg no.%d starts at pos %d\n",n,arg->poses[n]);
			break;

			case '\n':goto out;

			case '"':
			in=!in;
			break;

			default:
			buff[c++]=arg->all[i];
			break;
		}
		//printf("processed #%d [%c]\n",i,arg->all[i]);
		i++;
	}
	out:
	if(in)c--;
	buff[c]='\0';
	arg->argv[arg->argc++]=strdup(buff);
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

void dispose_vars(vars *s)
{
	int i;
	for(i=0;i<s->logSize;i++)
	{
		//printf("Disposing %s [%d]\n",s->elems[i].name,*((int*)s->elems[i].value));
		if(s->elems[i].type==STR)free(*((char **)s->elems[i].value));
		free(s->elems[i].name);
		free(s->elems[i].value);
	}
	free(s->elems);
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
	
	//printf("var \"%s\" index - %d\n",new->name,s->logSize);
	s -> logSize++;
}

int type(char *str) //guess type by string
{
	int i=0,dot=0,chars=0;
	if(str[i]=='-')
	{
		if(str[i+1]==0)return 0; else i++; // - sign
	}

	for(;str[i]!='\0';i++)
	{
		if(str[i]<48 || str[i]>57)
		{
			if(str[i]==46) //.
			{
				if(dot++==2)break;
			}
			else
				if(chars++==2)break;
		}
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
	return -1;
}

int typesize(int type)
{
	switch(type)
	{
		case INT: return sizeof(int);
		case CHAR: return sizeof(char);
		case STR: return sizeof(char*);
		case REAL: return sizeof(float);
	}
	return 0;
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
	return "unknown";
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
		{
			if(same!=type(arg->argv[i]))
			{
				printf("Wrong argument type!\n");
				va_end(types);
				return 0;
			}
			else
				same=type(arg->argv[i]);
		}

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
	if(type==STR)
	{
		*str=strdup(*((char **)addr));
		return;
	}
	
	*str=malloc(sizeof(char)*ARG_MCH);
	switch(type)
	{
		case CHAR:sprintf(*str,"%c",*((char *)(addr)));break;
		case INT:sprintf(*str,"%d",*((int *)(addr)));break;
		case REAL:sprintf(*str,"%.3f",*((float *)(addr)));break;
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
		//printf("int chvar 1- %s\n",*((char**)value1->value));
		//printf("int chvar 2- %s\n",*((char**)value2->value));
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
	return 0;
}

int get_comm(char *input, int *c)
{
	int s=0, b=0, comm; //str counter and buff counter
	char *buff=malloc(COMM_MAX);

	if(*input==0){free(buff);return 0;}
	while(input[s]==' ')s++; //skip spaces

	while(input[s]!=' ' && input[s]!=0) //FOR FOR FOR?
	{
		buff[b]=input[s];
		b++;s++;
	}

	buff[b]=0;
	//printf("COMMAND BUFF - %s\n",buff);
	comm = strtoi(buff,COMLEN);
	//printf("COMMAND CODE -%d\n",comm);
	free(buff);

	while(input[s]==' ')s++;
	if(input[s]==0) 
		*c = -1; //no args
	else
		*c = s; //beginning of args

	return comm;
}
					//if given, write in that
var *getvarue(char *value, vars *varlist) //if var (starts with $), returns it, esle creates and returns var struct with guessed value
{
	var *pi;
	void *buff;

	if(varlist!=NULL)
	{
		if(value[0]=='$')
			if((pi=get_var(varlist,value+1))!=0) return pi;
	}

	pi=malloc(sizeof(var));
	pi->type=type(value);

	switch(pi->type)
	{
		case INT:buff=malloc(sizeof(int));*((int*)buff) = atoi(value);break;
		case REAL:buff=malloc(sizeof(float));*((float*)buff) = atof(value);break;
		case CHAR:buff=malloc(sizeof(char));*((char*)buff) = value[0];break;
		case STR:buff=malloc(sizeof(char**));*((char**)buff)=strdup(value);break;
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
        if(pi==NULL){printf("operation: var \"%s\" found!\n",arg->argv[0]);return;}
        chvar(pi,pi2,op); //op);
	if(pi2->name==0){
		if(pi2->type==STR)free(*((char **)pi2->value));
		free(pi2->value);free(pi2);
	}
}

void initsubs(substack *sstack){ //same as init vars (reapiting code TT)
	sstack -> logSize = 0;
	sstack -> allocSize = 4;

	sstack -> subs = malloc(sizeof(subroutine)*sstack->allocSize);
}
void disposesubs(substack *sstack){
	free(sstack->subs);
	//cycle for freeing args
}

void pushsub(substack *stack, subroutine *sub) //char *args
{
	if(stack->logSize > stack->allocSize)
	{
		stack->allocSize+=4;
		stack->subs=realloc(stack->subs,stack->allocSize*sizeof(subroutine*));
	}

	stack->subs[stack->logSize]=sub;
	stack->subs[stack->logSize]->locvars=malloc(sizeof(vars));
	init_vars(stack->subs[stack->logSize]->locvars);
	stack->logSize++;
}

subroutine *popsub(substack *stack)
{
	if(stack->logSize==0)
	{
		printf("attempt to pop empty substack\n");
		return NULL;
	}

	stack->logSize--;
	dispose_vars(stack->subs[stack->logSize]->locvars);
	free(stack->subs[stack->logSize]->locvars);
	return stack->subs[stack->logSize];
}
