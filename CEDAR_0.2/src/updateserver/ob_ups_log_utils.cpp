/**
 * Copyright (C) 2013-2016 DaSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * @file ob_ups_log_utils.cpp
 * @brief utilities for commit log of ups
 *     modify by liubozhong: support multiple clusters for HA by
 *     adding or modifying some functions, member variables
 *
 * @version CEDAR 0.2
 * @author liubozhong <51141500077@ecnu.cn>
 * @date 2015_12_30
 */
/**
 * (C) 2007-2010 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Version: $Id$
 *
 * Authors:
 *   yuanqi <yuanqi.xhf@taobao.com>
 *     - some work details if you want
 */

#include "ob_ups_log_utils.h"
#include "common/ob_log_reader.h"
#include "common/ob_direct_log_reader.h"
#include "common/ob_log_dir_scanner.h"
#include "ob_ups_table_mgr.h"
#include "common/file_utils.h"
#include "common/file_directory_utils.h"
#include "ob_ups_log_mgr.h"

using namespace oceanbase::common;
namespace oceanbase
{
  namespace updateserver
  {
    const char* ObLogReplayPoint::REPLAY_POINT_FILE = "log_replay_point";

    int load_replay_point_func(const char* log_dir, const char* replay_point_file, int64_t& replay_point)
    {
      int err = 0;
      int64_t len = 0;
      int open_ret = 0;
      const int UINT64_MAX_LEN = 16;
      char rplpt_fn[OB_MAX_FILE_NAME_LENGTH];
      char rplpt_str[UINT64_MAX_LEN];
      int rplpt_str_len = 0;
      FileUtils rplpt_file;

      if (NULL == log_dir)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "Arguments are invalid[log_dir=%p]", log_dir);
      }
      else if ((len = snprintf(rplpt_fn, sizeof(rplpt_fn), "%s/%s", log_dir, replay_point_file) < 0)
               && len >= (int64_t)sizeof(rplpt_fn))
      {
        err = OB_ERROR;
        TBSYS_LOG(ERROR, "generate_replay_point_fn()=>%d", err);
      }
      else if (!FileDirectoryUtils::exists(rplpt_fn))
      {
        err = OB_FILE_NOT_EXIST;
        TBSYS_LOG(WARN, "replay point file[\"%s\"] does not exist", rplpt_fn);
      }
      else if (0 > (open_ret = rplpt_file.open(rplpt_fn, O_RDONLY)))
      {
        err = OB_IO_ERROR;
        TBSYS_LOG(WARN, "open file[\"%s\"] error[%s]", rplpt_fn, strerror(errno));
      }
      else if (0 > (rplpt_str_len = static_cast<int32_t>(rplpt_file.read(rplpt_str, sizeof(rplpt_str))))
               || rplpt_str_len >= (int)sizeof(rplpt_str))
      {
        err = rplpt_str_len < 0 ? OB_IO_ERROR: OB_BUF_NOT_ENOUGH;
        TBSYS_LOG(ERROR, "read error[%s] or file contain invalid data[len=%d]", strerror(errno), rplpt_str_len);
      }
      else
      {
        rplpt_str[rplpt_str_len] = '\0';
        const int STRTOUL_BASE = 10;
        char* endptr;
        replay_point = strtoul(rplpt_str, &endptr, STRTOUL_BASE);
        if ('\0' != *endptr)
        {
          err = OB_INVALID_DATA;
          TBSYS_LOG(ERROR, "non-digit exist in replay point file[rplpt_str=%.*s]", rplpt_str_len, rplpt_str);
        }
        else if (ERANGE == errno)
        {
          err = OB_INVALID_DATA;
          TBSYS_LOG(ERROR, "replay point contained in replay point file is out of range");
        }
      }
      if (0 > open_ret)
      {
        rplpt_file.close();
      }
      return err;
    }

    int write_replay_point_func(const char* log_dir, const char* replay_point_file, const int64_t replay_point)
    {
      int err = OB_SUCCESS;
      const int UINT64_MAX_LEN = 16;
      int len = 0;
      int open_ret = 0;
      FileUtils rplpt_file;
      char rplpt_fn[OB_MAX_FILE_NAME_LENGTH];
      char rplpt_str[UINT64_MAX_LEN];
      int64_t rplpt_str_len = 0;

      if (NULL == log_dir)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "Arguments are invalid[log_dir=%p]", log_dir);
      }
      else if ((len = snprintf(rplpt_fn, sizeof(rplpt_fn), "%s/%s", log_dir, replay_point_file) < 0)
               && len >= (int64_t)sizeof(rplpt_fn))
      {
        err = OB_BUF_NOT_ENOUGH;
        TBSYS_LOG(ERROR, "generate_replay_point_fn()=>%d", err);
      }
      else if (0 > (open_ret = rplpt_file.open(rplpt_fn, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)))
      {
        err = OB_FILE_NOT_EXIST;
        TBSYS_LOG(ERROR, "open file[\"%s\"] error[%s]", rplpt_fn, strerror(errno));
      }
      else if ((rplpt_str_len = snprintf(rplpt_str, sizeof(rplpt_str), "%lu", replay_point)) < 0
               || rplpt_str_len >= (int64_t)sizeof(rplpt_str))
      {
        err = OB_BUF_NOT_ENOUGH;
        TBSYS_LOG(ERROR, "snprintf rplpt_str error[%s][replay_point=%lu]", strerror(errno), replay_point);
      }
      else if (0 > (rplpt_str_len = rplpt_file.write(rplpt_str, rplpt_str_len)))
      {
        err = OB_ERR_SYS;
        TBSYS_LOG(ERROR, "write error[%s][rplpt_str=%p rplpt_str_len=%ld]", strerror(errno), rplpt_str, rplpt_str_len);
      }
      if (0 > open_ret)
      {
        rplpt_file.close();
      }
      return err;
    }

    int scan_log_dir_func(const char* log_dir, const char* replay_point_file,
                          int64_t& replay_point, int64_t& min_log_file_id, int64_t& max_log_file_id)
    {
      int err = OB_SUCCESS;
      ObLogDirScanner scanner;
      min_log_file_id = 0;
      max_log_file_id = 0;
      replay_point = 0;
      if (NULL == log_dir)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "log_dir == NULL");
      }
      else if (OB_SUCCESS != (err = scanner.init(log_dir))
               && OB_DISCONTINUOUS_LOG != err)
      {
        TBSYS_LOG(ERROR, "scanner.init(log_dir=%s)=>%d", log_dir, err);
      }
      else if (OB_SUCCESS != (err = scanner.get_min_log_id((uint64_t&)min_log_file_id)) && OB_ENTRY_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "scanner.get_min_log_file_id()=>%d", err);
      }
      else if (OB_SUCCESS != (err = scanner.get_max_log_id((uint64_t&)max_log_file_id)) && OB_ENTRY_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "get_max_log_file_id error[ret=%d]", err);
      }
      else if (OB_SUCCESS != (err = load_replay_point_func(log_dir, replay_point_file, replay_point))
               && OB_FILE_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "load_replay_point(log_dir=%s)=>%d", log_dir, err);
      }
      else
      {
        if (0 >= min_log_file_id)
        {
          min_log_file_id = 0;
        }
        if (0 >= replay_point)
        {
          replay_point = min_log_file_id;
        }
        if (min_log_file_id > replay_point || (max_log_file_id >0 && replay_point > max_log_file_id))
        {
          err = OB_DISCONTINUOUS_LOG;
          TBSYS_LOG(WARN, "min_log_file_id=%ld, max_log_file_id=%ld, replay_point=%ld", min_log_file_id, max_log_file_id, replay_point);
        }
      }
      if (OB_FILE_NOT_EXIST == err)
      {
        err = OB_SUCCESS;
      }
      return err;
    }

    int ObLogReplayPoint::init(const char* log_dir)
    {
      int err = OB_SUCCESS;
      log_dir_ = log_dir;
      return err;
    }

    int ObLogReplayPoint::write(const int64_t replay_point)
    {
      return write_replay_point_func(log_dir_, REPLAY_POINT_FILE, replay_point);
    }

    int ObLogReplayPoint::get(int64_t& replay_point)
    {
      int64_t min_log_id = -1;
      int64_t max_log_id = -1;
      return scan_log_dir_func(log_dir_, REPLAY_POINT_FILE, replay_point, min_log_id, max_log_id);
    }

    int64_t set_counter(tbsys::CThreadCond& cond, volatile int64_t& counter, const int64_t new_counter)
    {
      int64_t old_counter = 0;
      cond.lock();
      old_counter = counter;
      counter = new_counter;
      cond.broadcast();
      cond.unlock();
      return old_counter;
    }

    int64_t wait_counter(tbsys::CThreadCond& cond, volatile int64_t& counter, const int64_t limit, const int64_t timeout_us)
    {
      int64_t end_time_us = 0;
      int64_t wait_time_us = 0;
      int wait_time_ms = (int)(wait_time_us/1000LL);
      if (counter >= limit)
      {}
      else
      {
        end_time_us = tbsys::CTimeUtil::getTime() + timeout_us;
        wait_time_us = timeout_us;
        cond.lock();
        while((wait_time_ms = (int)(wait_time_us/1000LL)) > 0)
        {
          if (counter >= limit)
          {
            break;
          }
          else
          {
            cond.wait(wait_time_ms);
            wait_time_us = end_time_us - tbsys::CTimeUtil::getTime();
          }
        }
        cond.unlock();
      }
      return counter;
    }

    static bool is_align(int64_t x, int64_t n_bits)
    {
      return 0 == (x & ((1<<n_bits) - 1));
    }

    int get_local_max_log_cursor_func(const char* log_dir, const ObLogCursor& start_cursor, ObLogCursor& end_cursor)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      char* log_data = NULL;
      int64_t data_len = 0;
      LogCommand cmd = OB_LOG_UNKNOWN;
      uint64_t seq;
      end_cursor = start_cursor;

      if (NULL == log_dir || start_cursor.file_id_ <= 0 || start_cursor.log_id_ != 0 || start_cursor.offset_ != 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, 0, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (OB_SUCCESS == err)
      {
        if (OB_SUCCESS != (err = log_reader.read_log(cmd, seq, log_data, data_len)) &&
            OB_FILE_NOT_EXIST != err && OB_READ_NOTHING != err && OB_LAST_LOG_RUINNED != err)
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
        }
        else if (OB_LAST_LOG_RUINNED == err)
        {
          TBSYS_LOG(WARN, "last_log[%s] broken!", end_cursor.to_str());
          err = OB_ITER_END;
        }
        else if (OB_SUCCESS != err)
        {
          err = OB_ITER_END; // replay all
        }
        else if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
        }
      }
      if (OB_ITER_END == err)
      {
        err = OB_SUCCESS;
      }
      return err;
    }

    int get_local_max_log_cursor_func(const char* log_dir, const uint64_t log_file_id_by_sst, ObLogCursor& log_cursor)
    {
      int err = OB_SUCCESS;
      ObLogCursor new_cursor;
      int64_t max_log_file_id = 0;
      ObLogDirScanner scanner;
      set_cursor(log_cursor, 0, 0, 0);
      if (NULL == log_dir)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "log_dir == NULL");
      }
      else if (OB_SUCCESS != (err = scanner.init(log_dir)) && OB_DISCONTINUOUS_LOG != err)
      {
        TBSYS_LOG(ERROR, "scanner.init(log_dir=%s)=>%d", log_dir, err);
      }
      else if (OB_SUCCESS != (err = scanner.get_max_log_id((uint64_t&)max_log_file_id)) && OB_ENTRY_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "get_max_log_file_id error[ret=%d]", err);
      }
      else
      {
        err = OB_SUCCESS;
        if (OB_INVALID_ID != log_file_id_by_sst)
        {
          max_log_file_id = max(max_log_file_id, log_file_id_by_sst);
        }
        TBSYS_LOG(INFO, "max_log_file_id=%ld, log_file_id_by_sst=%ld", max_log_file_id, log_file_id_by_sst);
        log_cursor.file_id_ = max(max_log_file_id, 0);
        if (log_cursor.file_id_ >0 && OB_SUCCESS != (err = get_local_max_log_cursor_func(log_dir, log_cursor, new_cursor)))
        {
          TBSYS_LOG(ERROR, "get_max_local_cursor(file_id=%ld)=>%d", log_cursor.file_id_, err);
        }
        else
        {
          log_cursor = new_cursor;
        }
        // 可能最后一个文件存在，但是为空，这时如果前一个文件存在的话，从前一个文件开始找本地最后的日志
        if (OB_SUCCESS == err && log_cursor.log_id_ <= 0 && log_cursor.file_id_ - 1 > 0)
        {
          ObLogCursor tmp_cursor = log_cursor;
          tmp_cursor.file_id_--;
          if (OB_SUCCESS != (err = get_local_max_log_cursor_func(log_dir, tmp_cursor, new_cursor)))
          {
            TBSYS_LOG(ERROR, "get_max_local_cursor(): try previous file[id=%ld]=>%d", log_cursor.file_id_, err);
          }
          else if (new_cursor.log_id_ > 0)
          {
            log_cursor = new_cursor;
          }
        }
      }
      TBSYS_LOG(INFO, "get_local_max_log_cursor(return_cursor=%s)=>%d", log_cursor.to_str(), err);
      return err;
    }

    int replay_local_log_func(const volatile bool& stop, const char* log_dir,
                              const ObLogCursor& start_cursor, ObLogCursor& end_cursor,
                              ObLogReplayWorker& replay_worker
                              //add lbzhong [Commit Point] 20150930:b
                              ,const int64_t commit_seq
                              //add:e
                              )
    {
      int err = OB_SUCCESS;
      char* buf = NULL;
      int64_t len = ObLogWriter::LOG_BUFFER_SIZE;
      int64_t read_count = 0;
      int64_t end_id = 0;
      ObPosLogReader reader;
      ObLogLocation start_location;
      ObLogLocation end_location;
      int64_t start_time = tbsys::CTimeUtil::getTime();
      end_cursor = start_cursor;
      //add lbzhong [Commit Point] 20150930:b
      bool has_committed_end = false;
      //add:e

      if (NULL == log_dir || start_cursor.file_id_ <= 0 || start_cursor.log_id_ != 0 || start_cursor.offset_ != 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (NULL == (buf = (char*)ob_malloc(len, ObModIds::OB_UPS_COMMON)))
      {
        err = OB_ALLOCATE_MEMORY_FAILED;
        TBSYS_LOG(ERROR, "ob_malloc(len=%ld) fail", len);
      }
      else if (OB_SUCCESS != (err = get_first_log_id_func(log_dir, end_cursor.file_id_, end_cursor.log_id_, NULL))
               && OB_ENTRY_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "get_first_log_id_func()=>%d", err);
      }
      else if (OB_ENTRY_NOT_EXIST == err)
      {
        TBSYS_LOG(WARN, "replay_local_log(start_file=%ld): NOT EXIST", start_location.file_id_);
      }
      else if (OB_SUCCESS != (err = reader.init(log_dir, true)))
      {
        TBSYS_LOG(ERROR, "located_log_reader init error[err=%d]", err);
      }
      else if (OB_SUCCESS != (err = replay_worker.start_log(end_cursor)))
      {
        TBSYS_LOG(ERROR, "replay_worker.start_log(%s)=>%d", end_cursor.to_str(), err);
      }
      else
      {
        start_location.file_id_ = end_cursor.file_id_;
        start_location.log_id_ = end_cursor.log_id_;
        end_location = start_location;
      }
      while (
          //add lbzhong [Commit Point] 20150930:b
          !has_committed_end &&
          //add:e
          !stop && OB_SUCCESS == err)
      {
        if (OB_SUCCESS != (err = reader.get_log(start_location.log_id_, start_location, end_location,
                                                     buf, len, read_count
                                                     //add lbzhong [Commit Point] 20150930:b
                                                     , has_committed_end, commit_seq
                                                     //add:e
                                                     )))
        {
          TBSYS_LOG(ERROR, "reader.get_log(log_id=%ld)=>%d", start_location.log_id_, err);
        }
        else if (read_count <= 0)
        {
          break;
        }
        else if (OB_SUCCESS != (err = replay_worker.submit_batch(end_id, buf, read_count, RT_LOCAL)))
        {
          TBSYS_LOG(ERROR, "replay_worker.submit_batch(end_id=%ld, buf=%p[%ld])=>%d", end_id, buf, len, err);
        }
        else
        {
          //add by chujiajia [log synchronization][multi_cluster] 20160625:b
          replay_worker.set_next_flush_log_id(end_id + 1);
          //add:e
          start_location = end_location;
        }
      }
      if (stop)
      {
        err = OB_CANCELED;
      }
      if (OB_SUCCESS != err)
      {}
      else if (OB_SUCCESS != (err = replay_worker.wait_task(end_id)))
      {
        TBSYS_LOG(ERROR, "replay_worker.wait_task(end_id=%ld)=>%d", end_id, err);
      }
      else
      {
        end_cursor.log_id_ = end_location.log_id_;
        end_cursor.file_id_ = end_location.file_id_;
        end_cursor.offset_ = end_location.offset_;
      }
      if (NULL != buf)
      {
        ob_free(buf);
      }
      TBSYS_LOG(INFO, "replay_local_log_profile: duration=%ld", tbsys::CTimeUtil::getTime() - start_time);
      return err;
    }

    int replay_single_log_func(ObUpsMutator& mutator, CommonSchemaManagerWrapper& schema,
                               ObUpsTableMgr* table_mgr, LogCommand cmd, const char* log_data, int64_t data_len,
                               const int64_t commit_id, const ReplayType replay_type)
    {
      int err = OB_SUCCESS;
      int64_t pos = 0;
      int64_t file_id = 0;
      if (NULL == table_mgr || NULL == log_data || 0 >= data_len)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "replay_single_log_func(table_mgr=%p, cmd=%d, log_data=%p, data_len=%ld)=>%d", table_mgr, cmd, log_data, data_len, err);
      }
      else
      {
        switch(cmd)
        {
          case OB_LOG_UPS_MUTATOR:
            if (OB_SUCCESS != (err = mutator.deserialize(log_data, data_len, pos)))
            {
              TBSYS_LOG(ERROR, "UpsMutator deserialize error[ret=%d log_data=%p data_len=%ld]", err, log_data, data_len);
            }
            else if (OB_SUCCESS != (err = table_mgr->replay(mutator, replay_type))
                     && OB_MEM_OVERFLOW != err)
            {
              TBSYS_LOG(ERROR, "UpsTableMgr replay error[ret=%d]", err);
            }
            else if (OB_MEM_OVERFLOW == err)
            {
              TBSYS_LOG(WARN, "table_mgr->replay(log_cmd=%d, data=%p[%ld]):MEM_OVERFLOW", cmd, log_data, data_len);
            }
            break;
          case OB_UPS_SWITCH_SCHEMA:
            if (OB_SUCCESS != (err = schema.deserialize(log_data, data_len, pos)))
            {
              TBSYS_LOG(ERROR, "ObSchemaManagerWrapper deserialize error[ret=%d log_data=%p data_len=%ld]",
                        err, log_data, data_len);
            }
            else if (OB_SUCCESS != (err = table_mgr->set_schemas(schema)))
            {
              TBSYS_LOG(ERROR, "UpsTableMgr set_schemas error, ret=%d schema_version=%ld", err, schema.get_version());
            }
            else
            {
              TBSYS_LOG(INFO, "switch schema succ");
            }
            break;
          case OB_LOG_SWITCH_LOG:
            if (OB_SUCCESS != (err = serialization::decode_i64(log_data, data_len, pos, (int64_t*)&file_id)))
            {
              TBSYS_LOG(ERROR, "decode_i64 log_id error, err=%d", err);
            }
            else
            {
              pos = data_len;
              TBSYS_LOG(INFO, "replay log: SWITCH_LOG, file_id=%ld", file_id);
            }
            break;
          case OB_LOG_NOP:
            pos = data_len;
            break;
          default:
            err = OB_ERROR;
            break;
        }
      }
      if (pos != data_len)
      {
        err = OB_ERROR;
        TBSYS_LOG(ERROR, "pos[%ld] != data_len[%ld]", pos, data_len);
        //common::hex_dump(log_data, static_cast<int32_t>(data_len), false, TBSYS_LOG_LEVEL_WARN);
      }
      if (OB_SUCCESS != err && OB_MEM_OVERFLOW != err)
      {
        TBSYS_LOG(ERROR, "replay_log(cmd=%d, log_data=%p, data_len=%ld, commit_id=%ld)=>%d",
                  cmd, log_data, data_len, commit_id, err);
      }
      return err;
    }

    int replay_local_log_func(const volatile bool& stop, const char* log_dir,
                              const ObLogCursor& start_cursor, ObLogCursor& end_cursor, IObLogApplier* log_applier)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      char* log_data = NULL;
      int64_t data_len = 0;
      LogCommand cmd = OB_LOG_UNKNOWN;
      uint64_t seq;
      int64_t retry_wait_time_us = 100 * 1000;
      int64_t start_time = tbsys::CTimeUtil::getTime();
      end_cursor = start_cursor;

      if (NULL == log_dir || start_cursor.file_id_ <= 0 || start_cursor.log_id_ != 0 || start_cursor.offset_ != 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, 0, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (!stop && OB_SUCCESS == err)
      {
        //TBSYS_LOG(INFO, "log_cursor=%s", end_cursor.to_str());
        if (OB_SUCCESS != (err = log_reader.read_log(cmd, seq, log_data, data_len)) &&
            OB_FILE_NOT_EXIST != err && OB_READ_NOTHING != err && OB_LAST_LOG_RUINNED != err)
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
        }
        else if (OB_LAST_LOG_RUINNED == err)
        {
          TBSYS_LOG(WARN, "last_log[%s] broken!", end_cursor.to_str());
          err = OB_ITER_END;
        }
        else if (OB_SUCCESS != err)
        {
          err = OB_ITER_END; // replay all
        }
        else if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
        }
        else if (NULL != log_applier)
        {
          err = OB_NEED_RETRY;
          while(!stop && OB_NEED_RETRY == err)
          {
            if (OB_SUCCESS != (err = log_applier->apply_log(cmd, seq, log_data, data_len, RT_LOCAL))
                && OB_NEED_RETRY != err && OB_CANCELED != err)
            {
              TBSYS_LOG(ERROR, "replay_single_log()=>%d",  err);
            }
            else if (OB_NEED_RETRY == err)
            {
              usleep((useconds_t)retry_wait_time_us);
            }
          }
        }
      }
      if (stop)
      {
        err = OB_CANCELED;
      }
      if (OB_ITER_END == err)
      {
        err = OB_SUCCESS;
      }
      if (NULL == log_applier)
      {
        TBSYS_LOG(DEBUG, "get_local_log_cursor(log_dir=%s, end_cursor=%s)=>%d", log_dir, end_cursor.to_str(), err);
      }
      else
      {
        TBSYS_LOG(DEBUG, "replay_local_log(log_dir=%s, end_cursor=%s)=>%d", log_dir, end_cursor.to_str(), err);
      }
      TBSYS_LOG(INFO, "replay_local_log_profile: duration=%ld", tbsys::CTimeUtil::getTime() - start_time);
      return err;
    }

    //add chujiajia [log synchronization][multi_cluster] 20160524:b
    int get_tmp_log_data_checksum(const char* log_dir,
                                  uint64_t &seq,
                                  int64_t &tmp_data_checksum,
                                  const ObLogCursor& start_cursor,
                                  ObLogCursor& end_cursor)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      end_cursor = start_cursor;
      if (NULL == log_dir || start_cursor.file_id_ < 0 || start_cursor.log_id_ < 0 || start_cursor.offset_ < 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, start_cursor.log_id_-1, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (OB_SUCCESS == err && end_cursor.log_id_ <= static_cast<int64_t>(seq))
      {
        //TBSYS_LOG(INFO, "log_cursor=%s", end_cursor.to_str());
        if (OB_SUCCESS != (err = log_reader.read_log_for_data_checksum(seq, tmp_data_checksum)))
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
          break;
        }
        else if (static_cast<int64_t>(seq) == end_cursor.log_id_)
        {
          TBSYS_LOG(INFO, "tmp_data_checksum:%ld, seq:%ld", tmp_data_checksum, static_cast<int64_t>(seq));
          TBSYS_LOG(INFO, "compare_tmp_log, log_dir:%s, file_id:%ld, log_id:%ld", log_dir, end_cursor.file_id_, end_cursor.log_id_);
          break;
        }
        if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
          break;
        }
      }
      TBSYS_LOG(INFO, "compare_tmp_log start_cursor.log_id_:%ld, end_cursor.log_id_:%ld", start_cursor.log_id_, end_cursor.log_id_);
      TBSYS_LOG(INFO, "tmp_data_checksum:%ld, seq:%ld", tmp_data_checksum, static_cast<int64_t>(seq));
      err = OB_SUCCESS;
      return err;
    }
    //add:e

    int replay_log_in_buf_func(const char* log_data, int64_t data_len, IObLogApplier* log_applier)
    {
      int err = OB_SUCCESS;
      ObLogEntry log_entry;
      int64_t pos = 0;
      int64_t retry_wait_time_us = 100 * 1000;
      while (OB_SUCCESS == err && pos < data_len)
      {
        if (OB_SUCCESS != (err = log_entry.deserialize(log_data, data_len, pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.deserialize(log_data=%p, data_len=%ld, pos=%ld)=>%d", log_data, data_len, pos, err);
        }
        else if (OB_SUCCESS != (err = log_entry.check_data_integrity(log_data + pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.check_data_integrity()=>%d", err);
        }
        else
        {
          err = OB_NEED_RETRY;
          while(OB_NEED_RETRY == err)
          {
            if (OB_SUCCESS != (err = log_applier->apply_log((LogCommand)log_entry.cmd_,
                                                            log_entry.seq_, log_data + pos, log_entry.get_log_data_len(), RT_APPLY))
                && OB_NEED_RETRY != err && OB_CANCELED != err)
            {
              TBSYS_LOG(ERROR, "replay_log(cmd=%d, log_data=%p, data_len=%d)=>%d", log_entry.cmd_, log_data + pos, log_entry.get_log_data_len(), err);
            }
            else if (OB_NEED_RETRY == err)
            {
              usleep((useconds_t)retry_wait_time_us);
            }
          }
        }
        if (OB_SUCCESS != err)
        {}
        else
        {
          pos += log_entry.get_log_data_len();
        }
      }
      return err;
    }

    int serialize_log_entry(char* buf, const int64_t len, int64_t& pos, ObLogEntry& entry,
                            const char* log_data, const int64_t data_len)
    {
      int err = OB_SUCCESS;
      if (NULL == buf || 0 >= len || pos > len || NULL == log_data || 0 >= data_len)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "serialize_log_entry(buf=%p, len=%ld, pos=%ld, log_data=%p, data_len=%ld)=>%d",
                  buf, len, pos, log_data, data_len, err);
      }
      else if (pos + entry.get_serialize_size() + data_len > len)
      {
        err = OB_BUF_NOT_ENOUGH;
        TBSYS_LOG(DEBUG, "pos[%ld] + entry.serialize_size[%ld] + data_len[%ld] > len[%ld]",
                  pos, entry.get_serialize_size(), data_len, len);
      }
      else if (OB_SUCCESS != (err = entry.serialize(buf, len, pos)))
      {
        TBSYS_LOG(ERROR, "entry.serialize(buf=%p, pos=%ld, capacity=%ld)=>%d",
                  buf, len, pos, err);
      }
      else
      {
        memcpy(buf + pos, log_data, data_len);
        pos += data_len;
      }
      return err;
    }

    int generate_log(char* buf, const int64_t len, int64_t& pos, ObLogCursor& cursor, const LogCommand cmd,
                 const char* log_data, const int64_t data_len)
    {
      int err = OB_SUCCESS;
      ObLogEntry entry;
      if (NULL == buf || 0 >= len || pos > len || NULL == log_data || 0 >= data_len || !cursor.is_valid())
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "generate_log(buf=%p, len=%ld, pos=%ld, log_data=%p, data_len=%ld, cursor=%s)=>%d",
                  buf, len, pos, log_data, data_len, cursor.to_str(), err);
      }
      else if (OB_SUCCESS != (err = cursor.next_entry(entry, cmd, log_data, data_len)))
      {
        TBSYS_LOG(ERROR, "cursor[%s].next_entry()=>%d", cursor.to_str(), err);
      }
      else if (OB_SUCCESS != (err = serialize_log_entry(buf, len, pos, entry, log_data, data_len)))
      {
        TBSYS_LOG(DEBUG, "serialize_log_entry(buf=%p, len=%ld, entry[id=%ld], data_len=%ld)=>%d",
                  buf, len, entry.seq_, data_len, err);
      }
      else if (OB_SUCCESS != (err = cursor.advance(entry)))
      {
        TBSYS_LOG(ERROR, "cursor[id=%ld].advance(entry.id=%ld)=>%d", cursor.log_id_, entry.seq_, err);
      }
      return err;
    }

    int set_entry(ObLogEntry& entry, const int64_t seq, const LogCommand cmd, const char* log_data, const int64_t data_len)
    {
      int err = OB_SUCCESS;
      entry.set_log_seq(seq);
      entry.set_log_command(cmd);
      if (OB_SUCCESS != (err = entry.fill_header(log_data, data_len)))
      {
        TBSYS_LOG(ERROR, "entry.fill_header(log_data=%p, data_len=%ld)=>%d", log_data, data_len, err);
      }
      return err;
    }

    int serialize_log_entry(char* buf, const int64_t len, int64_t& pos, const LogCommand cmd, const int64_t seq, const char* log_data, const int64_t data_len)
    {
      int err = OB_SUCCESS;
      ObLogEntry entry;
      if (OB_SUCCESS != (err = set_entry(entry, seq, cmd, log_data, data_len)))
      {
        TBSYS_LOG(ERROR, "set_entry(seq=%ld, cmd=%d, log_data=%p, data_len=%ld)=>%d", seq, cmd, log_data, data_len, err);
      }
      else if (OB_SUCCESS != (err = serialize_log_entry(buf, len, pos, entry, log_data, data_len))
               && OB_BUF_NOT_ENOUGH != err)
      {
        TBSYS_LOG(ERROR, "serialize_log_entry(buf=%p, len=%ld, pos=%ld, log_data=%p, data_len=%ld)=>%d",
                  buf, len, pos, log_data, data_len, err);
      }
      return err;
    }

    int fill_log(char* buf, const int64_t len, int64_t& pos, ObLogCursor& cursor, const LogCommand cmd, const char* log_data, const int64_t data_len)
    {
      int err = OB_SUCCESS;
      ObLogEntry entry;
      if (NULL == buf || len < 0 || pos > len || NULL == log_data || data_len <= 0)
      {
        err = OB_INVALID_ARGUMENT;
      }
      else if (OB_SUCCESS != (err = cursor.this_entry(entry, cmd, log_data, data_len)))
      {
        TBSYS_LOG(ERROR, "cursor[%s].next_entry()=>%d", cursor.to_str(), err);
      }
      else if (OB_SUCCESS != (err = serialize_log_entry(buf, len, pos, entry, log_data, data_len)))
      {
        TBSYS_LOG(DEBUG, "serialize_log_entry(buf[remain=%ld], entry[id=%ld], data_len=%ld)=>%d",
                  len-pos, entry.seq_, data_len, err);
      }
      else if (OB_SUCCESS != (err = cursor.advance(entry)))
      {
        TBSYS_LOG(ERROR, "cursor[id=%ld].advance(entry.id=%ld)=>%d", cursor.log_id_, entry.seq_, err);
      }
      return err;
    }

    //modify chujiajia [log synchronization][multi_cluster] 20160530:b
    //int parse_log_buffer(const char* log_data, const int64_t len, int64_t& start_id, int64_t& end_id)
    int parse_log_buffer(const char* log_data, const int64_t len, int64_t& start_id, int64_t& end_id, int64_t& master_cmt_log_id)
    //modify:e
    {
      int err = OB_SUCCESS;
      int64_t end_pos = 0;
      //modify chujiajia [log synchronization][multi_cluster] 20160530:b
      //if (OB_SUCCESS != (err = trim_log_buffer(log_data, len, end_pos, start_id, end_id)))
      if (OB_SUCCESS != (err = trim_log_buffer(log_data, len, end_pos, start_id, end_id, master_cmt_log_id)))
      //modify:e
      {
        TBSYS_LOG(ERROR, "trim_log_buffer(log_data=%p[%ld])=>%d", log_data, len, err);
      }
      else if (end_pos != len)
      {
        err = OB_INVALID_LOG;
      }
      return err;
    }

    //modify chujiajia [log synchronization][multi_cluster] 20160530:b
    //int trim_log_buffer(const char* log_data, const int64_t len, int64_t& end_pos, int64_t& start_id, int64_t& end_id)
    int trim_log_buffer(const char* log_data, const int64_t len, int64_t& end_pos, int64_t& start_id, int64_t& end_id, int64_t& master_cmt_log_id)
    //modify:e
    {
      bool is_file_end = false;
      //modify chujiajia [log synchronization][multi_cluster] 20160530:b
      //return trim_log_buffer(log_data, len, end_pos, start_id, end_id, is_file_end);
      return trim_log_buffer(log_data, len, end_pos, start_id, end_id, master_cmt_log_id, is_file_end);
      //modify:e
    }

    //modify chujiajia [log synchronization][multi_cluster] 20160530:b
    //int trim_log_buffer(const char* log_data, const int64_t len, int64_t& end_pos, int64_t& start_id, int64_t& end_id, bool& is_file_end)
    int trim_log_buffer(const char* log_data, const int64_t len, int64_t& end_pos, int64_t& start_id, int64_t& end_id, int64_t& master_cmt_log_id, bool& is_file_end)
    //modify:e
    {
      int err = OB_SUCCESS;
      int64_t pos = 0;
      int64_t old_pos = 0;
      ObLogEntry log_entry;
      int64_t real_start_id = 0;
      int64_t real_end_id = 0;
      end_pos = 0;
      if (NULL == log_data || 0 > len)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "trim_log_buffer(buf=%p[%ld]): invalid argument", log_data, len);
      }
      while (OB_SUCCESS == err && pos + log_entry.get_serialize_size() < len)
      {
        old_pos = pos;
        if (OB_SUCCESS != (err = log_entry.deserialize(log_data, len, pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.deserialize(log_data=%p, len=%ld, pos=%ld)=>%d", log_data, len, pos, err);
        }
        else if (old_pos + log_entry.get_serialize_size() + log_entry.get_log_data_len() > len)
        {
          pos = old_pos;
          break;
        }
        else if (OB_SUCCESS != (err = log_entry.check_data_integrity(log_data + pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.check_data_integrity()=>%d", err);
        }
        //add chujiajia [log synchronization][multi_cluster] 20160530:b
        else if(log_entry.header_.max_cmt_id_ > master_cmt_log_id)
        {
          master_cmt_log_id = log_entry.header_.max_cmt_id_;
        }
        //add:e

        if (OB_SUCCESS != err)
        {}
        else if (real_end_id > 0 && real_end_id != (int64_t)log_entry.seq_)
        {
          err = OB_DISCONTINUOUS_LOG;
          TBSYS_LOG(WARN, "expected_id[%ld] != log_entry.seq[%ld]", real_end_id, log_entry.seq_);
        }
        else
        {
          pos = old_pos + log_entry.get_serialize_size() + log_entry.get_log_data_len();
          real_end_id = log_entry.seq_ + 1;
          if (0 >= real_start_id)
          {
            real_start_id = log_entry.seq_;
          }
          if (OB_LOG_SWITCH_LOG == log_entry.cmd_)
          {
            is_file_end = true;
            break;
          }
        }
      }

      if (OB_SUCCESS != err && OB_INVALID_ARGUMENT != err)
      {
        TBSYS_LOG(ERROR, "parse log buf error:");
        //hex_dump(log_data, static_cast<int32_t>(len), true, TBSYS_LOG_LEVEL_WARN);
      }
      else if (real_start_id > 0)
      {
        end_pos = pos;
        start_id = real_start_id;
        end_id = real_end_id;
      }
      return err;
    }

    int trim_log_buffer(const int64_t offset, const int64_t align_bits,
                        const char* log_data, const int64_t len, int64_t& end_pos,
                        int64_t& start_id, int64_t& end_id, bool& is_file_end)
    {
      int err = OB_SUCCESS;
      int64_t pos = 0;
      int64_t old_pos = 0;
      ObLogEntry log_entry;
      int64_t real_start_id = 0;
      int64_t real_end_id = 0;
      int64_t last_align_end_pos = 0;
      int64_t last_align_end_id = 0;
      end_pos = 0;
      if (NULL == log_data || 0 > len || align_bits > 12)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "trim_log_buffer(buf=%p[%ld], align_bits=%ld): invalid argument", log_data, len, align_bits);
      }
      if (!is_align(offset, align_bits))
      {
        TBSYS_LOG(WARN, "offset[%ld] is not aligned[bits=%ld]", offset, align_bits);
      }
      while (OB_SUCCESS == err && pos <= len)
      {
        if (is_align(offset + pos, align_bits))
        {
          last_align_end_id = real_end_id;
          last_align_end_pos = offset + pos;
        }
        old_pos = pos;
        if (ObLogGenerator::is_eof(log_data + pos, len - pos))
        {
          TBSYS_LOG(WARN, "read_eof: offset=%ld, pos=%ld, log_id=%ld", offset, pos, real_end_id);
          break;
        }
        else if (is_file_end)
        {
          break;
        }
        else if (pos + log_entry.get_serialize_size() > len || is_file_end)
        {
          break;
        }
        else if (OB_SUCCESS != (err = log_entry.deserialize(log_data, len, pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.deserialize(log_data=%p, len=%ld, pos=%ld)=>%d", log_data, len, pos, err);
        }
        else if (old_pos + log_entry.get_serialize_size() + log_entry.get_log_data_len() > len)
        {
          pos = old_pos;
          break;
        }
        else if (OB_SUCCESS != (err = log_entry.check_data_integrity(log_data + pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.check_data_integrity()=>%d", err);
        }
        else if (real_end_id > 0 && real_end_id != (int64_t)log_entry.seq_)
        {
          err = OB_DISCONTINUOUS_LOG;
          TBSYS_LOG(WARN, "expected_id[%ld] != log_entry.seq[%ld]", real_end_id, log_entry.seq_);
        }
        else
        {
          pos = old_pos + log_entry.get_serialize_size() + log_entry.get_log_data_len();
          real_end_id = log_entry.seq_ + 1;
          if (0 >= real_start_id)
          {
            real_start_id = log_entry.seq_;
          }
          if (OB_LOG_SWITCH_LOG == log_entry.cmd_)
          {
            is_file_end = true;
          }
        }
      }

      if (OB_SUCCESS != err && OB_INVALID_ARGUMENT != err)
      {
        TBSYS_LOG(ERROR, "parse log buf error:");
        //hex_dump(log_data, static_cast<int32_t>(len), true, TBSYS_LOG_LEVEL_WARN);
      }
      else if (last_align_end_id > 0)
      {
        // 只有当最后一条日志的位置恰好对齐时，才可能把SWITCH_LOG包含进来
        is_file_end = (last_align_end_pos == offset + pos)? is_file_end: false;
        end_pos = last_align_end_pos - offset;
        start_id = real_start_id;
        end_id = last_align_end_id;
      }
      else
      {
        is_file_end = false;
        end_pos = 0;
        end_id = start_id;
      }
      if (OB_SUCCESS == err && len > 0 && end_pos <= 0)
      {
        TBSYS_LOG(WARN, "trim_log_buffer(offset=%ld, align=%ld, len=%ld, end_pos=%ld): not found aligned pos",
                  offset, align_bits, len, end_pos);
      }
      return err;
    }

    //add lbzhong [Commit Point] 20150820:b
    int trim_log_buffer(const int64_t offset, const int64_t align_bits,
                                    const char* log_data, const int64_t len, int64_t& end_pos,
                                    int64_t& start_id, int64_t& end_id, bool& is_file_end, bool& has_committed_end, const int64_t commit_seq)
    {
      int err = OB_SUCCESS;
      int64_t pos = 0;
      int64_t old_pos = 0;
      ObLogEntry log_entry;
      int64_t real_start_id = 0;
      int64_t real_end_id = 0;
      int64_t last_align_end_pos = 0;
      int64_t last_align_end_id = 0;
      end_pos = 0;
      if (NULL == log_data || 0 > len || align_bits > 12)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "trim_log_buffer(buf=%p[%ld], align_bits=%ld): invalid argument", log_data, len, align_bits);
      }
      if (!is_align(offset, align_bits))
      {
        TBSYS_LOG(WARN, "offset[%ld] is not aligned[bits=%ld]", offset, align_bits);
      }
      while (OB_SUCCESS == err && pos <= len)
      {
        /*********************************************************/
        if(commit_seq >= 0 && real_end_id > commit_seq)
        {
          has_committed_end = true;
          break;
        }
        /*********************************************************/

        if (is_align(offset + pos, align_bits))
        {
          last_align_end_id = real_end_id;
          last_align_end_pos = offset + pos;
        }
        old_pos = pos;
        if (ObLogGenerator::is_eof(log_data + pos, len - pos))
        {
          TBSYS_LOG(WARN, "read_eof: offset=%ld, pos=%ld, log_id=%ld", offset, pos, real_end_id);
          break;
        }
        else if (is_file_end)
        {
          break;
        }
        else if (pos + log_entry.get_serialize_size() > len || is_file_end)
        {
          break;
        }
        else if (OB_SUCCESS != (err = log_entry.deserialize(log_data, len, pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.deserialize(log_data=%p, len=%ld, pos=%ld)=>%d", log_data, len, pos, err);
        }
        else if (old_pos + log_entry.get_serialize_size() + log_entry.get_log_data_len() > len)
        {
          pos = old_pos;
          break;
        }
        else if (OB_SUCCESS != (err = log_entry.check_data_integrity(log_data + pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.check_data_integrity()=>%d", err);
        }
        else if (real_end_id > 0 && real_end_id != (int64_t)log_entry.seq_)
        {
          err = OB_DISCONTINUOUS_LOG;
          TBSYS_LOG(WARN, "expected_id[%ld] != log_entry.seq[%ld]", real_end_id, log_entry.seq_);
        }
        else
        {
          pos = old_pos + log_entry.get_serialize_size() + log_entry.get_log_data_len();
          real_end_id = log_entry.seq_ + 1;
          if (0 >= real_start_id)
          {
            real_start_id = log_entry.seq_;
          }
          if (OB_LOG_SWITCH_LOG == log_entry.cmd_)
          {
            is_file_end = true;
          }
        }
      }

      if (OB_SUCCESS != err && OB_INVALID_ARGUMENT != err)
      {
        TBSYS_LOG(ERROR, "parse log buf error:");
        //hex_dump(log_data, static_cast<int32_t>(len), true, TBSYS_LOG_LEVEL_WARN);
      }
      else if (last_align_end_id > 0)
      {
        is_file_end = (last_align_end_pos == offset + pos)? is_file_end: false;
        end_pos = last_align_end_pos - offset;
        start_id = real_start_id;
        end_id = last_align_end_id;
      }
      else
      {
        is_file_end = false;
        end_pos = 0;
        end_id = start_id;
      }

      /*********************************************************/
      if(has_committed_end)
      {
        end_id = commit_seq;
      }
      /*********************************************************/

      if (OB_SUCCESS == err && len > 0 && end_pos <= 0)
      {
        TBSYS_LOG(WARN, "trim_log_buffer(offset=%ld, align=%ld, len=%ld, end_pos=%ld): not found aligned pos",
                  offset, align_bits, len, end_pos);
      }
      return err;
    }
    //add:e

    //add lbzhong [Max Log Timestamp] 20150820:b
    int get_log_timestamp(const LogCommand cmd, const char* buf, const int64_t len, int64_t& timestamp)
    {
      int ret = OB_SUCCESS;
      int64_t pos = 0;
      if(OB_LOG_UPS_MUTATOR == cmd)
      {
        ObUpsMutator mut;
        if(OB_SUCCESS != (ret = mut.deserialize(buf, len, pos)))
        {
          TBSYS_LOG(INFO, "Error occured when deserializing ObUpsMutator");
        }
        else
        {
          timestamp = mut.get_mutate_timestamp();
        }
      }
      else if(OB_LOG_NOP == cmd)
      {
        DebugLog debug_log;
        debug_log.deserialize(buf, len, pos);
        timestamp = debug_log.ctime_;
      }
      return ret;
    }


    int get_local_max_log_timestamp_func(const char* log_dir, const uint64_t log_file_id_by_sst, int64_t& max_timestamp)
    {
      int err = OB_SUCCESS;
      ObLogCursor new_cursor;
      int64_t max_log_file_id = 0;
      ObLogDirScanner scanner;
      /****************************************************/
      ObLogCursor log_cursor;
      /****************************************************/
      set_cursor(log_cursor, 0, 0, 0);
      if (NULL == log_dir)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "log_dir == NULL");
      }
      else if (OB_SUCCESS != (err = scanner.init(log_dir)) && OB_DISCONTINUOUS_LOG != err)
      {
        TBSYS_LOG(ERROR, "scanner.init(log_dir=%s)=>%d", log_dir, err);
      }
      else if (OB_SUCCESS != (err = scanner.get_max_log_id((uint64_t&)max_log_file_id)) && OB_ENTRY_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "get_max_log_file_id error[ret=%d]", err);
      }
      else
      {
        err = OB_SUCCESS;
        if (OB_INVALID_ID != log_file_id_by_sst)
        {
          max_log_file_id = max(max_log_file_id, log_file_id_by_sst);
        }
        TBSYS_LOG(INFO, "max_log_file_id=%ld, log_file_id_by_sst=%ld", max_log_file_id, log_file_id_by_sst);
        log_cursor.file_id_ = max(max_log_file_id, 0);
        if (log_cursor.file_id_ >0 && OB_SUCCESS != (err = get_local_max_log_timestamp_func(log_dir, log_cursor, new_cursor, max_timestamp)))
        {
          TBSYS_LOG(ERROR, "get_local_max_log_timestamp_func(file_id=%ld)=>%d", log_cursor.file_id_, err);
        }
        else
        {
          log_cursor = new_cursor;
        }
        /****************************************************/
        if (OB_SUCCESS == err && log_cursor.log_id_ <= 0 && log_cursor.file_id_ - 1 > 0 && max_timestamp <= 0)
        /****************************************************/
        {
          ObLogCursor tmp_cursor = log_cursor;
          tmp_cursor.file_id_--;
          if (OB_SUCCESS != (err = get_local_max_log_timestamp_func(log_dir, tmp_cursor, new_cursor, max_timestamp)))
          {
            TBSYS_LOG(ERROR, "get_local_max_log_timestamp_func(): try previous file[id=%ld]=>%d", log_cursor.file_id_, err);
          }
          else if (new_cursor.log_id_ > 0)
          {
            log_cursor = new_cursor;
          }
        }
      }
      return err;
    }


    int get_local_max_log_timestamp_func(const char* log_dir, const common::ObLogCursor& start_cursor, common::ObLogCursor& end_cursor, int64_t& timestamp)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      uint64_t seq;
      end_cursor = start_cursor;

      if (NULL == log_dir || start_cursor.file_id_ <= 0 || start_cursor.log_id_ != 0 || start_cursor.offset_ != 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, 0, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (OB_SUCCESS == err)
      {
        if (OB_SUCCESS != (err = log_reader.read_log(seq, timestamp)) &&
            OB_FILE_NOT_EXIST != err && OB_READ_NOTHING != err && OB_LAST_LOG_RUINNED != err)
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
        }
        if (OB_LAST_LOG_RUINNED == err)
        {
          TBSYS_LOG(WARN, "last_log[%s] broken!", end_cursor.to_str());
          err = OB_ITER_END;
        }
        else if (OB_SUCCESS != err)
        {
          err = OB_ITER_END; // replay all
        }
        else if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
        }
      }
      if (OB_ITER_END == err)
      {
        err = OB_SUCCESS;
      }
      return err;
    }

    int get_max_timestamp_from_log_buffer(const char* log_data, const int64_t data_len, const ObLogCursor& start_cursor, int64_t& max_timestamp)
    {
      int err = OB_SUCCESS;
      int64_t pos = 0;
      int64_t tmp_pos = 0;
      int64_t file_id = 0;
      ObLogCursor end_cursor;
      ObLogEntry log_entry;
      end_cursor = start_cursor;
      if (NULL == log_data || data_len <= 0 || !start_cursor.is_valid())
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument, log_data=%p, data_len=%ld, start_cursor=%s",
                  log_data, data_len, to_cstring(start_cursor));
      }

      while (OB_SUCCESS == err && pos < data_len)
      {
        if (OB_SUCCESS != (err = log_entry.deserialize(log_data, data_len, pos)))
        {
          TBSYS_LOG(ERROR, "log_entry.deserialize(log_data=%p, data_len=%ld, pos=%ld)=>%d", log_data, data_len, pos, err);
        }
        else if (pos + log_entry.get_log_data_len() > data_len)
        {
          err = OB_LAST_LOG_RUINNED;
          TBSYS_LOG(ERROR, "last_log broken, cursor=[%s,%s]", to_cstring(start_cursor), to_cstring(end_cursor));
        }
        else
        {
          tmp_pos = pos;
        }
        /****************************************************/
        max_timestamp = log_entry.header_.timestamp_;
        /****************************************************/
        if (OB_SUCCESS != err)
        {}
        else if (OB_LOG_SWITCH_LOG == log_entry.cmd_
                 && !(OB_SUCCESS == (err = serialization::decode_i64(log_data, data_len, tmp_pos, (int64_t*)&file_id)
                                     && start_cursor.log_id_ == file_id)))
        {
          TBSYS_LOG(ERROR, "decode switch_log failed(log_data=%p, data_len=%ld, pos=%ld)=>%d", log_data, data_len, tmp_pos, err);
        }
        else
        {
          pos += log_entry.get_log_data_len();
          if (OB_SUCCESS != (err = end_cursor.advance(log_entry)))
          {
            TBSYS_LOG(ERROR, "end_cursor[%ld].advance(%ld)=>%d", end_cursor.log_id_, log_entry.seq_, err);
          }
        }
      }
      if (OB_SUCCESS == err && pos != data_len)
      {
        err = OB_ERR_UNEXPECTED;
        TBSYS_LOG(ERROR, "pos[%ld] != data_len[%ld]", pos, data_len);
      }

      if (OB_SUCCESS != err)
      {
        hex_dump(log_data, static_cast<int32_t>(data_len), TBSYS_LOG_LEVEL_WARN);
      }
      return err;
    }
    //add:e
    //add chujiajia [log synchronization][multi_cluster] 20150419:b
    int get_local_max_cmt_id_func(const char* log_dir, const uint64_t log_file_id_by_sst, int64_t& cmt_id, ObLogCursor &tmp_end_cursor)
    {
      int err = OB_SUCCESS;
      ObLogCursor new_cursor;
      int64_t max_log_file_id = 0;
      ObLogDirScanner scanner;
      ObLogCursor log_cursor;
      set_cursor(log_cursor, 0, 0, 0);
      if (NULL == log_dir)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "log_dir == NULL");
      }
      else if (OB_SUCCESS != (err = scanner.init(log_dir)) && OB_DISCONTINUOUS_LOG != err)
      {
        TBSYS_LOG(ERROR, "scanner.init(log_dir=%s)=>%d", log_dir, err);
      }
      else if (OB_SUCCESS != (err = scanner.get_max_log_id((uint64_t&)max_log_file_id)) && OB_ENTRY_NOT_EXIST != err)
      {
        TBSYS_LOG(ERROR, "get_max_log_file_id error[ret=%d]", err);
      }
      else
      {
        err = OB_SUCCESS;
        if (OB_INVALID_ID != log_file_id_by_sst)
        {
          max_log_file_id = max(max_log_file_id, log_file_id_by_sst);
        }
        TBSYS_LOG(INFO, "max_log_file_id=%ld, log_file_id_by_sst=%ld", max_log_file_id, log_file_id_by_sst);
        log_cursor.file_id_ = max(max_log_file_id, 0);
        if (log_cursor.file_id_ >0 && OB_SUCCESS != (err = get_local_max_cmt_id_func(log_dir, log_cursor, new_cursor, cmt_id)))
        {
          TBSYS_LOG(ERROR, "get_local_max_cmt_id_func(file_id=%ld)=>%d", log_cursor.file_id_, err);
        }
        else
        {
          log_cursor = new_cursor;
        }
        if (OB_SUCCESS == err && log_cursor.log_id_ <= 0 && log_cursor.file_id_ - 1 > 0 && cmt_id <= 0)
        {
          ObLogCursor tmp_cursor = log_cursor;
          tmp_cursor.file_id_--;
          if (OB_SUCCESS != (err = get_local_max_cmt_id_func(log_dir, tmp_cursor, new_cursor, cmt_id)))
          {
            TBSYS_LOG(ERROR, "get_local_max_cmt_id_func(): try previous file[id=%ld]=>%d", log_cursor.file_id_, err);
          }
          else if (new_cursor.log_id_ > 0)
          {
            log_cursor = new_cursor;
          }
        }
      }
      tmp_end_cursor = log_cursor;
      return err;
    }

    int get_local_max_cmt_id_func(const char* log_dir, const common::ObLogCursor& start_cursor, common::ObLogCursor& end_cursor, int64_t& cmt_id)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      uint64_t seq;
      end_cursor = start_cursor;

      if (NULL == log_dir || start_cursor.file_id_ < 0 || start_cursor.log_id_ < 0 || start_cursor.offset_ < 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, 0, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (OB_SUCCESS == err)
      {
        if (OB_SUCCESS != (err = log_reader.read_log_for_cmt_id(seq, cmt_id)) &&
            OB_FILE_NOT_EXIST != err && OB_READ_NOTHING != err && OB_LAST_LOG_RUINNED != err)
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
        }
        if (OB_LAST_LOG_RUINNED == err)
        {
          TBSYS_LOG(WARN, "last_log[%s] broken!", end_cursor.to_str());
          err = OB_ITER_END;
        }
        else if (OB_SUCCESS != err)
        {
          err = OB_ITER_END; // replay all
        }
        else if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
        }
      }
      if (OB_ITER_END == err)
      {
        err = OB_SUCCESS;
      }
      return err;
    }

    int get_cursor_by_log_id(const char* log_dir, const int64_t log_id, const common::ObLogCursor& start_cursor, common::ObLogCursor& end_cursor)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      uint64_t seq = 1;
      end_cursor = start_cursor;
      int64_t cmt_id = 0;
      if (NULL == log_dir || start_cursor.file_id_ < 0 || start_cursor.log_id_ < 0 || start_cursor.offset_ < 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, 0, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (OB_SUCCESS == err)
      {
        if (OB_SUCCESS != (err = log_reader.read_log_for_cmt_id(seq, cmt_id)) &&
            OB_FILE_NOT_EXIST != err && OB_READ_NOTHING != err && OB_LAST_LOG_RUINNED != err)
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
        }
        if (OB_LAST_LOG_RUINNED == err)
        {
          TBSYS_LOG(WARN, "last_log[%s] broken!", end_cursor.to_str());
          err = OB_ITER_END;
        }
        else if (OB_SUCCESS != err)
        {
          err = OB_ITER_END; // replay all
        }
        else if(static_cast<int64_t>(seq) == log_id)
        {
           break;
        }
        else if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
        }
      }
      if (OB_ITER_END == err)
      {
        err = OB_SUCCESS;
      }
      return err;
    }

    int get_checksum_by_log_id(const char* log_dir, const int64_t log_id, const common::ObLogCursor& start_cursor, common::ObLogCursor& end_cursor, int64_t &checksum)
    {
      int err = OB_SUCCESS;
      ObLogReader log_reader;
      ObDirectLogReader direct_reader;
      uint64_t seq = 1;
      end_cursor = start_cursor;
      if (NULL == log_dir || start_cursor.file_id_ < 0 || start_cursor.log_id_ < 0 || start_cursor.offset_ < 0)
      {
        err = OB_INVALID_ARGUMENT;
        TBSYS_LOG(ERROR, "invalid argument: log_dir=%s, log_cursor=%s", log_dir, start_cursor.to_str());
      }
      else if (OB_SUCCESS != (err = log_reader.init(&direct_reader, log_dir, start_cursor.file_id_, 0, false)))
      {
        TBSYS_LOG(ERROR, "ObLogReader init error[err=%d]", err);
      }
      while (OB_SUCCESS == err)
      {
        if (OB_SUCCESS != (err = log_reader.read_log_for_data_checksum(seq, checksum)) &&
            OB_FILE_NOT_EXIST != err && OB_READ_NOTHING != err && OB_LAST_LOG_RUINNED != err)
        {
          TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", err);
        }
        if (OB_LAST_LOG_RUINNED == err)
        {
          TBSYS_LOG(WARN, "last_log[%s] broken!", end_cursor.to_str());
          err = OB_ITER_END;
        }
        else if (OB_SUCCESS != err)
        {
          err = OB_ITER_END; // replay all
        }
        else if(static_cast<int64_t>(seq) == log_id)
        {
           break;
        }
        else if (OB_SUCCESS != (err = log_reader.get_next_cursor(end_cursor)))
        {
          TBSYS_LOG(ERROR, "log_reader.get_cursor()=>%d",  err);
        }
      }
      if (OB_ITER_END == err)
      {
        err = OB_SUCCESS;
      }
      return err;
    }
    // add:e
  } // end namespace updateserver
} // end namespace oceanbase