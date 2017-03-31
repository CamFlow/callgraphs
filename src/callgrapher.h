#ifndef CALLGRAPHER_H
#define CALLGRAPHER_H

#include <map>
#include <string>
#include <set>
#include <functional>

#include <sqlite3.h>

#include "function_decl.h"

class Callgrapher
{
private:
	std::set<std::reference_wrapper<const FunctionDecl>> _functions;
	const std::string _dbFile;

	int insert(sqlite3* db, const FunctionDecl& fn, sqlite3_stmt* fnInserter);
	int getIndex(sqlite3* db, const FunctionDecl& fn, sqlite3_stmt* finder, sqlite3_stmt* staticFinder);
	void insertCall(sqlite3* db, int caller, int callee, sqlite3_stmt* fnCallInserter);


public:
	Callgrapher(std::string dbfile = "");
	void findAllCalls();
	void dumpInDb(const FunctionDecl& fn);
};
#endif /* CALLGRAPHER_H */
