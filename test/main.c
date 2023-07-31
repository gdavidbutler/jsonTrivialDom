/*
 * jsonTrivialDom - XML in SQLite
 * Copyright (C) 2020-2023 G. David Butler <gdb@dbSystems.com>
 *
 * This file is part of jsonTrivialDom
 *
 * jsonTrivialDom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * jsonTrivialDom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "sqlite3.h"
#include "jql.h"

int
main(
 int argc
,char *argv[]
){
  static const char *dn = ":memory:";
  sqlite3 *db;
  sqlite3_stmt *st;
  unsigned char *bf;
  int rc;
  int i;

  sqlite3_initialize();
  if (argc == 2)
    dn = argv[1];
  if ((rc = sqlite3_open(dn, &db))) {
    fprintf(stderr, "sqlite3_open:%d:%s\n", rc, sqlite3_errstr(rc));
    return (rc);
  }
  if ((rc = jqlSchema(db))) {
    fprintf(stderr, "jqlSchema:%d:%s\n", rc, sqlite3_errmsg(db));
    return (rc);
  }
  if (!(bf = sqlite3_malloc(BUFSIZ))) {
    fprintf(stderr, "sqlite3_malloc:%d\n", BUFSIZ);
    return (-1);
  }
  i = 0;
  while ((rc = fread(bf + i, 1, BUFSIZ, stdin)) > 0) {
    i += rc;
    if (!(bf = sqlite3_realloc(bf, i + BUFSIZ))) {
      fprintf(stderr, "sqlite3_realloc:%d\n", i + BUFSIZ);
      return (-1);
    }
  }
  if ((rc = sqlite3_exec(db, "BEGIN;", 0,0,0))) {
    fprintf(stderr, "sqlite3_exec:%d:%s\n", rc, sqlite3_errmsg(db));
    return (rc);
  }
  if ((rc = json2jql(db, 0, bf, i, 64)) < 0) {
    fprintf(stderr, "json2jql:%d:%s\n", rc, sqlite3_errmsg(db));
    return (rc);
  } else if (rc != i) {
    fprintf(stderr, "json2jql:parse failed %d!=%d\n", rc, i);
    return (SQLITE_ERROR);
  }
  if ((rc = sqlite3_exec(db, "COMMIT;", 0,0,0))) {
    fprintf(stderr, "sqlite3_exec:%d:%s\n", rc, sqlite3_errmsg(db));
    return (rc);
  }
  sqlite3_free(bf);
  if ((rc = -sqlite3_prepare_v2(db
   ,"SELECT \"i\" FROM \"JqlE\" WHERE \"e\"=0 ORDER BY \"o\" ASC"
   ,-1, &st, 0))) {
    fprintf(stderr, "sqlite3_prepare_v2:%d:%s\n", rc, sqlite3_errmsg(db));
    return (rc);
  }
  while (sqlite3_step(st) == SQLITE_ROW) {
    if (!(bf = (unsigned char *)jql2json(db, sqlite3_column_int64(st, 0), (unsigned int *)&i))) {
      fprintf(stderr, "jql2json:%s\n", sqlite3_errmsg(db));
      return (-1);
    }
    fwrite(bf, 1, i, stdout);
    putchar('\n');
    sqlite3_free(bf);
  }
  sqlite3_finalize(st);
  sqlite3_close(db);
  return (0);
}
