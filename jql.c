/*
 * JQL - JSON in SQLite
 * Copyright (C) 2020 G. David Butler <gdb@dbSystems.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sqlite3.h"
#include "json.h"
#include "jql.h"

int
jqlSchema(
  sqlite3 *d
){
  int rc;

  if ((rc = sqlite3_exec(d
  ,"SAVEPOINT \"jqlSchema\";"
# include "jql.str"
   "RELEASE \"jqlSchema\";"
  ,0,0,0)))
    sqlite3_exec(d, "ROLLBACK TO \"jqlSchema\";", 0,0,0);
  return (rc);
}

int
jqlTruncate(
  sqlite3 *d
){
  int fk;
  int tr;
  int ch;
  int rc;

  fk = tr = 0;
  ch = 1;
  if ((rc = sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_FKEY, -1, &fk)))
    goto exit;
  if ((rc = sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_TRIGGER, -1, &tr)))
    goto exit;
  if (!ch && (rc = sqlite3_exec(d, "PRAGMA ignore_check_constraints=1;", 0,0,0)))
    goto exit;
  if (fk && (rc = sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_FKEY, 0, 0)))
    goto exit;
  if (tr && (rc = sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_TRIGGER, 0, 0)))
    goto exit;
  rc = sqlite3_exec(d
   ,"SAVEPOINT \"jqlTruncate\";"
    "DELETE FROM \"JqlS\";"
    "DELETE FROM \"JqlN\";"
    "DELETE FROM \"JqlE\";"
   , 0,0,0);
exit:
  sqlite3_exec(d, "RELEASE \"jqlTruncate\";", 0,0,0);
  if (tr)
    sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_TRIGGER, tr, 0);
  if (fk)
    sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_FKEY, fk, 0);
  if (!ch)
    sqlite3_exec(d, "PRAGMA ignore_check_constraints=0;", 0,0,0);
  return (rc);
}

static sqlite3_int64
si(
  sqlite3 *d
 ,sqlite3_stmt *s
 ,sqlite3_stmt *i
 ,const jsonSt_t *v
){
  sqlite3_int64 o1;

  sqlite3_bind_text(s, 1, (const char *)v->s, v->l, SQLITE_STATIC);
  if (sqlite3_step(s) == SQLITE_ROW)
    o1 = sqlite3_column_int64(s, 0);
  else
    o1 = 0;
  sqlite3_reset(s);
  if (!o1) {
    sqlite3_bind_text(i, 1, (const char *)v->s, v->l, SQLITE_STATIC);
    if (sqlite3_step(i) == SQLITE_DONE)
      o1 = sqlite3_last_insert_rowid(d);
    sqlite3_reset(i);
  }
  return (o1);
}

static sqlite3_int64
ds(
  sqlite3 *d
 ,sqlite3_stmt *s
 ,sqlite3_stmt *i
 ,const jsonSt_t *v
){
  unsigned char *s1;
  void *tv;
  sqlite3_int64 o1;
  jsonSt_t s2;
  int rc;

  if (v->l) {
    if ((s1 = sqlite3_malloc(v->l))) {
      if ((rc = jsonDecodeString(s1, v->l, v->s, v->l)) > (int)v->l) {
        if ((tv = sqlite3_realloc(s1, rc)))
          rc = jsonDecodeString((s1 = tv), rc, v->s, v->l);
        else
          rc = -1;
      }
    } else
      rc = -1;
    if (rc > 0) {
      s2.s = s1;
      s2.l = rc;
      o1 = si(d, s, i, &s2);
    } else
      o1 = si(d, s, i, v);
    sqlite3_free(s1);
  } else
    o1 = si(d, s, i, v);
  return (o1);
}

struct cx {
  sqlite3 *db;
  sqlite3_stmt *stSs;
  sqlite3_stmt *stSi;
  sqlite3_stmt *stNs;
  sqlite3_stmt *stNi;
  sqlite3_stmt *stEi;
  sqlite3_int64 *pth;
  unsigned int pthM;
  unsigned int pthN;
};

static int
cb(
  jsonTp_t t
 ,unsigned int l
 ,const jsonSt_t *g
 ,const jsonSt_t *v
 ,void *x
#define X ((struct cx *)x)
){
  void *tv;
  sqlite3_int64 o1;
  sqlite3_int64 o2;
  int rc;

  switch (t) {

  case jsonTp_Jb:
    if (l && (g + l - 1)->s) {
      if (!(o2 = ds(X->db, X->stNs, X->stNi, g + l - 1)))
        goto exit;
    } else
      o2 = 0;
    sqlite3_bind_int64(X->stEi, 1, *(X->pth + l));
    if (!v->l)
      sqlite3_bind_int(X->stEi, 2, 4);
    else
      sqlite3_bind_int(X->stEi, 2, 5);
    sqlite3_bind_int(X->stEi, 3, 0);
    if (o2)
      sqlite3_bind_int64(X->stEi, 4, o2);
    else
      sqlite3_bind_null(X->stEi, 4);
    rc = sqlite3_step(X->stEi);
    sqlite3_reset(X->stEi);
    if (rc != SQLITE_DONE)
      goto exit;
    if (X->pthM == X->pthN) {
      if (!(tv = sqlite3_realloc(X->pth, (X->pthM + 1) * sizeof (*X->pth))))
        goto exit;
      X->pth = tv;
      ++X->pthM;
    }
    *(X->pth + X->pthN) = sqlite3_last_insert_rowid(X->db);
    ++X->pthN;
    break;

  case jsonTp_Js:
    if (!(o1 = ds(X->db, X->stSs, X->stSi, v)))
      goto exit;
    if (l && (g + l - 1)->s) {
      if (!(o2 = ds(X->db, X->stNs, X->stNi, g + l - 1)))
        goto exit;
    } else
      o2 = 0;
    sqlite3_bind_int64(X->stEi, 1, *(X->pth + l));
    sqlite3_bind_int(X->stEi, 2, 0);
    sqlite3_bind_int64(X->stEi, 3, o1);
    if (o2)
      sqlite3_bind_int64(X->stEi, 4, o2);
    else
      sqlite3_bind_null(X->stEi, 4);
    rc = sqlite3_step(X->stEi);
    sqlite3_reset(X->stEi);
    if (rc != SQLITE_DONE)
      goto exit;
    break;

  case jsonTp_Jn:
    if (l && (g + l - 1)->s) {
      if (!(o2 = ds(X->db, X->stNs, X->stNi, g + l - 1)))
        goto exit;
    } else
      o2 = 0;
    sqlite3_bind_int64(X->stEi, 1, *(X->pth + l));
    sqlite3_bind_int(X->stEi, 2, 1);
    sqlite3_bind_text(X->stEi, 3, (const char *)v->s, v->l, SQLITE_STATIC);
    if (o2)
      sqlite3_bind_int64(X->stEi, 4, o2);
    else
      sqlite3_bind_null(X->stEi, 4);
    rc = sqlite3_step(X->stEi);
    sqlite3_reset(X->stEi);
    if (rc != SQLITE_DONE)
      goto exit;
    break;

  case jsonTp_Jt:
    if (l && (g + l - 1)->s) {
      if (!(o2 = ds(X->db, X->stNs, X->stNi, g + l - 1)))
        goto exit;
    } else
      o2 = 0;
    sqlite3_bind_int64(X->stEi, 1, *(X->pth + l));
    sqlite3_bind_int(X->stEi, 2, 2);
    sqlite3_bind_int(X->stEi, 3, 1);
    if (o2)
      sqlite3_bind_int64(X->stEi, 4, o2);
    else
      sqlite3_bind_null(X->stEi, 4);
    rc = sqlite3_step(X->stEi);
    sqlite3_reset(X->stEi);
    if (rc != SQLITE_DONE)
      goto exit;
    break;

  case jsonTp_Jf:
    if (l && (g + l - 1)->s) {
      if (!(o2 = ds(X->db, X->stNs, X->stNi, g + l - 1)))
        goto exit;
    } else
      o2 = 0;
    sqlite3_bind_int64(X->stEi, 1, *(X->pth + l));
    sqlite3_bind_int(X->stEi, 2, 2);
    sqlite3_bind_int(X->stEi, 3, 0);
    if (o2)
      sqlite3_bind_int64(X->stEi, 4, o2);
    else
      sqlite3_bind_null(X->stEi, 4);
    rc = sqlite3_step(X->stEi);
    sqlite3_reset(X->stEi);
    if (rc != SQLITE_DONE)
      goto exit;
    break;

  case jsonTp_Ju:
    if (l && (g + l - 1)->s) {
      if (!(o2 = ds(X->db, X->stNs, X->stNi, g + l - 1)))
        goto exit;
    } else
      o2 = 0;
    sqlite3_bind_int64(X->stEi, 1, *(X->pth + l));
    sqlite3_bind_int(X->stEi, 2, 3);
    sqlite3_bind_int(X->stEi, 3, 0);
    if (o2)
      sqlite3_bind_int64(X->stEi, 4, o2);
    else
      sqlite3_bind_null(X->stEi, 4);
    rc = sqlite3_step(X->stEi);
    sqlite3_reset(X->stEi);
    if (rc != SQLITE_DONE)
      goto exit;
    break;

  case jsonTp_Je:
    if (X->pthN)
      --X->pthN;
    break;

  }
  return (0);
exit:
  return (1);
}
#undef X

int
json2jql(
  sqlite3 *d
 ,sqlite3_int64 o
 ,const unsigned char *s
 ,unsigned int l
 ,unsigned int m
){
  jsonSt_t *tg;
  struct cx cx;
  int rc;
  int ch;
  int fk;
  int tr;

  rc = -SQLITE_ERROR;
  if (!d || !s)
    return (rc);
  if (!l)
    return (SQLITE_OK);
  if (m) {
    if (!(tg = sqlite3_malloc(m * sizeof (*tg))))
      return (rc);
  } else
    tg = 0;
  cx.db = d;
  cx.stSs = cx.stSi = cx.stNs = cx.stNi = cx.stEi = 0;
  ch = 1;
  fk = tr = 0;
  if (!(cx.pth = sqlite3_malloc(sizeof (*cx.pth))))
    goto exit;
  *cx.pth = o;
  cx.pthM = cx.pthN = 1;
  if ((rc = -sqlite3_prepare_v2(d
   ,"PRAGMA ignore_check_constraints"
   ,-1, &cx.stSs, 0)))
    goto exit;
  if ((rc = sqlite3_step(cx.stSs)) != SQLITE_ROW)
    goto exit;
  ch = sqlite3_column_int(cx.stSs, 0);
  sqlite3_finalize(cx.stSs);
  cx.stSs = 0;
  if ((rc = -sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_FKEY, -1, &fk)))
    goto exit;
  if ((rc = -sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_TRIGGER, -1, &tr)))
    goto exit;
  if (!ch && (rc = -sqlite3_exec(d, "PRAGMA ignore_check_constraints=1;", 0,0,0)))
    goto exit;
  if (fk && (rc = -sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_FKEY, 0, 0)))
    goto exit;
  if (tr && (rc = -sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_TRIGGER, 0, 0)))
    goto exit;
  if ((rc = -sqlite3_exec(d, "SAVEPOINT \"json2jql\";", 0,0,0)))
    goto exit;
  if ((rc = -sqlite3_prepare_v3(d
   ,"SELECT \"i\" FROM \"JqlS\" WHERE \"v\"=?1"
   ,-1, SQLITE_PREPARE_PERSISTENT, &cx.stSs, 0)))
    goto exit;
  if ((rc = -sqlite3_prepare_v3(d
   ,"INSERT INTO \"JqlS\"(\"v\")VALUES(?1)"
   ,-1, SQLITE_PREPARE_PERSISTENT, &cx.stSi, 0)))
    goto exit;
  if ((rc = -sqlite3_prepare_v3(d
   ,"SELECT \"i\" FROM \"JqlN\" WHERE \"v\"=?1"
   ,-1, SQLITE_PREPARE_PERSISTENT, &cx.stNs, 0)))
    goto exit;
  if ((rc = -sqlite3_prepare_v3(d
   ,"INSERT INTO \"JqlN\"(\"v\")VALUES(?1)"
   ,-1, SQLITE_PREPARE_PERSISTENT, &cx.stNi, 0)))
    goto exit;
  if ((rc = -sqlite3_prepare_v3(d
   ,"INSERT INTO \"JqlE\"(\"e\",\"o\",\"t\",\"v\",\"n\")VALUES(?1,IFNULL((SELECT MAX(\"o\")+1 FROM \"JqlE\" WHERE \"e\"=?1),0),?2,CASE WHEN ?2=1 THEN CAST(?3 AS NUMBER) ELSE ?3 END,?4)"
   ,-1, SQLITE_PREPARE_PERSISTENT, &cx.stEi, 0)))
    goto exit;
  rc = jsonParse(cb, m, tg, s, l, &cx);
exit:
  sqlite3_free(cx.pth);
  sqlite3_finalize(cx.stEi);
  sqlite3_finalize(cx.stNi);
  sqlite3_finalize(cx.stNs);
  sqlite3_finalize(cx.stSi);
  sqlite3_finalize(cx.stSs);
  sqlite3_exec(d, "RELEASE \"json2jql\";", 0,0,0);
  if (tr)
    sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_TRIGGER, tr, 0);
  if (fk)
    sqlite3_db_config(d, SQLITE_DBCONFIG_ENABLE_FKEY, fk, 0);
  if (!ch)
    sqlite3_exec(d, "PRAGMA ignore_check_constraints=0;", 0,0,0);
  sqlite3_free(tg);
  return (rc);
}

char *
jql2json(
  sqlite3 *d
 ,sqlite3_int64 o
 ,unsigned int *l
){
  char *json;
  const unsigned char *s1;
  unsigned char *s2;
  sqlite3_stmt *stE;
  sqlite3_stmt *stS;
  sqlite3_stmt *stN;
  sqlite3_str *rs;
  int *li;
  unsigned int liN;
  int n;
  int rc;

  stE = stS = stN = 0;
  li = 0;
  liN = 0;
  if (!(rs = sqlite3_str_new(0))
   || sqlite3_str_errcode(rs))
    goto exit;
  if ((rc = sqlite3_exec(d, "SAVEPOINT \"jql2json\";", 0,0,0)))
    goto exit;
  if ((rc = sqlite3_prepare_v3(d
  ,"WITH"
   " \"w1\"(\"l\",\"e\",\"o\",\"i\",\"t\",\"v\",\"n\")AS("
              "SELECT 0,\"e1\".\"e\",\"e1\".\"o\",\"e1\".\"i\",\"e1\".\"t\",\"e1\".\"v\",\"e1\".\"n\""
             " FROM \"JqlE\" \"e1\""
             " WHERE \"e1\".\"i\"=?1"
   " UNION ALL SELECT \"w1\".\"l\"+1,\"e1\".\"e\",\"e1\".\"o\",\"e1\".\"i\",\"e1\".\"t\",\"e1\".\"v\",\"e1\".\"n\""
             " FROM \"w1\""
                  ",\"JqlE\" \"e1\""
             " WHERE \"e1\".\"e\"=\"w1\".\"i\""
   " ORDER BY 1 DESC,2 ASC,3 ASC"
    ")"
   "SELECT \"l\",\"i\",\"t\",\"v\",\"n\" FROM \"w1\""
   ,-1, SQLITE_PREPARE_PERSISTENT, &stE, 0)))
    goto exit;
  if ((rc = sqlite3_prepare_v3(d
   ,"SELECT \"v\" FROM \"JqlS\" WHERE \"i\"=?1"
   ,-1, SQLITE_PREPARE_PERSISTENT, &stS, 0)))
    goto exit;
  if ((rc = sqlite3_prepare_v3(d
   ,"SELECT \"v\" FROM \"JqlN\" WHERE \"i\"=?1"
   ,-1, SQLITE_PREPARE_PERSISTENT, &stN, 0)))
    goto exit;
  sqlite3_bind_int64(stE, 1, o);
  n = 0;
  while (sqlite3_step(stE) == SQLITE_ROW) {
    void *tv;
    unsigned int i;

    i = sqlite3_column_int(stE, 0);
    if (i < liN)
      n = 1;
    while (i < liN) {
      --liN;
      if (!*(li + liN))
   /*[*/sqlite3_str_append(rs, "]", 1);
      else
   /*{*/sqlite3_str_append(rs, "}", 1);
    }
    if (n)
      sqlite3_str_append(rs, ",", 1);
    if (sqlite3_column_type(stE, 4) == SQLITE_INTEGER) {
      sqlite3_str_append(rs, "\"", 1);
      sqlite3_bind_int64(stN, 1, sqlite3_column_int64(stE, 4));
      if (sqlite3_step(stN) == SQLITE_ROW
       && (s1 = sqlite3_column_text(stN, 0))
       && (i = sqlite3_column_bytes(stN, 0))) {
        if ((s2 = sqlite3_malloc(i))) {
          if ((rc = jsonEncodeString(s2, i, s1, i)) > (int)i) {
            if ((tv = sqlite3_realloc(s2, rc)))
              rc = jsonEncodeString((s2 = tv), rc, s1, i);
            else
              rc = -1;
          }
        } else
          rc = -1;
        if (rc > 0)
          sqlite3_str_append(rs, (const char *)s2, rc);
        else
          sqlite3_str_append(rs, (const char *)s1, i);
        sqlite3_free(s2);
      }
      sqlite3_reset(stN);
      sqlite3_str_append(rs, "\":", 2);
    }
    switch (sqlite3_column_int(stE, 2)) {

    case 0: /* string */
      sqlite3_str_append(rs, "\"", 1);
      sqlite3_bind_int64(stS, 1, sqlite3_column_int64(stE, 3));
      if (sqlite3_step(stS) == SQLITE_ROW
       && (s1 = sqlite3_column_text(stS, 0))
       && (i = sqlite3_column_bytes(stS, 0))) {
        if ((s2 = sqlite3_malloc(i))) {
          if ((rc = jsonEncodeString(s2, i, s1, i)) > (int)i) {
            if ((tv = sqlite3_realloc(s2, rc)))
              rc = jsonEncodeString((s2 = tv), rc, s1, i);
            else
              rc = -1;
          }
        } else
          rc = -1;
        if (rc > 0)
          sqlite3_str_append(rs, (const char *)s2, rc);
        else
          sqlite3_str_append(rs, (const char *)s1, i);
        sqlite3_free(s2);
      }
      sqlite3_reset(stS);
      sqlite3_str_append(rs, "\"", 1);
      n = 1;
      break;

    case 1: /* number */
      if ((s1 = sqlite3_column_text(stE, 3)))
        sqlite3_str_append(rs, (const char *)s1, sqlite3_column_bytes(stE, 3));
      n = 1;
      break;

    case 2: /* boolean */
      if (!sqlite3_column_int64(stE, 3))
        sqlite3_str_append(rs, "false", 5);
      else
        sqlite3_str_append(rs, "true", 4);
      n = 1;
      break;

    case 3: /* null */
      sqlite3_str_append(rs, "null", 4);
      n = 1;
      break;

    case 4: /* array */
      if (!(tv = sqlite3_realloc(li, (liN + 1) * sizeof (*li))))
        break;
      li = tv;
      *(li + liN) = 0;
      ++liN;
      sqlite3_str_append(rs, "[", 1);/*]*/
      n = 0;
      break;

    case 5: /* object */
      if (!(tv = sqlite3_realloc(li, (liN + 1) * sizeof (*li))))
        break;
      li = tv;
      *(li + liN) = 1;
      ++liN;
      sqlite3_str_append(rs, "{", 1);/*}*/
      n = 0;
      break;

    default:
      goto exit;
    }
  }
  while (liN) {
    --liN;
    if (!*(li + liN))
 /*[*/sqlite3_str_append(rs, "]", 1);
    else
 /*{*/sqlite3_str_append(rs, "}", 1);
  }
exit:
  sqlite3_free(li);
  sqlite3_finalize(stN);
  sqlite3_finalize(stS);
  sqlite3_finalize(stE);
  sqlite3_exec(d, "RELEASE \"jql2json\";", 0,0,0);
  if (!rs)
    json = 0;
  else if (sqlite3_str_errcode(rs))
    json = sqlite3_str_finish(rs);
  else if ((rc = sqlite3_str_length(rs)) > 0) {
    if ((json = sqlite3_str_finish(rs)) && l)
      *l = rc;
  } else {
    (void)sqlite3_str_finish(rs);
    if ((json = sqlite3_malloc(1))) {
      *json = '\0';
      if (l)
        *l = 0;
    }
  }
  return (json);
}
