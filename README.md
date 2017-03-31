# Kayrebt::Callgraphs
GCC plugin that extracts the callgraph of a codebase, one unit translation at a time.

## Installation
On the dev branch, the bare minimum is available. Releases as .tar.gz files should be more user-friendly (but they're not done yet...).

What you need:
* The GNU autotools, with `libtool` and `m4`
* gcc and g++ version 4.8 exactly
* sqlite3 to get the callgraph as a SQLITE3 database (hard requirement for now, should be made optional in the future)

Proceed as such
* `autoreconf -i`: generates the `./configure` script
* `./configure`: prepares the installation and checks the dependencies
* `make`: compiles the plugin
* `make install`: installs the plugin. It tries to copy the plugin in GCC plugins directory, this will fail if the user has not sufficient privileges.

## Usage
In the codebase directory for which a callgraph must be generated, run GCC with some additional CFLAGS:

    gcc-4.8.5 -fopt-info-all-all -fdump-tree-all-all file.c
    
Remember to always use GCC version 4.8. The above command will produce an impressive mess in your build directory in the form of dump files.
The dump files have names like `file.c.X.YYYY` where `X` is the internal compilation pass identifier and `YYYY`the shortname of this pass.
The interesting file is the one ending in `.findcalls`. It will be similar to this:

    ;; Function f (f, funcdef_no=0, decl_uid=2176, cgraph_uid=0)

    ;; f (defined in subdir/1.h, l.3) (static) calls no function (other than builtins)
    f ()
    {
      <bb 2>:
      __builtin_puts (&"Hello world!"[0]);
      return;

    }

    ;; Function main (main, funcdef_no=1, decl_uid=2180, cgraph_uid=1)

    ;; main (defined in subdir/1.c, l.7) calls the following functions:
    ;; 	f (defined in subdir/1.h, l.3) (static)
    main ()
    {
      void (*<T15a>) () fn;
      int D.2184;

      <bb 2>:
      f ();
      fn = g;
      f ();
      fn ();
      D.2184 = 0;

    <L0>:
      return D.2184;

    }

What you see is a complete tree dump of the unit translation. The plugin's pass is placed before any optimization, so the dump will be very close to your code.
The plugin produces the comment above every function, giving the list of functions called inside of it.


This output has the advantage of not requiring any dependency but is not very practical. For this reason it is also possible to store the same information in a database.
To do so you need a SQLITE3 database with two tables `functions` and `calls` created as such:

    CREATE TABLE functions (Id INTEGER PRIMARY KEY, Name TEXT, File TEXT, Line INTEGER, Global INTEGER);
    CREATE TABLE calls (Caller INTEGER, Callee INTEGER, PRIMARY KEY (Caller,Callee));

Quite straightforwardly, `functions` stores the functions met during compilation and `calls` the 'calls' relationship between functions.

Compile your codebase as such:

    gcc-4.8.5 -fplugin=kayrebt_callgraphs -fplugin-arg-callgraphs-dbfile=myDB.sqlite
    
You should obtain something like:

    sqlite> SELECT * FROM functions;
    1|f|subdir/1.h|3|0
    2|main|subdir/1.c|7|1
    sqlite> SELECT * FROM calls;
    2|1
    
## Limitations

This plugins slows compilation down noticeably and storing graphs in relational databases is not really a clever idea.
Import your databases into a graph database like Neo4J will make processing the callgraph easier, especially if it is very big.

The plugin only tracks static function calls, and does not care about compiler builtins or calls through function pointers.
This could be added to a future version.
