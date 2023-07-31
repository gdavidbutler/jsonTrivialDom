-- jsonTrivialDom - XML in SQLite
-- Copyright (C) 2020-2023 G. David Butler <gdb@dbSystems.com>
--
-- This file is part of jsonTrivialDom
--
-- jsonTrivialDom is free software: you can redistribute it and/or modify
-- it under the terms of the GNU Lesser General Public License as published
-- by the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- jsonTrivialDom is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU Lesser General Public License for more details.
--
-- You should have received a copy of the GNU Lesser General Public License
-- along with this program.  If not, see <https://www.gnu.org/licenses/>.

-- JQL string
-- v value
CREATE TABLE IF NOT EXISTS "JqlS"(
 "i" INTEGER PRIMARY KEY
,"v" TEXT NOT NULL CHECK(TYPEOF("v")='text' AND LENGTH("v")>=0)
);
CREATE UNIQUE INDEX IF NOT EXISTS "JqlS_v" ON "JqlS"("v");
CREATE TRIGGER IF NOT EXISTS "JqlS_bd" BEFORE DELETE ON "JqlS" BEGIN
 SELECT RAISE(ABORT,'JqlS referenced') WHERE EXISTS(SELECT * FROM "JqlE" WHERE("t","v")=(0,OLD."i"));
END;
CREATE TRIGGER IF NOT EXISTS "JqlS_au_i" AFTER UPDATE OF "i" ON "JqlS" WHEN NEW."i"!=OLD."i" BEGIN
 UPDATE "JqlE" SET "v"=NEW."i" WHERE("t","v")=(0,OLD."i");
END;

-- JQL name
-- v value
CREATE TABLE IF NOT EXISTS "JqlN"(
 "i" INTEGER PRIMARY KEY
,"v" TEXT NOT NULL CHECK(TYPEOF("v")='text' AND LENGTH("v")>=0)
);
CREATE UNIQUE INDEX IF NOT EXISTS "JqlN_v" ON "JqlN"("v");
CREATE TRIGGER IF NOT EXISTS "JqlN_bd" BEFORE DELETE ON "JqlN" BEGIN
 SELECT RAISE(ABORT,'JqlN referenced') WHERE EXISTS(SELECT * FROM "JqlE" WHERE "n"=OLD."i");
END;
CREATE TRIGGER IF NOT EXISTS "JqlN_au_i" AFTER UPDATE OF "i" ON "JqlN" WHEN NEW."i"!=OLD."i" BEGIN
 UPDATE "JqlE" SET "n"=NEW."i" WHERE "n"=OLD."i";
END;

