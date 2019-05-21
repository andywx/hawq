/*
 * copyrdf.h
 *
 *  Created on: 18 Mar 2019
 *      Author: liu
 */

#ifndef SRC_INCLUDE_COMMANDS_COPYRDF_H_
#define SRC_INCLUDE_COMMANDS_COPYRDF_H_
#define KEY_MAX_LENGTH 256
#define LINE 1024
#include "c.h"
#include "nodes/parsenodes.h"
#include "commands/coloring.h"
typedef struct{
	char* p;
	char* o;
}P2O_ITERM;
typedef struct{
	char* p;
	char* s;
}P2S_ITERM;
extern uint64 DoCopyRdf(const CopyRdfStmt *stmt, const char *queryString);
extern void CreateTable_s(int s_column_num,char* schema_name);
extern void CreateTable_o(int o_column_num,char* schema_name);
extern void Inserts_s(TRIPLE* triple_s,int s_column_num,RangeVar* create_ds,RangeVar* create_dph);
extern void Inserts_o(TRIPLE_DIV* triple_o,int o_column_num,RangeVar* create_rs,RangeVar* create_rph);
extern unsigned int SDBMHash(char *str);
extern unsigned int RSHash(char *str);
extern unsigned int JSHash(char *str);
extern unsigned int PJWHash(char *str);
extern unsigned int ELFHash(char *str);
extern unsigned int BKDRHash(char *str);
extern unsigned int DJBHash(char *str);
extern unsigned int APHash(char *str);
#endif /* SRC_INCLUDE_COMMANDS_COPYRDF_H_ */
