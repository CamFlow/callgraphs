#ifndef CALLGRAPHER_H
#define CALLGRAPHER_H

#include <map>
#include <string>
#include <set>
#include <functional>
#include <memory>

#include <sqlite3.h>

#include "function_decl.h"

class Callgrapher
{
private:
	std::set<std::reference_wrapper<const FunctionDecl>> _functions;
	const std::string _dbFile;
	std::unique_ptr<sqlite3,std::function<void(sqlite3*)>> _db{nullptr,&sqlite3_close};
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> _fnInserter{nullptr,&sqlite3_finalize};
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> _fnUpdater{nullptr,&sqlite3_finalize};
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> _callInserter{nullptr,&sqlite3_finalize};
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> _globalFnFinder{nullptr,&sqlite3_finalize};
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> _staticFnFinder{nullptr,&sqlite3_finalize};
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> _callFinder{nullptr,&sqlite3_finalize};

	sqlite3_stmt* prepareStmt(const std::string& stmt);
	int insert(const FunctionDecl& fn);
	void update(const FunctionDecl& fn, int id);
	int getIndex(const FunctionDecl& fn);
	void insertCall(int caller, int callee);
	bool alreadyRegistered(int caller);

public:
	Callgrapher(std::string dbfile = "");
	void findAllCalls();
	void dumpInDb(const FunctionDecl& fn);
};
#endif /* CALLGRAPHER_H */