-- JQL element
-- e is 0 for root else REFERENCES JqlE(i) ON DELETE CASCADE ON UPDATE CASCADE
-- o ordinal
-- t type 0:string 1:number 2:boolean 3:null 4:array 5:object
-- v value REFERENCES t:0:JqlS(i) ON DELETE RESTRICT ON UPDATE CASCADE
-- n name REFERENCES "JqlN"("i") ON DELETE RESTRICT ON UPDATE CASCADE
CREATE TABLE IF NOT EXISTS "JqlE"(
 "i" INTEGER PRIMARY KEY
,"e" INTEGER NOT NULL CHECK(TYPEOF("e")='integer')
,"o" INTEGER NOT NULL CHECK(TYPEOF("o")='integer' AND "o">=0)
,"t" INTEGER NOT NULL CHECK(TYPEOF("t")='integer' AND "t" BETWEEN 0 AND 5)
,"v" NOT NULL CHECK(TYPEOF("v")='integer' OR (TYPEOF("v")='real' AND "t"=1))
,"n" INTEGER CHECK(TYPEOF("n")IN('integer','null'))
);
CREATE UNIQUE INDEX IF NOT EXISTS "JqlE_e_o" ON "JqlE"("e","o");
CREATE INDEX IF NOT EXISTS "JqlE_v_t_e" ON "JqlE"("v","t","e");
CREATE INDEX IF NOT EXISTS "JqlE_n_e" ON "JqlE"("n","e") WHERE "n" IS NOT NULL;
CREATE TRIGGER IF NOT EXISTS "JqlE_bi" BEFORE INSERT ON "JqlE" BEGIN
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.e, not array/object') WHERE NEW."e"!=0 AND NOT EXISTS(SELECT * FROM "JqlE" WHERE "i"=NEW.e AND "t">3);
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.v') WHERE NEW."t"=0 AND NOT EXISTS(SELECT * FROM "JqlS" WHERE "i"=NEW."v");
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.n') WHERE NEW."n" IS NOT NULL AND NOT EXISTS(SELECT * FROM "JqlN" WHERE "i"=NEW."n");
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_bu_e" BEFORE UPDATE OF "e" ON "JqlE" WHEN NEW."e"!=OLD."e" BEGIN
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.e, not array/object') WHERE NEW."e"!=0 AND NOT EXISTS(SELECT * FROM "JqlE" WHERE "i"=NEW.e AND "t">3);
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_bu_t" BEFORE UPDATE OF "t" ON "JqlE" WHEN NEW."t"!=OLD."t" BEGIN
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.t, has elements') WHERE NEW."t"<4 AND EXISTS(SELECT * FROM "JqlE" WHERE "e"=NEW."i");
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.v') WHERE NEW."t"=0 AND NOT EXISTS(SELECT * FROM "JqlS" WHERE "i"=NEW."v");
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_bu_v" BEFORE UPDATE OF "v" ON "JqlE" WHEN NEW."v"!=OLD."v" BEGIN
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.v') WHERE NEW."t"=0 AND NOT EXISTS(SELECT * FROM "JqlS" WHERE "i"=NEW."v");
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_bu_n" BEFORE UPDATE OF "n" ON "JqlE" WHEN NEW."n" IS NOT OLD."n" BEGIN
 SELECT RAISE(ABORT,'FOREIGN KEY constraint failed JqlE.n') WHERE NEW."n" IS NOT NULL AND NOT EXISTS(SELECT * FROM "JqlN" WHERE "i"=NEW."n");
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_au_i" AFTER UPDATE OF "i" ON "JqlE" WHEN NEW."i"!=OLD."i" BEGIN
 UPDATE "JqlE" SET "e"=NEW."i" WHERE "e"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_au_t" AFTER UPDATE OF "t" ON "JqlE" WHEN NEW."t"!=OLD."t" BEGIN
 DELETE FROM "JqlS" WHERE(0,"i")=(OLD."t",OLD."v") AND NOT EXISTS(SELECT * FROM "JqlE" WHERE("t","v")=(OLD."t",OLD."v"));
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_au_v" AFTER UPDATE OF "v" ON "JqlE" WHEN NEW."v"!=OLD."v" BEGIN
 DELETE FROM "JqlS" WHERE(0,"i")=(OLD."t",OLD."v") AND NOT EXISTS(SELECT * FROM "JqlE" WHERE("t","v")=(OLD."t",OLD."v"));
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_au_n" AFTER UPDATE OF "n" ON "JqlE" WHEN NEW."n" IS NOT OLD."n" BEGIN
 DELETE FROM "JqlN" WHERE "i"=OLD."n" AND NOT EXISTS(SELECT * FROM "JqlE" WHERE "n"=OLD."n");
END;
CREATE TRIGGER IF NOT EXISTS "JqlE_ad" AFTER DELETE ON "JqlE" BEGIN
 DELETE FROM "JqlE" WHERE "e"=OLD."i";
 DELETE FROM "JqlS" WHERE(0,"i")=(OLD."t",OLD."v") AND NOT EXISTS(SELECT * FROM "JqlE" WHERE("t","v")=(OLD."t",OLD."v"));
 DELETE FROM "JqlN" WHERE "i"=OLD."n" AND NOT EXISTS(SELECT * FROM "JqlE" WHERE "n"=OLD."n");
END;

