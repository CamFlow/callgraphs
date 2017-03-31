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
			fprintf(stderr, "Couldn't store functions in database: %s\n", e.what());
		}
	}
}

sqlite3_stmt* Callgrapher::prepareStmt(const std::string& stmt)
{
	sqlite3_stmt* newStmt = nullptr;
	if (sqlite3_prepare_v2(_db.get(), stmt.c_str(), stmt.length()+1, &newStmt, nullptr) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(_db.get()));
	}

	return newStmt;
}

void Callgrapher::dumpInDb(const FunctionDecl& fn)
{
	sqlite3* db = nullptr;
	if (sqlite3_open(_dbFile.c_str(), &db) != SQLITE_OK) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
	_db.reset(db);

	_globalFnFinder.reset(prepareStmt("SELECT Id FROM functions WHERE Global=1 AND Name=?1;"));
	_staticFnFinder.reset(prepareStmt("SELECT Id FROM functions WHERE Global=0 AND Name=?1 AND File=?2;"));
	_fnInserter.reset(prepareStmt("INSERT INTO functions (Name,File,Line,Global) VALUES (?1,?2,?3,?4);"));
	_fnUpdater.reset(prepareStmt("UPDATE functions SET File=?1,Line=?2 WHERE Id=?3;"));
	_callInserter.reset(prepareStmt("INSERT INTO calls (Caller,Callee) VALUES (?1,?2);"));
	_callFinder.reset(prepareStmt("SELECT * FROM calls WHERE caller=?1;"));

	int id = getIndex(fn);
	bool registered;
	if (id == -1) {
		registered = false;
		id = insert(fn);
	} else {
		registered = alreadyRegistered(id);
		if (!registered)
			update(fn, id);
	}

	if (!registered) {
		for (const FunctionDecl& otherFn : _functions) {
			int otherId = getIndex(otherFn);
			if (otherId == -1)
				otherId = insert(otherFn);
			insertCall(id, otherId);
		}
	}
}

int Callgrapher::insert(const FunctionDecl& fn)
{
	sqlite3_stmt* fnInserter = _fnInserter.get();
	sqlite3* db = _db.get();

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

void Callgrapher::insertCall(int caller, int callee)
{
	sqlite3_stmt* callInserter = _callInserter.get();
	sqlite3* db = _db.get();

	sqlite3_bind_int(callInserter, 1, caller);
	sqlite3_bind_int(callInserter, 2, callee);
	if (sqlite3_step(callInserter) != SQLITE_DONE) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
	sqlite3_reset(callInserter);
}

void Callgrapher::update(const FunctionDecl& fn, int id)
{
	sqlite3_stmt* fnUpdater = _fnUpdater.get();
	sqlite3* db = _db.get();

	sqlite3_bind_text(fnUpdater, 1, fn.getFile().c_str(), fn.getFile().length(), SQLITE_STATIC);
	sqlite3_bind_int(fnUpdater, 2, fn.getLine());
	sqlite3_bind_int(fnUpdater, 3, id);
	if (sqlite3_step(fnUpdater) != SQLITE_DONE) {
		throw std::runtime_error(sqlite3_errmsg(db));
	}
	sqlite3_reset(fnUpdater);
}

int Callgrapher::getIndex(const FunctionDecl& fn)
{
	sqlite3_stmt* staticFnFinder = _staticFnFinder.get();
	sqlite3_stmt* globalFnFinder = _globalFnFinder.get();
	sqlite3* db = _db.get();

	sqlite3_stmt* stmt;
	if (fn.isGlobal()) {
		sqlite3_bind_text(globalFnFinder, 1, fn.getName().c_str(), fn.getName().length(), SQLITE_STATIC);
		stmt = globalFnFinder;
	} else {
		sqlite3_bind_text(staticFnFinder, 1, fn.getName().c_str(), fn.getName().length(), SQLITE_STATIC);
		sqlite3_bind_text(staticFnFinder, 2, fn.getFile().c_str(), fn.getFile().length(), SQLITE_STATIC);
		stmt = staticFnFinder;
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

bool Callgrapher::alreadyRegistered(int caller)
{
	sqlite3_stmt* callFinder = _callFinder.get();
	sqlite3* db = _db.get();

	sqlite3_bind_int(callFinder, 1, caller);

	bool ret;
	switch (sqlite3_step(callFinder)) {
		case SQLITE_ROW:
			ret = true;
			break;
		case SQLITE_DONE:
			ret = false;
			break;
		default:
			throw std::runtime_error(sqlite3_errmsg(db));
	}

	sqlite3_reset(callFinder);
	return ret;
}


