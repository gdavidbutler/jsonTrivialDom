# jsonTrivialDom
Trivial JSON DOM in SQLite

### JSON

My [jsonTrivialCallbackParser](https://github.com/gdavidbutler/jsonTrivialCallbackParser) enables fast and easy JSON document parsing and processing.
The callback style works well as a driving flow, in other words, when all the support to process an JSON document can be invoked as parsed.
However, many times, an JSON document is needed to provide support for other flows.
In this case, the document must be searchable and/or modifiable.

### Database.

Using a SQL database for a DOM has many advantages.
The biggest one is the ability to use a standard query language, [SQL](https://en.wikipedia.org/wiki/SQL).

Using [SQLite](https://sqlite.org) for a DOM has many advantages.
The biggest one is it supports small, fast and embeddable databases.

### JQL

Find the API in jql.h:

* jqlSchema
  * Create an JQL schema on a connection, if it is missing
* json2jql
  * Parse an JSON document into an JQL schema rooted at a specified element
* jql2json
  * Generate an JSON document from an JQL schema at a specified element

### Example

* test/main.c: read an JSON document(s) on standard input, adding to an optional SQLite "database", then output JSON document(s) from the "database".

### Building

Edit Makefile providing locations for SQLite and my JSON parser.
Use "make" or review the file "Makefile" to build.
