/* Copyright (c) 2026 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sql/operator/update_physical_operator.h"

#include "common/log/log.h"
#include "storage/field/field_meta.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"

RC UpdatePhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  unique_ptr<PhysicalOperator> &child = children_[0];
  RC                            rc    = child->open(trx);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  // Collect records first. Updating rows while scanning the table can invalidate
  // the scanner and, when an indexed column changes, alter the index being used.
  vector<Record> records;
  while (OB_SUCC(rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (tuple == nullptr) {
      child->close();
      return RC::INTERNAL;
    }

    RowTuple *row_tuple = static_cast<RowTuple *>(tuple);
    Record record;
    rc = record.copy_data(row_tuple->record().data(), row_tuple->record().len());
    if (OB_FAIL(rc)) {
      child->close();
      return rc;
    }
    record.set_rid(row_tuple->record().rid());
    records.emplace_back(std::move(record));
  }
  child->close();
  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to scan records for update: %s", strrc(rc));
    return rc;
  }

  for (Record &old_record : records) {
    Record new_record;
    rc = make_updated_record(old_record, new_record);
    if (OB_FAIL(rc)) {
      return rc;
    }

    rc = trx->update_record(table_, old_record, new_record);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to update record: %s", strrc(rc));
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::next()
{
  return RC::RECORD_EOF;
}

RC UpdatePhysicalOperator::close()
{
  return RC::SUCCESS;
}

RC UpdatePhysicalOperator::make_updated_record(const Record &old_record, Record &new_record) const
{
  RC rc = new_record.copy_data(old_record.data(), old_record.len());
  if (OB_FAIL(rc)) {
    return rc;
  }
  new_record.set_rid(old_record.rid());

  rc = new_record.reset_filed(field_->offset(), field_->len());
  if (OB_FAIL(rc)) {
    return rc;
  }

  int copy_len = field_->len();
  if (field_->type() == AttrType::CHARS && copy_len > value_.length()) {
    copy_len = value_.length() + 1;  // retain the terminating null character
  }
  return new_record.set_field(field_->offset(), copy_len, value_.data());
}
