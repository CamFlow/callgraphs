#ifndef FUNCTION_DECL_H
#define FUNCTION_DECL_H

#include <map>
#include <string>
#include <tree.h>

class FunctionDecl
{
private:
	static std::map<int, FunctionDecl> knownFunctions;
	std::string _name;
	std::string _file;
	int _line;
	bool _static;
	bool _locallyDefined;
	tree _fnDecl;
	explicit FunctionDecl(tree fnDecl);

public:
	static const FunctionDecl& giveFunctionDecl(tree fnDecl);
	friend std::ostream& operator<<(std::ostream& out, const FunctionDecl& fn);
	std::string str() const;
	inline bool isGlobal() const { return !_static; }
	inline bool isDefined() const { return !_locallyDefined; }
	inline const std::string& getFile() const { return _file; }
	inline const std::string& getName() const { return _name; }
	inline int getLine() const { return _line; }
	// needed for maps
	friend bool operator<(const FunctionDecl& one, const FunctionDecl& other);
};

std::ostream& operator<<(std::ostream& out, const FunctionDecl& fn);
bool operator<(const FunctionDecl& one, const FunctionDecl& other);
#endif /* FUNCTION_DECL_H */
