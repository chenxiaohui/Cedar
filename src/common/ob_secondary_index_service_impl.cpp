/*
 * ob_secondary_index_service_impl.cpp
 *
 *  Created on: 2015年10月29日
 *      Author: longfei
 */

#include "ob_secondary_index_service_impl.h"
#include "ob_postfix_expression.h"
#include "ob_define.h"
#include "ob_schema.h"

using namespace oceanbase;
using namespace common;
using namespace sql;

int ObSecondaryIndexServiceImpl::init(const ObSchemaManagerV2* schema_manager_)
{
//  TBSYS_LOG(ERROR, "test::longfei>>>init secondary index service.");
  int ret = OB_SUCCESS;
  if(NULL == schema_manager_)
  {
    TBSYS_LOG(ERROR,"sql context isn't init!");
    ret = OB_ERROR;
  }
  this->schema_manager_ = schema_manager_;
  return ret;
}

ObSecondaryIndexServiceImpl::ObSecondaryIndexServiceImpl() :
    schema_manager_(NULL)
{
}

const ObSchemaManagerV2* oceanbase::common::ObSecondaryIndexServiceImpl::getSchemaManager() const
{
  return schema_manager_;
}

void oceanbase::common::ObSecondaryIndexServiceImpl::setSchemaManager(
    const ObSchemaManagerV2* schemaManager)
{
  schema_manager_ = schemaManager;
}

/*************************************************************************
 * find the information in OB's expression
 *************************************************************************/

