/*
 * coloring.h
 *
 *  Created on: 2019年4月13日
 *      Author: root
 */
#include"nodes/nodes.h"
#ifndef SRC_INCLUDE_COMMANDS_COLORING_H_
#define SRC_INCLUDE_COMMANDS_COLORING_H_

#ifdef __cplusplus
int coloring_s(char* file,TRIPLE** tiple_s,DS_RS_LIST** ds,DS_RS_LIST** rs);
extern "C" {
#endif
int DoColoring_s(char* file,TRIPLE** tiple_s,DS_RS_LIST** ds,DS_RS_LIST** rs);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
int coloring_o(char* file,TRIPLE_DIV** triple_o);
extern "C" {
#endif
int DoColoring_o(char* file,TRIPLE_DIV** triple_o);
#ifdef __cplusplus
}
#endif

#endif /* SRC_INCLUDE_COMMANDS_COLORING_H_ */