-- JQL element view
CREATE VIEW IF NOT EXISTS "JqlEv"("i","e","o","t","v","n")AS SELECT "t0"."i","t0"."e","t0"."o","t0"."t",CASE WHEN "t0"."t"=0 THEN "t1"."v" ELSE "t0"."v" END,"t2"."v" FROM "JqlE" "t0" LEFT JOIN "JqlS" "t1" ON("t0"."t","t1"."i")=(0,"t0"."v") LEFT JOIN "JqlN" "t2" ON "t2"."i"="t0"."n";
CREATE TRIGGER IF NOT EXISTS "JqlEv_id" INSTEAD OF DELETE ON "JqlEv" BEGIN
 DELETE FROM "JqlE" WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_i_iu" INSTEAD OF UPDATE OF "i" ON "JqlEv" WHEN NEW."i"!=OLD."i" BEGIN
 UPDATE "JqlE" SET "i"=NEW."i" WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_e_iu" INSTEAD OF UPDATE OF "e" ON "JqlEv" WHEN NEW."e"!=OLD."e" BEGIN
 UPDATE "JqlE" SET "e"=NEW."e" WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_o_iu" INSTEAD OF UPDATE OF "o" ON "JqlEv" WHEN NEW."o"!=OLD."o" BEGIN
 UPDATE "JqlE" SET "o"=NEW."o" WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_t_iu" INSTEAD OF UPDATE OF "t" ON "JqlEv" WHEN NEW."t"!=OLD."t" BEGIN
 INSERT INTO "JqlS"("v")SELECT NEW."v" WHERE NEW."t"=0 AND NOT EXISTS(SELECT * FROM "JqlS" WHERE "v"=NEW."v");
 UPDATE "JqlE" SET("t","v")=(NEW."t",IFNULL((SELECT "i" FROM "JqlS" WHERE(0,"v")=(NEW."t",NEW."v")),NEW."v"))WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_v_iu" INSTEAD OF UPDATE OF "v" ON "JqlEv" WHEN NEW."v"!=OLD."v" BEGIN
 INSERT INTO "JqlS"("v")SELECT NEW."v" WHERE NEW."t"=0 AND NOT EXISTS(SELECT * FROM "JqlS" WHERE "v"=NEW."v");
 UPDATE "JqlE" SET "v"=IFNULL((SELECT "i" FROM "JqlS" WHERE(0,"v")=(NEW."t",NEW."v")),NEW."v")WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_n_iu" INSTEAD OF UPDATE OF "n" ON "JqlEv" WHEN NEW."n" IS NOT OLD."n" BEGIN
 INSERT INTO "JqlN"("v")SELECT NEW."n" WHERE NEW."n" IS NOT NULL AND NOT EXISTS(SELECT * FROM "JqlN" WHERE "v"=NEW."n");
 UPDATE "JqlE" SET "n"=(SELECT "i" FROM "JqlN" WHERE "v"=NEW."n")WHERE "i"=OLD."i";
END;
CREATE TRIGGER IF NOT EXISTS "JqlEv_ii" INSTEAD OF INSERT ON "JqlEv" BEGIN
 INSERT INTO "JqlS"("v")SELECT NEW."v" WHERE NEW."t"=0 AND NOT EXISTS(SELECT * FROM "JqlS" WHERE "v"=NEW."v");
 INSERT INTO "JqlN"("v")SELECT NEW."n" WHERE NEW."n" IS NOT NULL AND NOT EXISTS(SELECT * FROM "JqlN" WHERE "v"=NEW."n");
 INSERT INTO "JqlE"("i","e","o","t","v","n")VALUES(NEW."i",NEW."e",COALESCE(NEW."o",(SELECT MAX("o")+1 FROM "JqlE" WHERE "e"=NEW."e"),0),NEW."t",IFNULL((SELECT "i" FROM "JqlS" WHERE(0,"v")=(NEW."t",NEW."v")),NEW."v"),(SELECT "i" FROM "JqlN" WHERE "v"=NEW."n"));
END;
