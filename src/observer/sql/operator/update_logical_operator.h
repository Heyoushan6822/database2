/* Copyright (c) 2026 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/value.h"
#include "sql/operator/logical_operator.h"

class Table;
class FieldMeta;

/**
 * @brief Logical operator for UPDATE statements.
 */
class UpdateLogicalOperator : public LogicalOperator
{
public:
  UpdateLogicalOperator(Table *table, const FieldMeta *field, Value value)
      : table_(table), field_(field), value_(std::move(value))
  {}

  LogicalOperatorType type() const override { return LogicalOperatorType::UPDATE; }
  OpType              get_op_type() const override { return OpType::LOGICALUPDATE; }

  Table            *table() const { return table_; }
  const FieldMeta   *field() const { return field_; }
  const Value       &value() const { return value_; }

private:
  Table           *table_ = nullptr;
  const FieldMeta *field_ = nullptr;
  Value            value_;
};
