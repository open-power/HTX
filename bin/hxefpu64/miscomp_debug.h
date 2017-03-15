/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/* %Z%%M%   %I%  %W% %G% %U% */

#ifndef _MISCMP_DEBUG_H
#define _MISCMP_DEBUG_H

#include "framework.h"


#define INSTR_TYPE_LOAD 	(1)
#define INSTR_TYPE_STORE 	(2)
#define INSTR_TYPE_OTHER	(0)

#define VSR_DTYPE			(1)
#define GPR_DTYPE			(2)
#define	OTHER_DTYPE			(4)

void decode_tc_instructions(int cno);

struct reguse *prepare_ls_call_list(FILE *dump, int cno, int cmp_offset, struct miscmp_node **head);

struct reguse *create_instr_tree(FILE *dump, int cno, int reg_num, int tgt_dtype, int start_index, struct miscmp_node **head);

void delete_reg_use_list(struct reguse *reg_node);

#endif /* _MISCMP_DEBUG_H */
