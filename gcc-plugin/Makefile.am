lib_LTLIBRARIES=libkayrebt_callgraphs.la
libkayrebt_callgraphs_la_SOURCES=\
				    callgrapher.cpp\
				    callgrapher.h\
				    callgraphs.cpp\
				    function_decl.cpp\
				    function_decl.h

libkayrebt_callgraphs_la_CXXFLAGS = -std=c++11 -I @gcc_includes@ $(SQLITE3_CFLAGS)
libkayrebt_callgraphs_la_LDFLAGS  = -version-info 1:0:0
libkayrebt_callgraphs_la_LIBADD   = $(SQLITE3_LIBS)

install-exec-local:
	$(RM) `$(CC) -print-file-name=plugin`/kayrebt_callgraphs.so || true
	$(LN_S) @libdir@/libkayrebt_callgraphs.so `$(CC) -print-file-name=plugin`/kayrebt_callgraphs.so || echo "Couldn't install plugin in `$(CC) -print-file-name=plugin`"

