/*
 * copyrdf.c
 *
 *  created on: 18 Mar 2019
 *      Author: liu
 */
#include "postgres.h"
#include "access/attnum.h"
#include "access/xact.h"
#include "commands/copyrdf.h"
#include "commands/schemacmds.h"
#include "commands/tablecmds.h"
#include "commands/coloring.h"
#include "commands/copy.h"
#include "catalog/toasting.h"
#include "catalog/aoseg.h"
#include "catalog/gp_policy.h"
#include "cdb/cdbparquetfooterserializer_protocol.h"
#include "parser/gramparse.h"
#include "postmaster/identity.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "nodes/print.h"
#include "nodes/nodes.h"
#include "utils/elog.h"
#include "assert.h"
#include "string.h"
#include "nodes/nodes.h"

TRIPLE* triple_s;
TRIPLE_DIV* triple_o;
DS_RS_LIST* ds;
DS_RS_LIST* rs;
uint64 DoCopyRdf(const CopyRdfStmt *stmt, const char *queryString)
{
	uint64      processed=0;
	elog(DEBUG5, "COPYRDF START");

	/*CREATE SCHEMA */
	elog(DEBUG5, "create_rph SCHEMA!!!");
	CreateSchemaStmt* schema = makeNode(CreateSchemaStmt);
	schema->type=T_CreateSchemaStmt;
	schema->schemaname=stmt->schema_name;
	char *schema_name=stmt->schema_name;
	char schema_query[50]="create_rph schema ";
	strcat(schema_query,schema_name);
	char *query=schema_query;
	CreateSchemaCommand(schema,query);
	elog(DEBUG5, "SCHEMA SUCCESS!!!");

	/*GRAPH COLOR */
	int s_column_num;
	s_column_num = DoColoring_s(stmt->filename,&triple_s,&ds,&rs);
	int o_column_num ;
	o_column_num = DoColoring_o(stmt->filename,&triple_o);

	/*CREATE TABLES AND INSERTS INTO THE TABLES*/
	CreateTable_s(s_column_num,stmt->schema_name);
	CreateTable_o(o_column_num,stmt->schema_name);

	return processed;
}
void CreateTable_s(int s_column_num,char* schema_name)
{
	CreateStmt* create_dph = makeNode(CreateStmt);
	ColumnDef *columEle = makeNode(ColumnDef);
	Value* dis=makeNode(Value);
	create_dph->tableElts = NIL;
	create_dph->distributedBy = NIL;
	create_dph->type=T_CreateStmt;

	columEle->colname="sub";
	columEle->typname=makeTypeName("text");
	columEle->is_local=true;
	dis=makeString("sub");

	create_dph->distributedBy=list_make1( dis);
	create_dph->tableElts=list_make1(columEle);

	columEle = makeNode(ColumnDef);
	columEle->colname="spill";
	columEle->typname=SystemTypeName("int4");
	columEle->is_local=true;
	dis=makeString("spill");

	create_dph->distributedBy=lappend(create_dph->distributedBy,dis);
	create_dph->tableElts=lappend(create_dph->tableElts,columEle);

	int k=1;
	int flag_num=s_column_num;
	while(flag_num!=0)
	{
		char* p_flag=(char*)malloc(sizeof(char*)*3);
		memset(p_flag,0,sizeof(char*)*3);
		char* v_flag=(char*)malloc(sizeof(char*)*3);
		memset(v_flag,0,sizeof(char*)*3);
		p_flag[0]='p';
		v_flag[0]='v';
		char flag[3];
		sprintf(flag,"%d",k);
		k++;
		char *predicates=strcat(p_flag,flag);
		char *values=strcat(v_flag,flag);

		columEle = makeNode(ColumnDef);
		columEle->colname=predicates;
		columEle->typname=makeTypeName("text");
		columEle->is_local=true;
		create_dph->tableElts=lappend(create_dph->tableElts,columEle);

		columEle = makeNode(ColumnDef);
		columEle->colname=values;
		columEle->typname=makeTypeName("text");
		columEle->is_local=true;
		create_dph->tableElts=lappend(create_dph->tableElts,columEle);
		flag_num--;
	}
	char* catalogname=0x0;
	char* relname="dph";
	(create_dph->relation) = makeRangeVar(catalogname,schema_name,relname,13);

	GpPolicy  *policy = NULL;
	int			maxattrs = 200;
	policy = (GpPolicy *) palloc(sizeof(GpPolicy) + maxattrs *
								 sizeof(policy->attrs[0]));
	policy->ptype = POLICYTYPE_PARTITIONED;
	policy->bucketnum=GetRelOpt_bucket_num_fromOptions(create_dph->options, GetHashDistPartitionNum());
	policy->nattrs=2;
	policy->attrs[0]=1;
	policy->attrs[1]=2;
	*(&(create_dph->policy))=policy;
	create_dph->oncommit=ONCOMMIT_NOOP;

	create_dph->relKind='r';

	Oid relOid;
	relOid=DefineRelation(create_dph,'r','h');
	CommandCounterIncrement();
	AlterTableCreateToastTableWithOid(relOid,
			create_dph->oidInfo.toastOid,
			create_dph->oidInfo.toastIndexOid,
			&(create_dph->oidInfo.toastComptypeOid),
			create_dph->is_part_child);

	AlterTableCreateAoSegTableWithOid(relOid,
			create_dph->oidInfo.aosegOid,
			create_dph->oidInfo.aosegIndexOid,
			&(create_dph->oidInfo.aosegComptypeOid),
			create_dph->is_part_child);
	CommandCounterIncrement();
	EvaluateDeferredStatements(create_dph->deferredStmts);
	/*DS*/
	CreateStmt* create_ds = makeNode(CreateStmt);
	columEle = makeNode(ColumnDef);
	dis=makeNode(Value);
	create_ds->tableElts = NIL;
	create_ds->distributedBy = NIL;
	create_ds->type=T_CreateStmt;

	columEle->colname="l_id";
	columEle->typname=makeTypeName("text");
	columEle->is_local=true;
	dis=makeString("l_id");

	create_ds->distributedBy=list_make1( dis);
	create_ds->tableElts=list_make1(columEle);

	columEle = makeNode(ColumnDef);
	columEle->colname="elm";
	columEle->typname=SystemTypeName("text");
	columEle->is_local=true;

	create_ds->tableElts=lappend(create_ds->tableElts,columEle);

	catalogname=0x0;
	relname="ds";
	(create_ds->relation) = makeRangeVar(catalogname,schema_name,relname,13);

	policy = NULL;
	maxattrs = 200;
	policy = (GpPolicy *) palloc(sizeof(GpPolicy) + maxattrs *
								 sizeof(policy->attrs[0]));
	policy->ptype = POLICYTYPE_PARTITIONED;
	policy->bucketnum=GetRelOpt_bucket_num_fromOptions(create_ds->options, GetHashDistPartitionNum());
	policy->nattrs=1;
	policy->attrs[0]=1;
	*(&(create_ds->policy))=policy;
	create_ds->oncommit=ONCOMMIT_NOOP;

	create_ds->relKind='r';

	relOid=DefineRelation(create_ds,'r','h');
	CommandCounterIncrement();
	AlterTableCreateToastTableWithOid(relOid,
			create_ds->oidInfo.toastOid,
			create_ds->oidInfo.toastIndexOid,
			&(create_ds->oidInfo.toastComptypeOid),
			create_ds->is_part_child);

	AlterTableCreateAoSegTableWithOid(relOid,
			create_ds->oidInfo.aosegOid,
			create_ds->oidInfo.aosegIndexOid,
			&(create_ds->oidInfo.aosegComptypeOid),
			create_ds->is_part_child);
	CommandCounterIncrement();
	EvaluateDeferredStatements(create_ds->deferredStmts);
	Inserts_s(triple_s,s_column_num,create_ds->relation,create_dph->relation);
}
void CreateTable_o(int o_column_num,char* schema_name)
{
	/*RPH*/
	CreateStmt* create_rph = makeNode(CreateStmt);
	ColumnDef *columEle = makeNode(ColumnDef);
	Value* dis=makeNode(Value);
	create_rph->tableElts = NIL;
	create_rph->distributedBy = NIL;
	create_rph->type=T_CreateStmt;

	columEle->colname="obj";
	columEle->typname=makeTypeName("text");
	columEle->is_local=true;
	dis=makeString("sub");

	create_rph->distributedBy=list_make1( dis);
	create_rph->tableElts=list_make1(columEle);

	columEle = makeNode(ColumnDef);
	columEle->colname="spill";
	columEle->typname=SystemTypeName("int4");
	columEle->is_local=true;
	dis=makeString("spill");

	create_rph->distributedBy=lappend(create_rph->distributedBy,dis);
	create_rph->tableElts=lappend(create_rph->tableElts,columEle);

	int k=1;
	int flag_num=o_column_num;
	while(flag_num!=0)
	{
		char* p_flag=(char*)malloc(sizeof(char*)*3);
		memset(p_flag,0,sizeof(char*)*3);
		char* v_flag=(char*)malloc(sizeof(char*)*3);
		memset(v_flag,0,sizeof(char*)*3);
		p_flag[0]='p';
		v_flag[0]='v';
		char flag[3];
		sprintf(flag,"%d",k);
		k++;
		char *predicates=strcat(p_flag,flag);
		char *values=strcat(v_flag,flag);

		columEle = makeNode(ColumnDef);
		columEle->colname=predicates;
		columEle->typname=makeTypeName("text");
		columEle->is_local=true;
		create_rph->tableElts=lappend(create_rph->tableElts,columEle);

		columEle = makeNode(ColumnDef);
		columEle->colname=values;
		columEle->typname=makeTypeName("text");
		columEle->is_local=true;
		create_rph->tableElts=lappend(create_rph->tableElts,columEle);

		flag_num--;
	}
	char* catalogname=0x0;
	char* relname="rph";
	(create_rph->relation) = makeRangeVar(catalogname,schema_name,relname,13);

	GpPolicy  *policy = NULL;
	int			maxattrs = 200;
	policy = (GpPolicy *) palloc(sizeof(GpPolicy) + maxattrs *
								 sizeof(policy->attrs[0]));
	policy->ptype = POLICYTYPE_PARTITIONED;
	policy->bucketnum=GetRelOpt_bucket_num_fromOptions(create_rph->options, GetHashDistPartitionNum());
	policy->nattrs=2;
	policy->attrs[0]=1;
	policy->attrs[1]=2;
	*(&(create_rph->policy))=policy;
	create_rph->oncommit=ONCOMMIT_NOOP;

	create_rph->relKind='r';

	Oid relOid;
	relOid=DefineRelation(create_rph,'r','h');
	CommandCounterIncrement();
	AlterTableCreateToastTableWithOid(relOid,
			create_rph->oidInfo.toastOid,
			create_rph->oidInfo.toastIndexOid,
			&(create_rph->oidInfo.toastComptypeOid),
			create_rph->is_part_child);

	AlterTableCreateAoSegTableWithOid(relOid,
			create_rph->oidInfo.aosegOid,
			create_rph->oidInfo.aosegIndexOid,
			&(create_rph->oidInfo.aosegComptypeOid),
			create_rph->is_part_child);
	CommandCounterIncrement();
	EvaluateDeferredStatements(create_rph->deferredStmts);

	/*RS*/
	CreateStmt* create_rs = makeNode(CreateStmt);
	columEle = makeNode(ColumnDef);
	dis=makeNode(Value);
	create_rs->tableElts = NIL;
	create_rs->distributedBy = NIL;
	create_rs->type=T_CreateStmt;

	columEle->colname="l_id";
	columEle->typname=makeTypeName("text");
	columEle->is_local=true;
	dis=makeString("l_id");

	create_rs->distributedBy=list_make1( dis);
	create_rs->tableElts=list_make1(columEle);

	columEle = makeNode(ColumnDef);
	columEle->colname="elm";
	columEle->typname=SystemTypeName("text");
	columEle->is_local=true;

	create_rs->tableElts=lappend(create_rs->tableElts,columEle);

	catalogname=0x0;
	relname="rs";
	(create_rs->relation) = makeRangeVar(catalogname,schema_name,relname,13);

	policy = NULL;
	maxattrs = 200;
	policy = (GpPolicy *) palloc(sizeof(GpPolicy) + maxattrs *
								 sizeof(policy->attrs[0]));
	policy->ptype = POLICYTYPE_PARTITIONED;
	policy->bucketnum=GetRelOpt_bucket_num_fromOptions(create_rs->options, GetHashDistPartitionNum());
	policy->nattrs=1;
	policy->attrs[0]=1;
	*(&(create_rs->policy))=policy;
	create_rs->oncommit=ONCOMMIT_NOOP;

	create_rs->relKind='r';

	relOid=DefineRelation(create_rs,'r','h');
	CommandCounterIncrement();
	AlterTableCreateToastTableWithOid(relOid,
			create_rs->oidInfo.toastOid,
			create_rs->oidInfo.toastIndexOid,
			&(create_rs->oidInfo.toastComptypeOid),
			create_rs->is_part_child);

	AlterTableCreateAoSegTableWithOid(relOid,
			create_rs->oidInfo.aosegOid,
			create_rs->oidInfo.aosegIndexOid,
			&(create_rs->oidInfo.aosegComptypeOid),
			create_rs->is_part_child);
	CommandCounterIncrement();
	EvaluateDeferredStatements(create_rs->deferredStmts);
	Inserts_o(triple_o,o_column_num,create_rs->relation,create_rph->relation);
}
void Inserts_s(TRIPLE* triple_s,int s_column_num,RangeVar* create_ds,RangeVar* create_dph)
{
	FILE* out,*out1;
	P2O_ITERM p2o_array[3][s_column_num];

	P2O* p2o_tmp=triple_s->sub_pre_obj.values;
	TRIPLE* triple_tmp=triple_s;
	if ((out1=fopen("dph.csv", "w")) == NULL) {
		printf("open file write error!!\n");
		return ;
	}
	while(triple_tmp->next)
	{
		int i=0,j=0;
		while(i!=3)
		{
			while(j!=s_column_num)
			{
				p2o_array[i][j].p=(char*)malloc(sizeof(char)*100);
				memset(p2o_array[i][j].p,0,sizeof(char)*100);
				p2o_array[i][j].o=(char*)malloc(sizeof(char)*1000);
				memset(p2o_array[i][j].o,0,sizeof(char)*1000);
				j++;
			}
			i++;
			j=0;
		}
		int spill;
		bool split=false;
		while(p2o_tmp->p)
		{
			int v1=SDBMHash(p2o_tmp->p)%s_column_num;
			int v2=RSHash(p2o_tmp->p)%s_column_num;
			int v3=JSHash(p2o_tmp->p)%s_column_num;
			int v4=PJWHash(p2o_tmp->p)%s_column_num;
			int v5=ELFHash(p2o_tmp->p)%s_column_num;
			int v6=BKDRHash(p2o_tmp->p)%s_column_num;
			int v7=DJBHash(p2o_tmp->p)%s_column_num;
			int v8=APHash(p2o_tmp->p)%s_column_num;
			bool found=false;
			spill=0;
			while(!found)
			{
				if(spill==3)
					break;
				if(*p2o_array[spill][v1].p=='\0')
				{
					char* p_tmp=(char*)malloc(sizeof(char)*100);
					memset(p_tmp,0,sizeof(char)*100);
					strcat(p_tmp,p2o_tmp->p);
					char* o_tmp=(char*)malloc(sizeof(char)*1000);
					memset(o_tmp,0,sizeof(char)*1000);
					strcat(o_tmp,p2o_tmp->o);
					p2o_array[spill][v1].p=p_tmp;
					p2o_array[spill][v1].o=o_tmp;
					found=true;
				}else{
					if(*p2o_array[spill][v2].p=='\0')
					{
						char* p_tmp=(char*)malloc(sizeof(char)*100);
						memset(p_tmp,0,sizeof(char)*100);
						strcat(p_tmp,p2o_tmp->p);
						char* o_tmp=(char*)malloc(sizeof(char)*1000);
						memset(o_tmp,0,sizeof(char)*1000);
						strcat(o_tmp,p2o_tmp->o);
						p2o_array[spill][v2].p=p_tmp;
						p2o_array[spill][v2].o=o_tmp;
						found=true;
					}else{
						if(*p2o_array[spill][v3].p=='\0')
						{
							char* p_tmp=(char*)malloc(sizeof(char)*100);
							memset(p_tmp,0,sizeof(char)*100);
							strcat(p_tmp,p2o_tmp->p);
							char* o_tmp=(char*)malloc(sizeof(char)*1000);
							memset(o_tmp,0,sizeof(char)*1000);
							strcat(o_tmp,p2o_tmp->o);
							p2o_array[spill][v3].p=p_tmp;
							p2o_array[spill][v3].o=o_tmp;
							found=true;
						}else{
							if(*p2o_array[spill][v4].p=='\0')
							{
								char* p_tmp=(char*)malloc(sizeof(char)*100);
								memset(p_tmp,0,sizeof(char)*100);
								strcat(p_tmp,p2o_tmp->p);
								char* o_tmp=(char*)malloc(sizeof(char)*1000);
								memset(o_tmp,0,sizeof(char)*1000);
								strcat(o_tmp,p2o_tmp->o);
								p2o_array[spill][v4].p=p_tmp;
								p2o_array[spill][v4].o=o_tmp;
								found=true;
							}else{
								if(*p2o_array[spill][v5].p=='\0')
								{
									char* p_tmp=(char*)malloc(sizeof(char)*100);
									memset(p_tmp,0,sizeof(char)*100);
									strcat(p_tmp,p2o_tmp->p);
									char* o_tmp=(char*)malloc(sizeof(char)*1000);
									memset(o_tmp,0,sizeof(char)*1000);
									strcat(o_tmp,p2o_tmp->o);
									p2o_array[spill][v5].p=p_tmp;
									p2o_array[spill][v5].o=o_tmp;
									found=true;
								}else{
									if(*p2o_array[spill][v6].p=='\0')
									{
										char* p_tmp=(char*)malloc(sizeof(char)*100);
										memset(p_tmp,0,sizeof(char)*100);
										strcat(p_tmp,p2o_tmp->p);
										char* o_tmp=(char*)malloc(sizeof(char)*1000);
										memset(o_tmp,0,sizeof(char)*1000);
										strcat(o_tmp,p2o_tmp->o);
										p2o_array[spill][v6].p=p_tmp;
										p2o_array[spill][v6].o=o_tmp;
										found=true;
									}else{
										if(*p2o_array[spill][v7].p=='\0')
										{
											char* p_tmp=(char*)malloc(sizeof(char)*100);
											memset(p_tmp,0,sizeof(char)*100);
											strcat(p_tmp,p2o_tmp->p);
											char* o_tmp=(char*)malloc(sizeof(char)*1000);
											memset(o_tmp,0,sizeof(char)*1000);
											strcat(o_tmp,p2o_tmp->o);
											p2o_array[spill][v7].p=p_tmp;
											p2o_array[spill][v7].o=o_tmp;
											found=true;
										}else{
											if(*p2o_array[spill][v8].p=='\0')
											{
												char* p_tmp=(char*)malloc(sizeof(char)*100);
												memset(p_tmp,0,sizeof(char)*100);
												strcat(p_tmp,p2o_tmp->p);
												char* o_tmp=(char*)malloc(sizeof(char)*1000);
												memset(o_tmp,0,sizeof(char)*1000);
												strcat(o_tmp,p2o_tmp->o);
												p2o_array[spill][v8].p=p_tmp;
												p2o_array[spill][v8].o=o_tmp;
												found=true;
											}else{
												spill++;
												split=true;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			p2o_tmp=p2o_tmp->next;

		}

		if(split==false)
		{
			fputs(triple_tmp->sub_pre_obj.s,out1);
			fputs(",",out1);
			fputs("0",out1);
			int flag=0;
			while(flag!=s_column_num)
			{
				fputs(",",out1);
				if(*p2o_array[0][flag].p!='\0')
				{
					fputs(p2o_array[0][flag].p,out1);
				}else{
					fputs("NULL",out1);
				}
				fputs(",",out1);
				if(*p2o_array[0][flag].o!='\0')
				{
					fputs(p2o_array[0][flag].o,out1);
				}else{
					fputs("NULL",out1);
				}
				flag++;
			}
			fputs("\n",out1);
		}else{
			int round=spill;
			while(round>=0)
			{
				fputs(triple_tmp->sub_pre_obj.s,out1);
				fputs(",",out1);
				char spill_c[3];
				sprintf(spill_c,"%d",spill);
				fputs(spill_c,out1);
				int flag=0;
				while(flag!=s_column_num)
				{
					fputs(",",out1);
					if(*p2o_array[round][flag].p!='\0')
					{
						fputs(p2o_array[round][flag].p,out1);
					}else{
						fputs("NULL",out1);
					}
					fputs(",",out1);
					if(*p2o_array[round][flag].o!='\0')
					{
						fputs(p2o_array[round][flag].o,out1);
					}else{
						fputs("NULL",out1);
					}
					flag++;
				}
				fputs("\n",out1);
				round--;
			}
		}
		triple_tmp=triple_tmp->next;
		p2o_tmp=triple_tmp->sub_pre_obj.values;
	}
	if ((out=fopen("ds.csv", "w")) == NULL) {
		return ;
	}
	if(fclose(out1))
	{
		return;
	}
	fflush(out1);
	DS_RS_LIST* ds_tmp=(ds);
	while(ds_tmp!=NULL&&ds_tmp->l_id!=NULL)
	{
		fputs(ds_tmp->l_id,out);
		fputs(",",out);
		fputs(ds_tmp->elm,out);
		fputs("\n",out);

		ds_tmp=ds_tmp->next;
	}
	if(fclose(out))
	{
		return;
	}

	/*COPY INSERTS DPH*/
	CopyStmt *copy_dph = makeNode(CopyStmt);
	copy_dph->relation=create_dph;
	copy_dph->is_from=true;
	copy_dph->filename="dph.csv";
	DefElem * elem =makeDefElem("delimiter", (Node *)makeString(","));
	copy_dph->options=list_make1(elem);


	char debug_query_string_dph[50]="copy ";
	strcat(debug_query_string_dph,create_dph->schemaname);
	strcat(debug_query_string_dph,".");
	strcat(debug_query_string_dph,create_dph->relname);
	char from[7]=" from ";
	strcat(debug_query_string_dph,from);
	strcat(debug_query_string_dph,"dph.csv");
	DoCopy(copy_dph, debug_query_string_dph);

	/*COPY INSERTS DS*/
	CopyStmt *copy_ds = makeNode(CopyStmt);
	copy_ds->relation=create_ds;
	copy_ds->is_from=true;
	copy_ds->filename="ds.csv";
	copy_ds->options=list_make1(elem);
	char debug_query_string_ds[50]="copy ";
	strcat(debug_query_string_ds,create_ds->schemaname);
	strcat(debug_query_string_ds,".");
	strcat(debug_query_string_ds,create_ds->relname);
	strcat(debug_query_string_ds,from);
	strcat(debug_query_string_ds,"ds.csv");
	DoCopy(copy_ds, debug_query_string_ds);
}
void Inserts_o(TRIPLE_DIV* triple_o,int o_column_num,RangeVar* create_rs,RangeVar* create_rph)
{
	P2S_ITERM p2s_array[3][o_column_num];

	P2S* p2s_tmp=triple_o->obj_sub_pre.values;
	TRIPLE_DIV* triple_tmp=triple_o;
	FILE* out,*out1;
	if ((out1=fopen("rph.csv", "w")) == NULL) {
		printf("open file write error!!\n");
		return;
	}
	while(triple_tmp->next)
	{
		int i=0,j=0;
		while(i!=3)
		{
			while(j!=o_column_num)
			{
				p2s_array[i][j].p=(char*)malloc(sizeof(char)*100);
				memset(p2s_array[i][j].p,0,sizeof(char)*100);
				p2s_array[i][j].s=(char*)malloc(sizeof(char)*100);
				memset(p2s_array[i][j].s,0,sizeof(char)*100);
				j++;
			}
			i++;
			j=0;
		}
		int spill;
		bool split=false;
		while(p2s_tmp->p)
		{
			int v1=SDBMHash(p2s_tmp->p)%o_column_num;
			int v2=RSHash(p2s_tmp->p)%o_column_num;
			int v3=JSHash(p2s_tmp->p)%o_column_num;
			int v4=PJWHash(p2s_tmp->p)%o_column_num;
			int v5=ELFHash(p2s_tmp->p)%o_column_num;
			int v6=BKDRHash(p2s_tmp->p)%o_column_num;
			int v7=DJBHash(p2s_tmp->p)%o_column_num;
			int v8=APHash(p2s_tmp->p)%o_column_num;
			bool found=false;
			spill=0;
			while(!found)
			{
				if(spill==3)
					break;
				if(*p2s_array[spill][v1].p=='\0')
				{
					char* p_tmp=(char*)malloc(sizeof(char)*100);
					memset(p_tmp,0,sizeof(char)*100);
					strcat(p_tmp,p2s_tmp->p);
					char* s_tmp=(char*)malloc(sizeof(char)*100);
					memset(s_tmp,0,sizeof(char)*100);
					strcat(s_tmp,p2s_tmp->s);
					p2s_array[spill][v1].p=p_tmp;
					p2s_array[spill][v1].s=s_tmp;
					found=true;
				}else{
					if(*p2s_array[spill][v2].p=='\0')
					{
						char* p_tmp=(char*)malloc(sizeof(char)*100);
						memset(p_tmp,0,sizeof(char)*100);
						strcat(p_tmp,p2s_tmp->p);
						char* s_tmp=(char*)malloc(sizeof(char)*100);
						memset(s_tmp,0,sizeof(char)*100);
						strcat(s_tmp,p2s_tmp->s);
						p2s_array[spill][v2].p=p_tmp;
						p2s_array[spill][v2].s=s_tmp;
						found=true;
					}else{
						if(*p2s_array[spill][v3].p=='\0')
						{
							char* p_tmp=(char*)malloc(sizeof(char)*100);
							memset(p_tmp,0,sizeof(char)*100);
							strcat(p_tmp,p2s_tmp->p);
							char* s_tmp=(char*)malloc(sizeof(char)*100);
							memset(s_tmp,0,sizeof(char)*100);
							strcat(s_tmp,p2s_tmp->s);
							p2s_array[spill][v3].p=p_tmp;
							p2s_array[spill][v3].s=s_tmp;
							found=true;
						}else{
							if(*p2s_array[spill][v4].p=='\0')
							{
								char* p_tmp=(char*)malloc(sizeof(char)*100);
								memset(p_tmp,0,sizeof(char)*100);
								strcat(p_tmp,p2s_tmp->p);
								char* s_tmp=(char*)malloc(sizeof(char)*100);
								memset(s_tmp,0,sizeof(char)*100);
								strcat(s_tmp,p2s_tmp->s);
								p2s_array[spill][v4].p=p_tmp;
								p2s_array[spill][v4].s=s_tmp;
								found=true;
							}else{
								if(*p2s_array[spill][v5].p=='\0')
								{
									char* p_tmp=(char*)malloc(sizeof(char)*100);
									memset(p_tmp,0,sizeof(char)*100);
									strcat(p_tmp,p2s_tmp->p);
									char* s_tmp=(char*)malloc(sizeof(char)*100);
									memset(s_tmp,0,sizeof(char)*100);
									strcat(s_tmp,p2s_tmp->s);
									p2s_array[spill][v5].p=p_tmp;
									p2s_array[spill][v5].s=s_tmp;
									found=true;
								}else{
									if(*p2s_array[spill][v6].p=='\0')
									{
										char* p_tmp=(char*)malloc(sizeof(char)*100);
										memset(p_tmp,0,sizeof(char)*100);
										strcat(p_tmp,p2s_tmp->p);
										char* s_tmp=(char*)malloc(sizeof(char)*100);
										memset(s_tmp,0,sizeof(char)*100);
										strcat(s_tmp,p2s_tmp->s);
										p2s_array[spill][v6].p=p_tmp;
										p2s_array[spill][v6].s=s_tmp;
										found=true;
									}else{
										if(*p2s_array[spill][v7].p=='\0')
										{
											char* p_tmp=(char*)malloc(sizeof(char)*100);
											memset(p_tmp,0,sizeof(char)*100);
											strcat(p_tmp,p2s_tmp->p);
											char* s_tmp=(char*)malloc(sizeof(char)*100);
											memset(s_tmp,0,sizeof(char)*100);
											strcat(s_tmp,p2s_tmp->s);
											p2s_array[spill][v7].p=p_tmp;
											p2s_array[spill][v7].s=s_tmp;
											found=true;
										}else{
											if(*p2s_array[spill][v8].p=='\0')
											{
												char* p_tmp=(char*)malloc(sizeof(char)*100);
												memset(p_tmp,0,sizeof(char)*100);
												strcat(p_tmp,p2s_tmp->p);
												char* s_tmp=(char*)malloc(sizeof(char)*100);
												memset(s_tmp,0,sizeof(char)*100);
												strcat(s_tmp,p2s_tmp->s);
												p2s_array[spill][v8].p=p_tmp;
												p2s_array[spill][v8].s=s_tmp;
												found=true;
											}else{
												spill++;
												split=true;
											}
										}
									}
								}
							}
						}
					}
				}
			}

			p2s_tmp=p2s_tmp->next;

		}

		if(split==false)
		{
			fputs(triple_tmp->obj_sub_pre.o,out1);
			fputs(",",out1);
			fputs("0",out1);
			int flag=0;
			while(flag!=o_column_num)
			{
				fputs(",",out1);
				if(*p2s_array[0][flag].p!='\0')
				{
					fputs(p2s_array[0][flag].p,out1);
				}else{
					fputs("NULL",out1);
				}
				fputs(",",out1);
				if(*p2s_array[0][flag].s!='\0')
				{
					fputs(p2s_array[0][flag].s,out1);
				}else{
					fputs("NULL",out1);
				}
				flag++;
			}
			fputs("\n",out1);
		}else{
			int round=spill;
			while(round)
			{
				fputs(triple_tmp->obj_sub_pre.o,out1);
				fputs(",",out1);
				char spill_c[3];
				sprintf(spill_c,"%d",spill);
				fputs(spill_c,out1);
				int flag=0;
				while(flag!=o_column_num)
				{
					fputs(",",out1);
					if(*p2s_array[0][flag].p!='\0')
					{
						fputs(p2s_array[0][flag].p,out1);
					}else{
						fputs("NULL",out1);
					}
					fputs(",",out1);
					if(*p2s_array[0][flag].s!='\0')
					{
						fputs(p2s_array[0][flag].s,out1);
					}else{
						fputs("NULL",out1);
					}
					flag++;
				}
				fputs("\n",out1);
				round--;
			}
		}
		triple_tmp=triple_tmp->next;
		p2s_tmp=triple_tmp->obj_sub_pre.values;
	}
	if(fclose(out1))
	{
		return;
	}
	fflush(out1);
	if ((out=fopen("rs.csv", "w")) == NULL) {
		return ;
	}
	DS_RS_LIST* rs_tmp=(rs);
	while(rs_tmp!=NULL&&rs_tmp->l_id!=NULL)
	{
		fputs(rs_tmp->l_id,out);
		fputs(",",out);
		fputs(rs_tmp->elm,out);
		fputs("\n",out);

		rs_tmp=rs_tmp->next;
	}
	if(fclose(out))
	{
		return;
	}

	/*COPY INSERTS RPH*/
	CopyStmt *copy_rph = makeNode(CopyStmt);
	copy_rph->relation=create_rph;
	copy_rph->is_from=true;
	copy_rph->filename="rph.csv";
	DefElem * elem =makeDefElem("delimiter", (Node *)makeString(","));
	copy_rph->options=list_make1(elem);


	char debug_query_string_dph[50]="copy ";
	strcat(debug_query_string_dph,create_rph->schemaname);
	strcat(debug_query_string_dph,".");
	strcat(debug_query_string_dph,create_rph->relname);
	char from[7]=" from ";
	strcat(debug_query_string_dph,from);
	strcat(debug_query_string_dph,"rph.csv");
	DoCopy(copy_rph, debug_query_string_dph);

	/*COPY INSERTS RS*/
	CopyStmt *copy_rs = makeNode(CopyStmt);
	copy_rs->relation=create_rs;
	copy_rs->is_from=true;
	copy_rs->filename="rs.csv";
	copy_rs->options=list_make1(elem);
	char debug_query_string_ds[50]="copy ";
	strcat(debug_query_string_ds,create_rs->schemaname);
	strcat(debug_query_string_ds,".");
	strcat(debug_query_string_ds,create_rs->relname);
	strcat(debug_query_string_ds,from);
	strcat(debug_query_string_ds,"rs.csv");
	DoCopy(copy_rs, debug_query_string_ds);
}
unsigned int SDBMHash(char *str)
{
    unsigned int hash = 0;

    while (*str)
    {
        // equivalent to: hash = 65599*hash + (*str++);
        hash = (*str++) + (hash << 6) + (hash << 16) - hash;
    }

    return (hash & 0x7FFFFFFF);
}

// RS Hash Function
unsigned int RSHash(char *str)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * a + (*str++);
        a *= b;
    }

    return (hash & 0x7FFFFFFF);
}

// JS Hash Function
unsigned int JSHash(char *str)
{
    unsigned int hash = 1315423911;

    while (*str)
    {
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }

    return (hash & 0x7FFFFFFF);
}

// P. J. Weinberger Hash Function
unsigned int PJWHash(char *str)
{
    unsigned int BitsInUnignedInt = (unsigned int)(sizeof(unsigned int) * 8);
    unsigned int ThreeQuarters    = (unsigned int)((BitsInUnignedInt  * 3) / 4);
    unsigned int OneEighth        = (unsigned int)(BitsInUnignedInt / 8);
    unsigned int HighBits         = (unsigned int)(0xFFFFFFFF) << (BitsInUnignedInt - OneEighth);
    unsigned int hash             = 0;
    unsigned int test             = 0;

    while (*str)
    {
        hash = (hash << OneEighth) + (*str++);
        if ((test = hash & HighBits) != 0)
        {
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits));
        }
    }

    return (hash & 0x7FFFFFFF);
}

// ELF Hash Function
unsigned int ELFHash(char *str)
{
    unsigned int hash = 0;
    unsigned int x    = 0;

    while (*str)
    {
        hash = (hash << 4) + (*str++);
        if ((x = hash & 0xF0000000L) != 0)
        {
            hash ^= (x >> 24);
            hash &= ~x;
        }
    }

    return (hash & 0x7FFFFFFF);
}

// BKDR Hash Function
unsigned int BKDRHash(char *str)
{
    unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
    unsigned int hash = 0;

    while (*str)
    {
        hash = hash * seed + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

// DJB Hash Function
unsigned int DJBHash(char *str)
{
    unsigned int hash = 5381;

    while (*str)
    {
        hash += (hash << 5) + (*str++);
    }

    return (hash & 0x7FFFFFFF);
}

// AP Hash Function
unsigned int APHash(char *str)
{
    unsigned int hash = 0;
    int i;

    for (i=0; *str; i++)
    {
        if ((i & 1) == 0)
        {
            hash ^= ((hash << 7) ^ (*str++) ^ (hash >> 3));
        }
        else
        {
            hash ^= (~((hash << 11) ^ (*str++) ^ (hash >> 5)));
        }
    }

    return (hash & 0x7FFFFFFF);
}
