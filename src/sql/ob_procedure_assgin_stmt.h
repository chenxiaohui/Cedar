#ifndef OCEANBASE_SQL_OB_PROCEDURE_ASSGIN_STMT_H_
#define OCEANBASE_SQL_OB_PROCEDURE_ASSGIN_STMT_H_
#include "common/ob_string.h"
#include "common/ob_string_buf.h"
#include "common/ob_array.h"
#include "ob_basic_stmt.h"
#include "parse_node.h"
#include "ob_sql_expression.h"
using namespace oceanbase::common;

namespace oceanbase {
  namespace sql {

    struct VariableSet
    {
      VariableSet() {}
      VariableSet(ObArray<ObString> &array) : var_set_(array)
      {}

      int addVariable(ObString &var_name)
      {
        bool flag = false;
        for(int64_t i = 0; i < var_set_.count(); ++i)
        {
          if( var_name.compare(var_set_.at(i)) == 0 )
          {
            flag = true;
            break;
          }
        }
        if( !flag ) var_set_.push_back(var_name);
        return OB_SUCCESS;
      }

//      NEED_SERIALIZE_AND_DESERIALIZE;

      ObArray<ObString> var_set_;
    };

    struct ObVarAssignVal
    {
      ObString    variable_name_;/*参数名称*/
      uint64_t	var_expr_id_;
      ObSqlExpression* var_value_;/*赋值*/
      VariableSet rs_;
      int add_rs_var(ObString &r_var)
      {
        rs_.addVariable(r_var); //potential has some memory problem
        return OB_SUCCESS;
      }

    };

    class ObProcedureAssginStmt: public ObBasicStmt {
    public:
      ObProcedureAssginStmt() :
              ObBasicStmt(T_PROCEDURE_ASSGIN) {
      }
      virtual ~ObProcedureAssginStmt() {
      }


      int add_var_val(ObVarAssignVal &var_val);/*添加一个var_val*/

      ObArray<ObVarAssignVal>& get_var_val_list();/*返回所有赋值*/

      ObVarAssignVal& get_var_val(int64_t index);/*返回一个赋值*/

      int64_t get_var_val_size();/*返回变量列表大小*/

      virtual void print(FILE* fp, int32_t level, int32_t index);
    private:
      ObArray<ObVarAssignVal> var_val_list_;/*赋值变量列表*/
    };
  }
}

#endif
