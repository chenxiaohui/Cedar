/**
* Copyright (C) 2013-2015 ECNU_DaSE.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* @file ob_cursor_fetch_stmt.cpp
* @brief this class  present a "cursor fetch" logical plan in oceanbase
*
* Created by zhounan: support curosr
*
* @version __DaSE_VERSION
* @author zhounan <zn4work@gmail.com>
* @date 2014_11_23
*/
#include "ob_cursor_fetch_stmt.h"
using namespace oceanbase::common;
using namespace oceanbase::sql;

void ObCursorFetchStmt::print(FILE* fp, int32_t level, int32_t index)
{
  UNUSED(index);
  print_indentation(fp, level);
  fprintf(fp, "<ObCursorFetchStmt %d Begin>\n", index);
  print_indentation(fp, level + 1);
  fprintf(fp, "\n");
  print_indentation(fp, level + 1);
  fprintf(fp, "<ObCursorFetchStmt %d End>\n", index);

}

