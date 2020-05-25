SQLITE_INC=
SQLITE_LIB=-lsqlite3

JSON_INC=-I../jsonTrivialCallbackParser
JSON_OBJ=../jsonTrivialCallbackParser/json.o

CFLAGS = $(SQLITE_INC) $(JSON_INC) -I. -Os -g

all: jql.o main

clean:
	rm -f jql.str jql.o main

jql.str: str.sed jql.sql
	sed -f str.sed <jql.sql >jql.str

jql.o: jql.c jql.h jql.str
	cc $(CFLAGS) -c jql.c

main: test/main.c jql.o
	cc $(CFLAGS) -o main test/main.c jql.o $(JSON_OBJ) $(SQLITE_LIB)

check: main
	echo '["a",{"":"b","c":"d"},"1",["e","2",["f","3"],"4",["f","5"],"6",["g"],"7"],"8"]' | ./main