int ObSecondaryIndexServiceImpl::find_cid(ObSqlExpression& sql_expr,
    uint64_t &cid)  //获得表达式中列的cid
{
  int ret = OB_SUCCESS;
  //bool return_ret = false;
  int64_t type = 0;
  const sql::ObPostfixExpression& pf_expr = sql_expr.get_decoded_expression();
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  int32_t column_count = 0;
  int64_t tmp_cid = OB_INVALID_ID;
  int64_t tid = OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      //return_ret=true;
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      if (OB_SUCCESS != (ret = expr_[idx + 1].get_int(tid)))
      {
        //return_ret=true;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = expr_[idx + 2].get_int(tmp_cid)))
      {
        //return_ret=true;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else
      {
        column_count++;
        idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
      }
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW
        || type == CONST_OBJ || type == END || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      ret = OB_ERROR;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
  if (column_count == 1 && ret == OB_SUCCESS)
  {
    cid = tmp_cid;
//    TBSYS_LOG(ERROR, "test::longfei>>>in find_cid() func() && cid = %d", (int)cid);
  }
  else
  {
    ret = OB_ERROR;
    TBSYS_LOG(WARN, "too many columns in one expr");
  }
  return ret;
}

//add fanqiushi_index
int ObSecondaryIndexServiceImpl::change_tid(ObSqlExpression* sql_expr, uint64_t& array_index) //获得表达式中记录列的tid的ObObj在ObObj数组里的下标
{

  int ret = OB_SUCCESS;
  int64_t type = 0;
  const sql::ObPostfixExpression& pf_expr = sql_expr->get_decoded_expression();
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  int32_t column_count = 0;
  int64_t tmp_index = OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      tmp_index = idx + 1;
      column_count++;
      idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW || type == CONST_OBJ || type == END || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      ret = OB_ERROR;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
  if (column_count == 1 && ret == OB_SUCCESS)
    array_index = (uint64_t) tmp_index;
  else
    ret = OB_ERROR;
  return ret;
}

int ObSecondaryIndexServiceImpl::get_cid(ObSqlExpression* sql_expr, uint64_t& cid)  //获得表达式中列的cid，如果表达式中有多个列，则报错
{
  int ret = OB_SUCCESS;
  int64_t type = 0;
  const sql::ObPostfixExpression& pf_expr = sql_expr->get_decoded_expression();
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  int32_t column_count = 0;
  int64_t tmp_cid = OB_INVALID_ID;
  int64_t tmp_tid = OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      if (OB_SUCCESS != (ret = expr_[idx + 1].get_int(tmp_tid)))
      {
        //return_ret=false;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = expr_[idx + 2].get_int(tmp_cid)))
      {
        //return_ret=false;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else
      {
        column_count++;
        idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
      }
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW || type == CONST_OBJ || type == END || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      ret = OB_ERROR;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
  if (column_count == 1 && ret == OB_SUCCESS)
    cid = (uint64_t) tmp_cid;
  else
    ret = OB_ERROR;
  return ret;
}
//add:e

//判断该表达式的所有列是否都在索引表index_tid中
bool ObSecondaryIndexServiceImpl::is_all_expr_cid_in_indextable(
    uint64_t index_tid, const sql::ObPostfixExpression& pf_expr,
    const ObSchemaManagerV2 *sm_v2)
{
  bool return_ret = true;
  int ret = OB_SUCCESS;
  //bool return_ret = false;
  int64_t type = 0;
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  //int32_t column_count=0;
  int64_t tmp_cid = OB_INVALID_ID;
  int64_t tid = OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      return_ret = false;
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      if (OB_SUCCESS != (ret = expr_[idx + 1].get_int(tid)))
      {
        return_ret = false;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = expr_[idx + 2].get_int(tmp_cid)))
      {
        return_ret = false;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else
      {
        const ObColumnSchemaV2* column_schema = NULL;
        if (NULL
            == (column_schema = sm_v2->get_column_schema(index_tid, tmp_cid)))
        {
          return_ret = false;
          break;
        }
        else
          idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
      }
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW  // ??
    || type == CONST_OBJ || type == END || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      return_ret = false;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
  return return_ret;
}

bool ObSecondaryIndexServiceImpl::is_have_main_cid(
    sql::ObSqlExpression& sql_expr, uint64_t main_column_id)
{ //如果表达式中有主表的第一主键，或者表达式中有超过两列的，返回true
//  TBSYS_LOG(ERROR, "test::longfei>>>in is_have_main_cid() func.");
  int ret = OB_SUCCESS;
  bool return_ret = false;
  int64_t type = 0;
  const sql::ObPostfixExpression& pf_expr = sql_expr.get_decoded_expression();
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  int32_t column_count = 0;
  int64_t cid = OB_INVALID_ID;
  int64_t tid = OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      return_ret = true;
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      if (OB_SUCCESS != (ret = expr_[idx + 1].get_int(tid)))
      {
        return_ret = true;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = expr_[idx + 2].get_int(cid)))
      {
        return_ret = true;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else
      {
        if (cid == (int64_t) main_column_id)
        {
          return_ret = true;
          break;
        }
        else
        {
          column_count++;
          idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
        }
      }
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW
        || type == CONST_OBJ || type == END || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      return_ret = true;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
  if (column_count > 1)
    return_ret = true;
  return return_ret;
}

//获得表达式中所有列的存tid的ObObj在ObObj数组里的下标
int ObSecondaryIndexServiceImpl::get_all_cloumn(sql::ObSqlExpression& sql_expr,
    ObArray<uint64_t> &column_index)
{
  //bool return_ret=true;
  int ret = OB_SUCCESS;
  //bool return_ret = false;
  int64_t type = 0;
  const sql::ObPostfixExpression& pf_expr = sql_expr.get_decoded_expression();
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  //int32_t column_count=0;
  //int64_t tmp_cid = OB_INVALID_ID;
  // int64_t tid = OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      //return_ret=false;
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      column_index.push_back(idx + 1);
      idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW
        || type == CONST_OBJ || type == END
        || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      ret = OB_ERROR;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
  return ret;
}

bool ObSecondaryIndexServiceImpl::is_this_expr_can_use_index(
    sql::ObSqlExpression& sql_expr, uint64_t &index_tid, uint64_t main_tid,
    const ObSchemaManagerV2 *sm_v2)
{
  //判断该表达式是否能够使用索引。如果该表达式只有一列，并且是个等值或in表达式，并且该表达式的列的cid是主表main_tid的某一张索引表的第一主键，则该表达式能够使用索引
  int ret = OB_SUCCESS;
  bool return_ret = false;
  int64_t type = 0;
  int64_t tmp_type = 0;
  const sql::ObPostfixExpression& pf_expr = sql_expr.get_decoded_expression();
  const ExprArray& expr_ = pf_expr.get_expr();
  int64_t count = expr_.count();
  int64_t idx = 0;
  int32_t column_count = 0;
  int32_t EQ_count = 0;
  int32_t IN_count = 0;
  int64_t cid = OB_INVALID_ID;
  int64_t tid = OB_INVALID_ID;
  //uint64_t tmp_index_tid= OB_INVALID_ID;
  while (idx < count)
  {
    if (OB_SUCCESS != (ret = expr_[idx].get_int(type)))
    {
      return_ret = false;
      TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
      break;
    }
    else if (type == COLUMN_IDX)
    {
      if (OB_SUCCESS != (ret = expr_[idx + 1].get_int(tid)))
      {
        return_ret = false;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else if (OB_SUCCESS != (ret = expr_[idx + 2].get_int(cid)))
      {
        return_ret = false;
        TBSYS_LOG(WARN, "Fail to get op type. unexpected! ret=%d", ret);
        break;
      }
      else
      {
        column_count++;
        idx = idx + pf_expr.get_type_num(idx, COLUMN_IDX);
      }
    }
    else if (type == OP)
    {
      if (ObIntType != expr_[idx + 1].get_type())
      {
        return_ret = false;
        ret = OB_ERROR;
        break;
      }
      else if (OB_SUCCESS != (ret = expr_[idx + 1].get_int(tmp_type)))
      {
        TBSYS_LOG(WARN, "fail to get int value.err=%d", ret);
        return_ret = false;
        break;
      }
      else if (tmp_type == T_OP_EQ)
      {
        EQ_count++;
        idx = idx + pf_expr.get_type_num(idx, OP);
      }
      else if (tmp_type == T_OP_IN)
      {
        IN_count++;
        idx = idx + pf_expr.get_type_num(idx, OP);
      }
      else
      {
        idx = idx + pf_expr.get_type_num(idx, OP);
      }
    }
    //add fanqiushi_index_prepare
    else if (type == OP || type == COLUMN_IDX || type == T_OP_ROW
        || type == CONST_OBJ || type == END
        || type == UPS_TIME_OP)
    {
      idx = idx + pf_expr.get_type_num(idx, type);
    }
    else
    {
      return_ret = false;
      TBSYS_LOG(WARN, "wrong expr type: %ld", type);
      break;
    }
    //add:e
  }
//  TBSYS_LOG(ERROR,"test::fanqs,column_count=%d,EQ_count=%d",column_count,EQ_count);
  if ((column_count == 1 && EQ_count == 1)
      || (column_count == 1 && IN_count == 1))
  {
    uint64_t tmp_index_tid[OB_MAX_INDEX_NUMS];
    for (int32_t m = 0; m < OB_MAX_INDEX_NUMS; m++)
    {
      tmp_index_tid[m] = OB_INVALID_ID;
    }
    if (sm_v2->is_cid_in_index(cid, main_tid, tmp_index_tid))
    {
      index_tid = tmp_index_tid[0];
      return_ret = true;
//      TBSYS_LOG(ERROR,"test::fanqs,column_count=%d,EQ_count=%d",column_count,EQ_count);
    }
  }
  return return_ret;
}

/*************************************************************************
 * for transformer to generate physical plan
 *************************************************************************/

bool ObSecondaryIndexServiceImpl::is_this_table_avalibale(uint64_t tid) //判断tid为参数的表是否是可用的索引表
{
  bool ret = false;
  const ObTableSchema *main_table_schema = NULL;
  if (NULL == (main_table_schema = schema_manager_->get_table_schema(tid)))
  {
    //ret = OB_ERR_ILLEGAL_ID;
    TBSYS_LOG(WARN, "fail to get table schema for table[%ld]", tid);
  }
  else
  {
    if (main_table_schema->get_index_status() == INDEX_INIT)
    {
      ret = true;
    }

  }
  return ret;
}

bool ObSecondaryIndexServiceImpl::is_index_table_has_all_cid_V2(
    uint64_t index_tid, Expr_Array *filter_array, Expr_Array *project_array)
{
  //判断索引表是否包含sql语句中出现的所有列
  bool ret = true;
  if (is_this_table_avalibale(index_tid))
  {
    int64_t w_num = project_array->count();
    for (int32_t i = 0; i < w_num; i++)
    {
      ObSqlExpression col_expr = project_array->at(i);
      const sql::ObPostfixExpression& postfix_pro_expr =
          col_expr.get_decoded_expression();
//      TBSYS_LOG(ERROR,"test::fanqs,,col_expr=%s",to_cstring(col_expr));
      if (!is_all_expr_cid_in_indextable(index_tid, postfix_pro_expr,
          schema_manager_))
      {
        ret = false;
        break;
      }

    }
    int64_t c_num = filter_array->count();
    for (int32_t j = 0; j < c_num; j++)
    {

      ObSqlExpression c_filter = filter_array->at(j);
      const sql::ObPostfixExpression& postfix_fil_expr =
          c_filter.get_decoded_expression();
      if (!is_all_expr_cid_in_indextable(index_tid, postfix_fil_expr,
          schema_manager_))
      {
        ret = false;
        break;
      }
    }
  }
//  TBSYS_LOG(ERROR,"test::fanqs,,return_ret=%d,,index_tid=%ld",ret,index_tid);
  return ret;
}

int64_t ObSecondaryIndexServiceImpl::is_cid_in_index_table(uint64_t cid,
    uint64_t tid)
{
  //判断该列是否在该索引表中。结果是0，表示不在；结果��?1，表示该列是索引表的主键；结果是2，表示该列是索引表的非主��?
  int64_t return_ret = 0;
  int ret = OB_SUCCESS;
  bool is_in_rowkey = false;
  bool is_in_other_column = false;
  const ObTableSchema *index_table_schema = NULL;
  if (NULL == (index_table_schema = schema_manager_->get_table_schema(tid)))
  {
    ret = OB_ERROR;
    TBSYS_LOG(WARN, "Fail to get table schema for table[%ld]", tid);
  }
  else
  {
    uint64_t tmp_cid = OB_INVALID_ID;
    int64_t rowkey_column = index_table_schema->get_rowkey_info().get_size();
    for (int64_t j = 0; j < rowkey_column; j++)
    {
      if (OB_SUCCESS
          != (ret = index_table_schema->get_rowkey_info().get_column_id(j,
              tmp_cid)))
      {
        TBSYS_LOG(ERROR, "get column schema failed,cid[%ld]", tmp_cid);
        ret = OB_SCHEMA_ERROR;
      }
      else
      {
        if (tmp_cid == cid)
        {
          is_in_rowkey = true;
          break;
        }
      }
    }
    // TBSYS_LOG(ERROR,"test::fanqs,,cid=%ld,is_in_rowkey=%d",cid,is_in_rowkey);
    if (!is_in_rowkey)
    {
      /* uint64_t max_cid=OB_INVALID_ID;
       max_cid=index_table_schema->get_max_column_id();
       //TBSYS_LOG(ERROR,"test::fanqs,,max_cid=%ld,",max_cid);
       for(uint64_t k=OB_APP_MIN_COLUMN_ID;k<=max_cid;k++)
       {
       if(cid==k)
       {
       is_in_other_column=true;
       break;
       }
       }*/
      const ObColumnSchemaV2* index_column_schema = NULL;
      index_column_schema = schema_manager_->get_column_schema(tid, cid);
      if (index_column_schema != NULL)
      {
        is_in_other_column = true;
      }
    }
  }
  if (is_in_rowkey)
    return_ret = 1;
  else if (is_in_other_column)
    return_ret = 2;
  return return_ret;
}

bool ObSecondaryIndexServiceImpl::is_expr_can_use_storing_V2(
    ObSqlExpression c_filter, uint64_t mian_tid, uint64_t &index_tid,
    Expr_Array * filter_array, Expr_Array *project_array)
{
  //输出：bool类型返回��?; uint64_t &index_tid：索引表的tid
  bool ret = false;
  uint64_t expr_cid = OB_INVALID_ID;
  uint64_t tmp_index_tid = OB_INVALID_ID;
  uint64_t index_tid_array[OB_MAX_INDEX_NUMS];
  for (int32_t k = 0; k < OB_MAX_INDEX_NUMS; k++)
  {
    index_tid_array[k] = OB_INVALID_ID;
  }
  if (OB_SUCCESS == find_cid(c_filter, expr_cid)) //获得表达式中存的列的column id:expr_cid。如果表达式中有多列，返回ret不等于OB_SUCCESS
  {
    if (schema_manager_->is_cid_in_index(expr_cid, mian_tid, index_tid_array)) //根据原表的tid，找到该表的��?有的第一主键为expr_cid的索引表，存到index_tid_array里面
    {
      for (int32_t i = 0; i < OB_MAX_INDEX_NUMS; i++)  //对每��?张符合条件的索引��?
      {
//        TBSYS_LOG(ERROR,"test::fanqs,,index_tid_array[i]=%ld",index_tid_array[i]);
        //uint64_t tmp_tid=index_tid_array[i];
        if (index_tid_array[i] != OB_INVALID_ID)
        {
          //判断是否��?有在sql语句里面出现的列，都在这张索引表��?
          if (is_index_table_has_all_cid_V2(index_tid_array[i], filter_array,
              project_array))
          {
            tmp_index_tid = index_tid_array[i];
//            TBSYS_LOG(ERROR,"test::fanqs,,tmp_index_tid=%ld",tmp_index_tid);
            ret = true;
            break;
          }
        }
      }
      index_tid = tmp_index_tid;
    }
  }
  return ret;
}

bool ObSecondaryIndexServiceImpl::is_wherecondition_have_main_cid_V2(
    Expr_Array *filter_array, uint64_t main_cid)
{   //如果where条件的某个表达式有main_cid或�?�某个表达式有多个列,返回true
  bool return_ret = false;
  int ret = OB_SUCCESS;

  int64_t c_num = filter_array->count();
  int32_t i = 0;
  for (; ret == OB_SUCCESS && i < c_num; i++)
  {
    ObSqlExpression c_filter = filter_array->at(i);
    if (is_have_main_cid(c_filter, main_cid))
    {
      return_ret = true;
      break;
    }
  }
//  TBSYS_LOG(ERROR, "test::longfei>>>in is_wherecondition_have_main_cid_V2() func and return_ret is %s", return_ret?"true":"false");
  return return_ret;
}

//add wenghaixing [secondary index for paper]20150505
bool ObSecondaryIndexServiceImpl::if_rowkey_in_expr(Expr_Array *filter_array,
    uint64_t main_tid)
{
  bool return_ret = false;
  uint64_t tid = main_tid;
// uint64_t index_tid=OB_INVALID_ID;
  const ObTableSchema *mian_table_schema = NULL;
  if (NULL == (mian_table_schema = schema_manager_->get_table_schema(tid)))
  {
    TBSYS_LOG(WARN, "Fail to get table schema for table[%ld]", tid);
  }
  else
  {
    const ObRowkeyInfo *rowkey_info = &mian_table_schema->get_rowkey_info();
    uint64_t main_cid = OB_INVALID_ID;
    rowkey_info->get_column_id(0, main_cid); //获得原表的第��?主键的column id,存到main_cid里�??
    return_ret = is_wherecondition_have_main_cid_V2(filter_array, main_cid); //判断where条件中是否有原表的第��?主键，如果有，则不用索引

  }
  return return_ret;
}
//add e

bool ObSecondaryIndexServiceImpl::decide_is_use_storing_or_not_V2(
    Expr_Array *filter_array, Expr_Array *project_array,
    uint64_t &index_table_id, uint64_t main_tid)
{
//  TBSYS_LOG(ERROR, "test::longfei>>>in decide_is_use_storing_or_not_V2() func.");
  //输出：bool类型   返回值： uint64_t &index_table_id：索引表的tid
  bool return_ret = false;
  int ret = OB_SUCCESS;

  uint64_t tid = main_tid;
  uint64_t index_tid = OB_INVALID_ID;
  const ObTableSchema *mian_table_schema = NULL;
  if (NULL == (mian_table_schema = schema_manager_->get_table_schema(tid)))
  {
    TBSYS_LOG(WARN, "Fail to get table schema for table[%ld]", tid);
  }
  else
  {
    const ObRowkeyInfo *rowkey_info = &mian_table_schema->get_rowkey_info();
    uint64_t main_cid = OB_INVALID_ID;
    rowkey_info->get_column_id(0, main_cid); //获得原表的第一主键的column id,存到main_cid里�??
    if (!is_wherecondition_have_main_cid_V2(filter_array, main_cid)) //判断where条件中是否有原表的第一主键，如果有，则不用索引
    {
      int64_t c_num = filter_array->count();
//      TBSYS_LOG(ERROR, "test::longfei>>>do not have original tab main_cid in filter array and # of filter array is %ld", c_num);
      int32_t i = 0;
      for (; ret == OB_SUCCESS && i < c_num; i++)    //对where条件中的��?有表达式依次处理
      {
        ObSqlExpression c_filter = filter_array->at(i);
//        TBSYS_LOG(ERROR, "test::longfei>>>filter arrary is %s",to_cstring(c_filter));
        //判断该表达式能否使用不回表的索引
        if (is_expr_can_use_storing_V2(c_filter, tid, index_tid, filter_array,
            project_array))
        {
          index_table_id = index_tid;
          return_ret = true;
          break;
        }
      }
    }
  }
//  TBSYS_LOG(ERROR,"test::fanqs,,return_ret=%d,index_table_id=%ld",return_ret,index_table_id);
  return return_ret;
}

bool ObSecondaryIndexServiceImpl::is_can_use_hint_for_storing_V2(
    Expr_Array *filter_array, Expr_Array *project_array,
    uint64_t index_table_id)
{
//  TBSYS_LOG(ERROR, "test::longfei>>>in is_can_use_hint_for_storing_V2() func.");
  bool cond_has_main_cid = false;
  bool can_use_hint_for_storing = false;
  const ObTableSchema *index_table_schema = NULL;
  if (NULL == (index_table_schema = schema_manager_->get_table_schema(index_table_id)))
  {
    TBSYS_LOG(WARN, "Fail to get table schema for table[%ld]", index_table_id);
  }
  else if (schema_manager_->is_this_table_avalibale(index_table_id))
  {
    const ObRowkeyInfo& rowkey_info = index_table_schema->get_rowkey_info();
    uint64_t index_key_cid = OB_INVALID_ID;
    // 获得索引表的第一主键的column id
    if (OB_SUCCESS != rowkey_info.get_column_id(0, index_key_cid))
    {
      TBSYS_LOG(WARN,
          "Fail to get column id, index_table name:[%s], index_table id: [%ld]",
          index_table_schema->get_table_name(),
          index_table_schema->get_table_id());
      cond_has_main_cid = false;
    }
    // 判断where条件的表达式中是否包含索引表的第��?主键，每个表达式都只有一列且其中有一列是索引表的第一主键时返回true
    else if (!is_wherecondition_have_main_cid_V2(filter_array, index_key_cid))
    {
      cond_has_main_cid = false;
    }
    else
    {
      cond_has_main_cid = true;
    }
  }

  if (cond_has_main_cid)
  {
    // 如果where条件中包含索引表的第��?主键再判断这些表达式中的列和select的输出列是不是都在索引表��?
    can_use_hint_for_storing = is_index_table_has_all_cid_V2(index_table_id,
        filter_array, project_array);
  }
  // 如果对于where条件不能使用主键索引的情况则认为不能使用索引表的storing��?
  else
  {
    can_use_hint_for_storing = false;
  }

  return can_use_hint_for_storing;
}

bool ObSecondaryIndexServiceImpl::is_can_use_hint_index_V2(
    Expr_Array *filter_ayyay, uint64_t index_table_id)
{
  bool can_use_hint_index = false;
  bool cond_has_main_cid = false;
  const ObTableSchema *index_table_schema = NULL;
  if (NULL
      == (index_table_schema = schema_manager_->get_table_schema(index_table_id)))
  {
    TBSYS_LOG(WARN, "Fail to get table schema for table[%ld]", index_table_id);
  }
  else
  {
    const ObRowkeyInfo& rowkey_info = index_table_schema->get_rowkey_info();
    uint64_t index_key_cid = OB_INVALID_ID;
    // 获得索引表的第一主键的column id
    if (OB_SUCCESS != rowkey_info.get_column_id(0, index_key_cid))
    {
      TBSYS_LOG(WARN,
          "Fail to get column id, index_table name:[%s], index_table id: [%ld]",
          index_table_schema->get_table_name(),
          index_table_schema->get_table_id());
      cond_has_main_cid = false;
    }
    // 判断where条件的表达式中是否包含索引表的第��?主键，每个表达式都只有一列且其中有一列是索引表的第一主键时返回true
    else if (!is_wherecondition_have_main_cid_V2(filter_ayyay, index_key_cid))
    {
      cond_has_main_cid = false;
    }
    else
    {
      cond_has_main_cid = true;
    }
  }

  if (cond_has_main_cid)
  {
    can_use_hint_index = true;
  }
  if (!schema_manager_->is_this_table_avalibale(index_table_id))
  {
    can_use_hint_index = false;
  }

  return can_use_hint_index;
}
//add:e

