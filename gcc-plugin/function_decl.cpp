#include <iostream>
#include <sstream>
#include <stdexcept>
#include <memory>

#include <gcc-plugin.h>
#include <tree.h>
#include <function.h>

#include "function_decl.h"

std::map<int, FunctionDecl> FunctionDecl::knownFunctions;

FunctionDecl::FunctionDecl(tree fnDecl) :
	_fnDecl(fnDecl)
{
	_name.assign(fndecl_name(_fnDecl));
	_file.assign(DECL_SOURCE_FILE(_fnDecl));
	_line = DECL_SOURCE_LINE(_fnDecl);
	_static = !TREE_PUBLIC(_fnDecl);
}

const FunctionDecl& FunctionDecl::giveFunctionDecl(tree fnDecl)
{
	if (TREE_CODE(fnDecl) != FUNCTION_DECL)
		throw std::domain_error(std::string("Got ") + tree_code_name[TREE_CODE(fnDecl)] + std::string(" while we were expecting FUNCTION_DECL"));

	auto found = knownFunctions.find(DECL_PT_UID(fnDecl));
	if (found == knownFunctions.end())
		found = knownFunctions.emplace(DECL_PT_UID(fnDecl), FunctionDecl(fnDecl)).first;

	return found->second;
}

std::string FunctionDecl::str() const
{
	std::ostringstream out;
	out << _name << " (defined in " << _file << ", l." << _line << ")";
	if (_static)
		out << " (static)";
	return out.str();
}

std::ostream& operator<<(std::ostream& out, const FunctionDecl& fn)
{
	out << fn._name << " (defined in " << fn._file << ", l." << fn._line << ")";
	if (fn._static)
		out << " (static)";
	return out;
}

bool operator<(const FunctionDecl& one, const FunctionDecl& other)
{
	return one._fnDecl < other._fnDecl;
}
