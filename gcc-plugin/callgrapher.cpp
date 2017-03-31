#include <iostream>
#include <iterator>
#include <algorithm>
#include <set>
#include <memory>

#include <cstdio>

#include <gcc-plugin.h>
#include <gimple.h>
#include <function.h>
#include <dumpfile.h>

#include <sqlite3.h>

#include "callgrapher.h"
#include "function_decl.h"

Callgrapher::Callgrapher(std::string dbfile) :
	_dbFile{dbfile}
{}

void Callgrapher::findAllCalls()
{
	basic_block bb;
	FOR_ALL_BB(bb)
	{
		gimple_stmt_iterator gsi;
		for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
		{
			gimple stmt = gsi_stmt (gsi);

			if (gimple_code(stmt) == GIMPLE_CALL &&
					!is_gimple_builtin_call(stmt) &&
					!gimple_call_internal_p(stmt)) {

				tree fnCall = gimple_call_fn(stmt);
				tree fnDecl = gimple_call_addr_fndecl(fnCall);
				if (fnDecl != NULL) {
					_functions.emplace(FunctionDecl::giveFunctionDecl(fnDecl));
				}
			}
		}
	}

	const FunctionDecl& fn = FunctionDecl::giveFunctionDecl(cfun->decl);
	if (dump_file) {
		if (_functions.size() == 0) {
			fprintf(dump_file, ";; %s calls no function (other than builtins)\n", fn.str().c_str());
		} else {
			fprintf(dump_file, ";; %s calls the following functions:\n", fn.str().c_str());
			for (const FunctionDecl& fn : _functions)
				fprintf(dump_file, ";; \t%s\n", fn.str().c_str());
		}
	}

	if (!_dbFile.empty()) {
		try {
			dumpInDb(fn);
		} catch (std::runtime_error& e) {
			fprintf(stderr, "Couldn't store functions in database: %s", e.what());
		}
	}
}

void Callgrapher::dumpInDb(const FunctionDecl& fn)
{
	sqlite3* p_db = nullptr;
	if (sqlite3_open(_dbFile.c_str(), &p_db) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(p_db));
	}
	std::unique_ptr<sqlite3,std::function<void(sqlite3*)>> db{p_db,&sqlite3_close};

	std::string stmt = "SELECT Id FROM functions WHERE Global=1 AND Name=?1;";
	sqlite3_stmt* p_finder = nullptr;
	if (sqlite3_prepare_v2(db.get(), stmt.c_str(), stmt.length()+1, &p_finder, nullptr) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(p_db));
	}
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> finder{p_finder,&sqlite3_finalize};

	stmt = "SELECT Id FROM functions WHERE Global=0 AND Name=?1 AND File=?2;";
	sqlite3_stmt* p_staticFinder = nullptr;
	if (sqlite3_prepare_v2(db.get(), stmt.c_str(), stmt.length()+1, &p_staticFinder, nullptr) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(p_db));
	}
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> staticFinder{p_staticFinder,&sqlite3_finalize};

	if (getIndex(db.get(), fn, finder.get(), staticFinder.get()) != -1)
		return;

	stmt = "INSERT INTO functions (Name,File,Line,Global) VALUES (?1,?2,?3,?4);";
	sqlite3_stmt* p_fnInserter = nullptr;
	if (sqlite3_prepare_v2(db.get(), stmt.c_str(), stmt.length()+1, &p_fnInserter, nullptr) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(db.get()));
	}
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> fnInserter{p_fnInserter,&sqlite3_finalize};

	stmt = "INSERT INTO calls (Caller,Callee) VALUES (?1,?2);";
	sqlite3_stmt* p_fnCallInserter = nullptr;
	if (sqlite3_prepare_v2(db.get(), stmt.c_str(), stmt.length()+1, &p_fnCallInserter, nullptr) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(p_db));
	}
	std::unique_ptr<sqlite3_stmt,std::function<void(sqlite3_stmt*)>> fnCallInserter{p_fnCallInserter,&sqlite3_finalize};

	int id = insert(db.get(), fn, fnInserter.get());
	for (const FunctionDecl& otherFn : _functions) {
		int otherId = getIndex(db.get(), otherFn, finder.get(), staticFinder.get());
		if (otherId == -1)
			otherId = insert(db.get(), otherFn, fnInserter.get());
		insertCall(db.get(), id, otherId, fnCallInserter.get());
	}
}

int Callgrapher::insert(sqlite3* db, const FunctionDecl& fn, sqlite3_stmt* fnInserter)
{
	sqlite3_bind_text(fnInserter, 1, fn.getName().c_str(), fn.getName().length(), SQLITE_STATIC);
	sqlite3_bind_text(fnInserter, 2, fn.getFile().c_str(), fn.getFile().length(), SQLITE_STATIC);
	sqlite3_bind_int(fnInserter, 3, fn.getLine());
	sqlite3_bind_int(fnInserter, 4, fn.isGlobal());
	if (sqlite3_step(fnInserter) != SQLITE_DONE) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
	sqlite3_reset(fnInserter);
	return sqlite3_last_insert_rowid(db);
}

void Callgrapher::insertCall(sqlite3* db, int caller, int callee, sqlite3_stmt* fnCallInserter)
{
	sqlite3_bind_int(fnCallInserter, 1, caller);
	sqlite3_bind_int(fnCallInserter, 2, callee);
	if (sqlite3_step(fnCallInserter) != SQLITE_DONE) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
	sqlite3_reset(fnCallInserter);
}

int Callgrapher::getIndex(sqlite3* db, const FunctionDecl& fn, sqlite3_stmt* finder, sqlite3_stmt* staticFinder)
{
	sqlite3_stmt* stmt;
	if (fn.isGlobal()) {
		sqlite3_bind_text(finder, 1, fn.getName().c_str(), fn.getName().length(), SQLITE_STATIC);
		stmt = finder;
	} else {
		sqlite3_bind_text(staticFinder, 1, fn.getName().c_str(), fn.getName().length(), SQLITE_STATIC);
		sqlite3_bind_text(staticFinder, 2, fn.getFile().c_str(), fn.getFile().length(), SQLITE_STATIC);
		stmt = staticFinder;
	}

	int ret;
	switch (sqlite3_step(stmt)) {
		case SQLITE_ROW:
			ret = sqlite3_column_int(stmt, 0);
			break;
		case SQLITE_DONE:
			ret = -1;
			break;
		default:
			throw std::runtime_error(sqlite3_errmsg(db));
	}

	sqlite3_reset(stmt);
	return ret;
}
