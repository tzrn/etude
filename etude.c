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
	REAL,
	IARR
} type_e;

typedef struct{
	int argc;
	char **argv;
	int *poses; //starting position of each argument
	char *all;
} args;

typedef struct{
	int comm;
	args *arg;
} command;

typedef struct{
	char *name;
	void *value;
	int varSize; // never used but would be wasted any way 'cause of padding
	type_e type; // 1 - Int; 2 - Real; 0 - Char; -1 - String; (can be interpreted as any);
	int useindex; // current index being used if array
} var;

typedef struct{
	var *elems;
	int allocSize;
	int logSize;
} vars;

typedef struct{ //label for goto to go to
	long int name;
	unsigned int linenum;
	int doifacc;
} label;

typedef struct{
	int ifline;
	int elseline;
	int finline;
	int nests[2]; //how many nested in if and else
} doif;

typedef struct{
	int startline;
	int retline; //line to return to after subroutine
	long int name;
	vars *locvars;
	char **argnames;
	int argnum;
	int doifacc;
	var *retvar; //var to return value to
	doif doifs[40]; //max conditions in an function (make dynamic?)
	int doifsnum;
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
void callsub(char *name,int *finger,vars **currvarlist,substack *sstack, subroutine *subs, int sublen, args *arg);

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
char **split(char *str,char del);

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
void exec(command *code, vars **currvarlist,int *finger, label *labels,int *doifacc,\
		int leblen, substack *sstack, subroutine *subs, int sublen);
//   ^
//   |--- whole action is here

int compare(void *val1, void *val2, int type); // 0 - equel; <0 - less; >0 - more
int get_comm(char *, int *); //return position where args start, int * changed to position of args
char *skipspaces(char *str);

int main(int argc, char **argv)
{
	int c,i,n,codlen,slen=0,finger,commbuff,doifc=0;
	FILE *file;
	char **source, *buff;

	label labels[40]; //max labels (put in define?)
	subroutine subs[40];//reapiting code TT ;;
	command *code;

	//realloc and dispose needed will do later
	for(c=0;c<40;c++)subs[c].argnames=malloc(sizeof(char *) * 8);
	substack sstack; 

	int leblen=0; //number of labels
	int sublen=0; //number of subroutines
	int doifqueue[40];
	int doifacc;

	initsubs(&sstack);
	vars *currvarlist; // now that i think, some of these could be global
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

	while(fgets(source[slen],INP_MAX,file)!=NULL)
	{
		switch(*source[slen]) //add support for spaces and tabs at the beginning
		{
			case '\n': //empty strings
			break;
	
			default:
			//printf("%d %s",slen,source[slen]);
			source[++slen]=malloc(INP_MAX); //this is dumb!
			break;
		}
	}
	fclose(file);
	if(slen==0){return 0;}

	subargbuff.argv=malloc(sizeof(char *)*ARG_MAX);
	subargbuff.poses=malloc(sizeof(int)*ARG_MAX);

 	code = malloc(sizeof(command) * slen);
	codlen = 0;
	doifc=0;

	for(i=0;i<slen;i++)
	{
		buff=skipspaces(source[i]);
		//printf("PARSING - %s",buff);
		//printf("First char - %c\n",*buff);
		
		switch(*buff)
		{
			case '@':
			break;

			case '!': //labels
			//printf("%d  %s - #%d\n",leblen,source[i],slen);
			labels[leblen].linenum=codlen;
			labels[leblen].doifacc=subs[sublen-1].doifsnum;
			labels[leblen++].name=strtoi(source[i]+1,LABLEN);		
			break;
	
			case '%': //subroutines
			subargbuff.all=source[i]+1;
			//printf("i = %d;  codlen - %d\n",i,codlen);
			subs[sublen].startline=codlen;
			subs[sublen].argnum=0;
			subs[sublen].doifsnum=0;
			subs[sublen].doifacc=0;

			parseargs(&subargbuff);
			for(c=1;c<subargbuff.argc;c++)
			{
				//printf("adding arg \"%s\"\n",subargbuff.argv[c]);
				subs[sublen].argnames[c-1]=strdup(subargbuff.argv[c]);
				//printf("CLEARING \"%s\"\n",subargbuff.argv[c]);
				free(subargbuff.argv[c]);
				subs[sublen].argnum++;
			}

			//printf("Adding subroutine %s\n",subargbuff.argv[0]);
			subs[sublen++].name=strtoi(subargbuff.argv[0],LABLEN);
			free(subargbuff.argv[0]);
			break;

			default:
			commbuff=get_comm(buff,&c);
			switch(commbuff)
			{
				case 1718185828: //doif
				//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA!!!!!
				//printf("INIIALIZING %d\n",subs[sublen-1].doifsnum);
				subs[sublen-1].doifs[subs[sublen-1].doifsnum].ifline=codlen;
				subs[sublen-1].doifs[subs[sublen-1].doifsnum].elseline=-1;
				subs[sublen-1].doifs[subs[sublen-1].doifsnum].nests[0]=0;
				subs[sublen-1].doifs[subs[sublen-1].doifsnum].nests[1]=0;
				//printf("doif. num - %d, line - %d\n",subs[sublen-1].doifsnum,codlen);
				doifqueue[doifc++]=subs[sublen-1].doifsnum;
				subs[sublen-1].doifsnum++;
				break;

				case 1702063205: //else
				subs[sublen-1].doifs[doifqueue[doifc-1]].elseline=codlen;
				/*printf("else. num - %d, line - %d\n",\
					doifqueue[doifc-1],codlen);*/
				break;

				case 7235942: //fin doifqueue is bsically a stack
				for(n=0;n<doifc-1;n++)
				//how many loops are nested in if and else
				if(subs[sublen-1].doifs[doifqueue[n]].elseline==-1)
				subs[sublen-1].doifs[doifqueue[n]].nests[0]++;
				else
				subs[sublen-1].doifs[doifqueue[n]].nests[1]++;


				subs[sublen-1].doifs[doifqueue[doifc-1]].finline=codlen;
				/*printf("fin. num - %d, line - %d\n",\
					doifqueue[doifc-1],codlen);*/
				doifc--;
				break;
			}
			code[codlen].comm=commbuff;
	
			if(c!=-1)
			{
				code[codlen].arg=malloc(sizeof(args));
				code[codlen].arg->argv=malloc(sizeof(char *)*ARG_MAX);
				code[codlen].arg->poses=malloc(sizeof(int)*ARG_MAX);
				code[codlen].arg->all=buff+c;
				parseargs(code[codlen].arg); //if some chars after command
			} else code[codlen].arg=NULL;
			//printf("%d. %s\n",codlen,source[i]);
			codlen++;
			break;
		}
	}

	for(i=0;i<slen+1;i++)
	free(source[i]);

	//for(i=0;i<sublen;i++)
	//for(n=0;n<subs[i].doifsnum;n++)
	//printf("COND N.%d - nedsted in if %d | in else  %d\n",n,subs[i].doifs[n].nests[0],subs[i].doifs[n].nests[1]);

	srand(time(NULL));

	callsub("main",&finger,&currvarlist,&sstack,subs,sublen,NULL);
	doifacc=0; //sstack.subs[0]->doifacc; //0
	finger++;
	//init_vars(&gvarlist);

	//for(c=0;c<argc;c++) printf("%d) %s\n",c,argv[c]);

	do {
		exec(code,&currvarlist,&finger,labels,&doifacc,leblen,&sstack,subs,sublen);
		finger++;
	} while(sstack.logSize>0);//++finger<slen && finger>0);


	for(i=0;i<codlen;i++)
	{
		if(code[i].arg!=NULL)
		{
		for(c=0;c<code[i].arg->argc;c++)
			free(code[i].arg->argv[c]);
		free(code[i].arg->argv);
		free(code[i].arg->poses);
		free(code[i].arg);
		}
	}
	
	for(c=0;c<40;c++)
	{
		for(i=0;c<sublen&&i<subs[c].argnum;i++)
			free(subs[c].argnames[i]);
		free(subs[c].argnames);
	}

	disposesubs(&sstack);
	free(code);
	//FUNCTION FINCTION FUNTION DISPOSE(ARGV/SUBARGBUFF)
	free(subargbuff.argv);
	free(subargbuff.poses);
	//for(c=0;c<slen+1;c++)free(source[c]);
	free(source);
	return 0;
}

void exec(command *code, vars **currvarlist, int *finger, label *labels,int *doifacc,\
	       	int leblen, substack *sstack, subroutine *subs, int sublen)
{
	int c,i,min,max,ivarb,jepp;
	long int lgo; //to compare with label name
	float fvarb; // buffer for float var
	var *pi, *pi2; //initially was supposed to point to var type (point to int)
	char *sbuf, *sbuf2; //for creating string and for doif if both are not variables (second one is also for anything else)
	subroutine *wayback;//帰り道

	int comm = code[*finger].comm;
	args *arg = code[*finger].arg;

	switch(comm) //is it even ok for a switch to be so long?
	{
		default:
		printf("%d: Unknown command: [%d]\n",*finger,comm);
		break;

		case 1667594341: //exec
		system(arg->all);
		break;

		case 1852404336: //prin# <-for search
		for(c=0;c<arg->argc;c++)
		{
			switch(arg->argv[c][0])
			{
				case '$':case '/':
				pi=getvarue(arg->argv[c],*currvarlist);
				get_str_value(pi->value,&sbuf2,pi->type);
				printf("%s",sbuf2);
				free(sbuf2);
				continue;
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
		if(!check_args(arg,1,0))break;
		printf("%s => %s\n",arg->argv[0],stype(type(arg->argv[0])));
		break;

		case 7630441: //int
		if(!check_args(arg,2,0,0))break;
		ivarb = atoi(arg->argv[1]);
		add_var(*currvarlist,sizeof(int),&ivarb,arg->argv[0],INT); //vars size value name type
		break;

		case 1818322290: //real
		if(!check_args(arg,2,0,0))break;
		fvarb = atof(arg->argv[1]);
		add_var(*currvarlist,sizeof(float),&fvarb,arg->argv[0],REAL); //vars size value name type
		break;

		case 1918986339: //char
		if(!check_args(arg,2,0,0))break;
		add_var(*currvarlist,sizeof(char),arg->argv[1],arg->argv[0],CHAR);
		break;

		case 7500915: //str
		if(!check_args(arg,2,0,0))break;
		sbuf=strdup(arg->argv[1]);
		add_var(*currvarlist,sizeof(char *),&sbuf,arg->argv[0],STR); //vars size value name type
		break;
		case 1920098665: //iarr
		if(!check_args(arg,2,0,0))break;
		add_var(*currvarlist,sizeof(int *)*atoi(arg->argv[1]),NULL,arg->argv[0],IARR); //vars size value name type
		break;

		case 1885435763: //swap //so 0 sum 1 swap 2 sub 3 divi 4 prod
		//printf("Swapping, args - %s\n",arg->argv[0]);
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
			printf("#%p \"%s\" ",pi->value,pi->name);
			if(pi->type!=IARR)
			{
				get_str_value(pi->value,&sbuf2,pi->type);
				printf("[%s",sbuf2);
				putchar(']');
				free(sbuf2);
			}
			putchar('\n');
		}
		break;

		case 1634036835://clea to clear screen (define for win and linux)
		system(CLSC);
		break;

		case 1718185828://doif
		// doif a[0] =[1] b[2] command blah blah[3]
		if(!check_args(arg,3,0,0,0))break;
		pi=getvarue(arg->argv[0],*currvarlist); //dont forget to free var
		pi2=getvarue(arg->argv[2],*currvarlist);

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
		//printf("\n\nDOIFACC BEFORE = %d\n",*doifacc);
		//printf("%d %c %d = %d\n",*((int*)pi->value),*arg->argv[1],*((int*)pi2->value),jepp);

		if(!jepp)
	       	{
			//if there is no else branch
			if(subs[sstack->logSize-1].doifs[*doifacc].elseline==-1)
			{
				//skip nested
				//printf("There is no else branch\n");
				//printf("skipping %d nests\n",subs[sstack->logSize-1].doifs[*doifacc].nests[0] + subs[sstack->logSize-1].doifs[*doifacc].nests[1]);
				//printf("fin. jumping to %d\n",subs[sstack->logSize-1].doifs[*doifacc].finline);
				*finger=subs[sstack->logSize-1].doifs[*doifacc].finline;
				*doifacc+=subs[sstack->logSize-1].doifs[*doifacc].nests[0] + \
					subs[sstack->logSize-1].doifs[*doifacc].nests[1];
			}
			else
			{
				//printf("else. jumping to %d\n",subs[sstack->logSize-1].doifs[*doifacc].elseline);
				//printf("In the else branch\n");
				//printf("else. skipping %d nests\n",subs[sstack->logSize-1].doifs[*doifacc].nests[0]);
				*finger=subs[sstack->logSize-1].doifs[*doifacc].elseline;
				*doifacc+=subs[sstack->logSize-1].doifs[*doifacc].nests[0];
			}
		}
		(*doifacc)++;
		sstack->subs[sstack->logSize-1]->doifacc=*doifacc;
		//printf("DOIFACC AFTER = %d\n",*doifacc);
		// if true we skip else, if else we skip true and if fin we skip everything

		if(pi->name==0) {free(pi->value);free(pi);}
		if(pi2->name==0) {free(pi2->value);free(pi2);}
		break;

		case 1702063205://else
		//printf("else comm. jumping to %d doifacc - %d\n",subs[sstack->logSize-1].doifs[*doifacc-1].finline,*doifacc);
		//skip else if reached from if
		*finger=subs[sstack->logSize-1].doifs[*doifacc-1].finline;
		*doifacc+=subs[sstack->logSize-1].doifs[*doifacc-1].nests[1];
		sstack->subs[sstack->logSize-1]->doifacc=*doifacc;
		//printf("DOIFACC AFTER = %d\n",*doifacc);
		break;

		case 7235942://fin
		//(*doifacc)++;
		//printf("in fin. doifacc - %d\n",*doifacc);
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
			//printf("changing doifacc to %d\n",labels[i].doifacc);
			*doifacc=labels[i].doifacc;
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
		callsub(arg->argv[0],finger,currvarlist,sstack,subs,sublen,arg);
		*doifacc=sstack->subs[sstack->logSize-1]->doifacc;
		//printf("Setting doifacc to %d\n\n",*doifacc);
		break;

		case 7628146://ret(urn)
		if(arg!=NULL) //return value to the var
		{
			pi=getvarue(arg->argv[0],*currvarlist);

			if(sstack->subs[sstack->logSize-1]->retvar!=NULL)
			memcpy(sstack->subs[sstack->logSize-1]->retvar->value,\
					pi->value,typesize(pi->type));
			else
				printf("Trying to return value altough it doesn't go anwhere");
		}
		wayback=popsub(sstack);	
		
		if(sstack->logSize>0) //when not popping main
		{
			*currvarlist=sstack->subs[sstack->logSize-1]->locvars;
			*doifacc=sstack->subs[sstack->logSize-1]->doifacc;
		}
		*finger=wayback->retline;
		break;

		//glob command to go back to global varlist to be able to goto from gosub to main
		//or to use global vars in sub
		//DO NOT GOTO BETWEED SUBS THAT WILL BREAK CONDITIONS
		case 1651469415://glob
		*currvarlist=sstack->subs[0]->locvars;
		break;

		case 6516588://loc go back to local
		if(sstack->logSize==0)
		{
			fprintf(stderr,"loc: attempt to go back to local when not in function\n");
			break;
		}
		*currvarlist=sstack->subs[sstack->logSize-1]->locvars;
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
		if(arg==NULL){getchar();break;}
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
			default: return;
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
			case ' ':case '	':
			if(in){buff[c++]=arg->all[i];break;}
			buff[c]='\0';
			c=0;
			arg->argv[arg->argc++]=strdup(buff);
			while(arg->all[i+1]==' ')i++;
			if(arg->all[i+1]=='\n')
			{
				free(buff);
				return; //if spaces at the end but no args
			}
			arg->poses[arg->argc]=i+1;
			//printf("arg no.%d [%s] starts at pos %d\n",arg->argc,arg->argv[arg->argc-1],arg->poses[arg->argc]);
			break;

			case '\n':goto out;

			case '"':
			in=!in;
			break;

			default:
			buff[c++]=arg->all[i];
			break;
		}
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
	//assert(new -> value != NULL);
	if(value != NULL)memcpy(new -> value, value, elemSize);
	
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

	if(arg->argc < min || (min > 0 && arg == NULL))
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

char **split(char *sstr, char del) //can only split in two, skips delimeter
{
	char **buff = malloc(sizeof(char *)*2);
	char *str = strdup(sstr);
	int done = 0, i;

	for(i=0;str[i]!=0 && done < 2;i++)
	{
		if(str[i]==del || str[i]==0)
		{
			if(!done)
			{
			str[i]=0;
			buff[0]=str;
			buff[1]=str+i+1;
			return buff;
			}
		}
	}

	//printf("--> %s <--\n",buff[0]);
	//printf("--> %s <--\n",buff[1]);
	buff[0]=str;
	buff[1]=NULL;
	return buff; //free
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
		break;

		case IARR:
		//printf("VALUE BEFORE - %d\n",*(((int *)value1->value)+value1->useindex));
		(iop)[op]( ((int *)value1->value)+value1->useindex,value2->value);
		//printf("VALUE AFTER - %d\n",*(((int *)value1->value)+value1->useindex));
		break;

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

char *skipspaces(char *str)
{
	int s=0;
	if(*str==0){return 0;} //empty
	while(str[s]==' ' || str[s]=='	')s++; //skip spaces
	return str+s;
}

int get_comm(char *input, int *c)
{
	int s=0, b=0, comm; //str counter and buff counter
	char *buff=malloc(COMM_MAX);

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
	var *pi,*pi2, *arrpi;
	void *buff;
	int index;
	char **buffer;

	if(varlist!=NULL)
	{
		switch(value[0])
		{
			case '$':
			if((pi=get_var(varlist,value+1))!=0) return pi;
			break;
			
			case '/': //IARR
			buffer=split(value+1,'/');
			pi2=getvarue(buffer[1],varlist); //index FREEEEEEE
			index=*((int *)pi2->value);

			if((arrpi=get_var(varlist,buffer[0]))!=0)
			{
			pi=malloc(sizeof(var));
			buff=malloc(sizeof(int));
			*((int*)buff) = *((int *)arrpi->value+index);
			pi->value=buff;
			pi->type=INT;
			pi->name=0;

			if(pi2->name==0)
			{
			free(pi2->value);
			free(pi2);
			}

			free(*buffer);
			free(buffer);
			return pi;
			}
			else printf("iarr getvarue error\n");
			free(*buffer);
			free(buffer);
			break;
		}
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

	if(arg->argv[0][0]=='/')
	{
		//printf("Operation args1 - %s\n",arg->argv[0]);
		char **buff=split(arg->argv[0]+1,'/');
		//printf("buff0 %s buff1 %s\n",buff[0], buff[1]);
		pi2=getvarue(buff[1],varlist); //index FREEEEEEE
		int index=*((int *)pi2->value);
		if(pi2->name==0){free(pi2->value);free(pi2);}
		//printf("Index - %d\n",index);
		
        	pi=get_var(varlist,buff[0]);
		pi->useindex=index;
		pi->type=IARR;
		free(*buff);
		free(buff);
	}
	else
        pi=get_var(varlist,arg->argv[0]);

	pi2=getvarue(arg->argv[1],varlist);
        if(pi==NULL){printf("operation: var \"%s\" not found!\n",arg->argv[0]);return;}

        chvar(pi,pi2,op); //op);

	if(pi2->name==0){
		if(pi2->type==STR) free(*((char **)pi2->value));
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
	stack->subs[stack->logSize]->doifacc=0;
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

void callsub(char *name,int *finger,vars **currvarlist,substack *sstack, subroutine *subs, int sublen, args *arg)
{
	long int lgo=strtoi(name,LABLEN);
	var *pi;
	int i,c;

	for(i=0;i<sublen;i++)
	{
		if(lgo==subs[i].name)
		{
		//void add_var(vars *s, int elemSize, void *value, char *name, int type)
		subs[i].retline=*finger;
		*finger=subs[i].startline-1;
		pushsub(sstack,subs+i);

		if(arg!=NULL)
		{ //taking argumets
		for(c=1; c<arg->argc && arg->argv[c][0]!='>'; c++) //convert arguments to local variables
		{
			//printf("arg - %s\n",arg->argv[c]);
			pi=getvarue(arg->argv[c],subs[sstack->logSize-2].locvars);
			add_var(subs[i].locvars,typesize(pi->type),
				pi->value,subs[i].argnames[c-1],pi->type);
			//it's seems it's already being freed in getvarue or something
			//free(pi->value); //this is dumb 'cause i could just giveaway
			//pointer to new var, but instead i copying and freeing this one
			//free(pi);
		}
		
		if(c<arg->argc) //if '>' exists
			subs[i].retvar=get_var(*currvarlist,arg->argv[c+1]);
		else
			subs[i].retvar=NULL;
		}

		*currvarlist=subs[i].locvars;
		goto sfi;
		}
	}
	printf("%d subroutine \"%s\" not found!\n",*finger,name);
	sfi:
	return;
}
