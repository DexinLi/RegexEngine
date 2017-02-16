#pragma once
#include <vector>
#include <string>
#include<functional>
#include<map>
#include<set>
#include<unordered_map>
#include<unordered_set>
class RegexTree
{
protected:
	RegexTree() = default;
public:
	
	virtual ~RegexTree()=default;
	enum OpCode
	{
		RE_ID, RE_STAR, RE_OR, RE_CAT
	};
	OpCode Type;
private:
	
};


class Id :public RegexTree
{
public:
	Id();
	Id(char name);
	char name;
};

class Star :public RegexTree
{
public:
	Star();
	Star(RegexTree *operand);
	~Star() override;
	RegexTree *operand;
};


class Or :public RegexTree
{
public:
	Or();
	Or(RegexTree* left, RegexTree* right);
	~Or() override;

	RegexTree *left;
	RegexTree *right;
};


class Cat :public RegexTree
{
public:
	Cat();
	Cat(RegexTree* left, RegexTree* right);
	~Cat() override;
	RegexTree* left;
	RegexTree* right;
};

class DFA
{
public:
	explicit DFA(int begin,const std::vector<std::unordered_map<char, int>>& trans,const std::vector<char>& accept_table);
	explicit DFA(int begin, std::vector<std::unordered_map<char, int>>&& trans, const std::vector<char>& accept_table);
	explicit DFA(int begin, const std::vector<std::unordered_map<char, int>>& trans, std::vector<char>&& accept_table);
	explicit DFA(int begin, std::vector<std::unordered_map<char, int>>&& trans, std::vector<char>&& accept_table);
	void Minization();
	bool Check(const std::string&) const;
private:
	int begin;
	std::vector<std::unordered_map<char, int>> transitions;
	std::vector<char> accept_table;
};



class RegexToDFA
{
public:
	RegexToDFA(const std::string &regex_exp);
	~RegexToDFA();
	Cat * GetRegexTree();
	DFA GetDFA();
private:
	Cat* regex_tree;
	bool Nullable(RegexTree* Regex) const;
	std::unordered_map<RegexTree*, std::unordered_set<Id*>> first_ops;
	std::unordered_map<RegexTree*, std::unordered_set<Id*>> last_ops;
	std::unordered_map<Id*, std::unordered_set<Id*>> follow_ops;
	void FirstOps() ;
	void LastOps() ;
	void FollowOps() ;
};

