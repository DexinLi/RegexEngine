#include "RegexEngine.h"


Id::Id()
{
	Type = RegexTree::OpCode::RE_ID;
}

Id::Id(char name):name(name)
{
	Type = RegexTree::OpCode::RE_ID;
}

Star::Star()
{
	Type = RegexTree::OpCode::RE_STAR;
}

Star::Star(RegexTree * operand):operand(operand)
{
	Type = RegexTree::OpCode::RE_STAR;
}

Star::~Star()
{
	delete operand;
}

Or::Or()
{
	Type = RegexTree::OpCode::RE_OR;
}

Or::Or(RegexTree * left, RegexTree * right):left(left),right(right)
{
	Type = RegexTree::OpCode::RE_OR;
}

Or::~Or()
{
	delete left;
	delete right;
}

Cat::Cat()
{
	Type = RegexTree::OpCode::RE_CAT;
}

Cat::Cat(RegexTree * left, RegexTree * right):left(left),right(right)
{
	Type = RegexTree::OpCode::RE_CAT;
}

Cat::~Cat()
{
	delete left;
	delete right;
}


Cat * RegexToDFA::GetRegexTree()
{
	return regex_tree;
}


DFA RegexToDFA::GetDFA()
{
	std::map<std::set<Id*>, std::unordered_map<char, std::set<Id*>>> dfa;
	std::unordered_set<Id*>& temp = first_ops[regex_tree];
	std::set<Id*> begin_state(temp.begin(), temp.end());
	std::set<std::set<Id*>> dfa_states;
	std::vector<std::set<Id*>> temp_states;
	temp_states.push_back(begin_state);
	std::unordered_map<char,std::set<Id*>> follow_of_state;
	while (!temp_states.empty())
	{
		std::set<Id*> t = temp_states.back();
		temp_states.pop_back();
		if (dfa_states.find(t) == dfa_states.end())
		{
			continue;
		}
		for (auto i = t.cbegin(), e = t.cend(); i != e; ++i)
		{
			auto x = *i;
			auto t = follow_ops.find(x);
			if (t != follow_ops.end())
			{
				follow_of_state[x->name].insert(t->second.begin(), t->second.end());
			}
		}
		dfa_states.insert(t);
		for (auto &i : follow_of_state)
		{
			temp_states.push_back(i.second);
		}
		dfa.insert({ t,follow_of_state });
	}
	std::map<std::set<Id*>, int> table;
	Id* end_state = static_cast<Id*>(regex_tree->right);
	std::vector<char> accept_states;
	int num = 0;
	for (auto& i : dfa)
	{
		table.insert({ i.first,num });
		if (i.first.count(end_state) != 0)
		{
			accept_states.push_back(1);
		}
		accept_states.push_back(0);
		num += 1;
	}
	std::vector<std::unordered_map<char, int>> edges;
	for (auto &i : dfa)
	{
		std::unordered_map<char, int> trans;
		for (auto &j : i.second)
		{
			trans.insert({ j.first,table[j.second] });
		}
		edges.push_back(trans);
	}
	return DFA(std::move(table[begin_state]),std::move(edges),std::move(accept_states));

}


bool RegexToDFA::Nullable(RegexTree * regex) const
{
	return regex->Type==RegexTree::RE_STAR;
}

void RegexToDFA::FirstOps()
{
	std::function<void(RegexTree* node)> firstOps = [this,&firstOps](RegexTree* node)->void 
	{
		switch (node->Type)
		{
		case RegexTree::RE_STAR:
			auto child = static_cast<Star*>(node)->operand;
			firstOps(child);
			auto& x = first_ops[child];
			for (auto i : x)
			{
				first_ops[node].insert(i);
			}
			break;
		case RegexTree::RE_ID:
			first_ops[node].insert(static_cast<Id*> (node));
			break;
		case RegexTree::RE_CAT:
			auto cat_ptr = static_cast<Cat*>(node);
			auto l = cat_ptr->left;
			auto r = cat_ptr->right;
			firstOps(l);
			firstOps(r);
			first_ops[node] = first_ops[l];
			if (Nullable(l))
			{
				auto& temp = first_ops[r];
				for (auto i : temp)
				{
					first_ops[node].insert(i);
				}
			}
			break;
		case RegexTree::RE_OR:
			auto cat_ptr = static_cast<Cat*>(node);
			auto l = cat_ptr->left;
			auto r = cat_ptr->right;
			firstOps(l);
			firstOps(r);
			first_ops[node] = first_ops[l];
			auto& temp = first_ops[r];
			for (auto i : temp)
			{
				first_ops[node].insert(i);
			}
			break;
		default:
			break;
		}
	
	};
	firstOps(regex_tree);
}

void RegexToDFA::LastOps()
{
	std::function<void(RegexTree* node)> lastOps = [this, &lastOps](RegexTree* node)->void
	{
		switch (node->Type)
		{
		case RegexTree::RE_STAR:
			auto child = static_cast<Star*>(node)->operand;
			lastOps(child);
			auto& x = last_ops[child];
			for (auto i : x)
			{
				last_ops[node].insert(i);
			}
			break;
		case RegexTree::RE_ID:
			last_ops[node].insert(static_cast<Id*> (node));
			break;
		case RegexTree::RE_CAT:
			auto cat_ptr = static_cast<Cat*>(node);
			auto l = cat_ptr->left;
			auto r = cat_ptr->right;
			lastOps(l);
			lastOps(r);
			last_ops[node] = last_ops[r];
			if (Nullable(l))
			{
				auto& temp = last_ops[l];
				for (auto i : temp)
				{
					last_ops[node].insert(i);
				}
			}
			break;
		case RegexTree::RE_OR:
			auto cat_ptr = static_cast<Cat*>(node);
			auto l = cat_ptr->left;
			auto r = cat_ptr->right;
			lastOps(l);
			lastOps(r);
			last_ops[node] = last_ops[l];
			auto& temp = last_ops[r];
			for (auto i : temp)
			{
				last_ops[node].insert(i);
			}
			break;
		default:
			break;
		}

	};
}

void RegexToDFA::FollowOps() 
{
	for (auto& i : first_ops)
	{
		if (i.first->Type == RegexTree::RE_CAT)
		{
			auto cat_ptr = static_cast<Cat*>(i.first);
			auto& last_of_left = last_ops[cat_ptr->left];
			auto& first_of_right = first_ops[cat_ptr->right];
			for (auto& j : last_of_left)
			{
				follow_ops[j].insert(first_of_right.begin(),first_of_right.end());
			}
		}
		else if (i.first->Type == RegexTree::RE_STAR)
		{
			auto star_ptr = static_cast<Star*>(i.first);
			auto &last_of_star = last_ops[star_ptr];
			auto &first_of_star = first_ops[star_ptr];
			for (auto &j : last_of_star)
			{
				follow_ops[j].insert(first_of_star.begin(), first_of_star.end());
			}
		}
	}
}

RegexToDFA::RegexToDFA(const std::string &regex_exp)
{
	std::vector<RegexTree*> operand_stack;
	std::vector<RegexTree*> rpn_queue;
	std::vector<char> operator_stack;
	bool need_cat = false;
	char curr;
	for (auto i = regex_exp.cbegin(), e = regex_exp.cend(); i != e; ++i)
	{
		curr = *i;
		switch (curr)
		{
		case '\\':
			++i; 
			if (need_cat)
			{
				rpn_queue.push_back(new Cat());
			}
			need_cat = !need_cat;
			rpn_queue.push_back(new Id(curr));
			break;
		case '*': operator_stack.push_back('*'); need_cat = false; break;
		case '|':
			while (!operator_stack.empty())
			{
				auto top = operator_stack.back();
				if (top == '.')
				{
					rpn_queue.push_back(new Cat());
				}
				else if (top == '*')
				{
					rpn_queue.push_back(new Star());
				}
				operator_stack.pop_back();
			}
			operator_stack.push_back('|'); need_cat = false; break;
		case '(':
			if (need_cat)
			{
				operator_stack.push_back('.');
				need_cat = false;
			}
			operator_stack.push_back('(');
			break;
		case ')':
			char c;
			while ((c=operator_stack.back()) != '(')
			{
				if (c == '.')
				{
					rpn_queue.push_back(new Cat());
				}
				else if (c == '*')
				{
					rpn_queue.push_back(new Star());
				}
				operator_stack.pop_back();
			}
			need_cat = true;
			break;
		default:
			if (need_cat)
			{
				rpn_queue.push_back(new Cat());
			}
			need_cat = !need_cat;
			rpn_queue.push_back(new Id(curr));
			break;
		}
	}
	while (!operator_stack.empty())
	{
		char c = operator_stack.back();
		if (c == '.')
		{
			rpn_queue.push_back(new Cat());
		}
		else if (c == '*')
		{
			rpn_queue.push_back(new Star());
		}
		operator_stack.pop_back();
	}
	for (auto i = rpn_queue.cbegin(), e = rpn_queue.cend(); i != e; ++i)
	{
		auto cur = *i;
		switch (cur->Type)
		{
		case RegexTree::RE_STAR:
			Star* star_ptr = static_cast<Star*>(cur);
			star_ptr->operand = operand_stack.back();
			operand_stack.back() = star_ptr; 
			break;
		case RegexTree::RE_OR:
			Or* or_ptr = static_cast<Or*>(cur);
			or_ptr->right = operand_stack.back();
			operand_stack.pop_back();
			or_ptr->left = operand_stack.back();
			operand_stack.back() = or_ptr;
			break;
		case RegexTree::RE_CAT:
			Cat* cat_ptr = static_cast<Cat*>(cur);
			cat_ptr->right = operand_stack.back();
			operand_stack.pop_back();
			cat_ptr->left = operand_stack.back();
			operand_stack.back() = cat_ptr;
			break;
		default:
			operand_stack.push_back(cur);
			break;
		}
	}
	regex_tree = new Cat(operand_stack.back(),new Id('\0'));
}

RegexToDFA::~RegexToDFA()
{
	delete regex_tree;
}

DFA::DFA(int begin, 
	const std::vector<std::unordered_map<char, int>>& trans, 
	const std::vector<char>& accept_table):begin(begin),transitions(trans),accept_table(accept_table)
{}

DFA::DFA(int begin, 
	std::vector<std::unordered_map<char, int>>&& trans, 
	const std::vector<char>& accept_table):
	begin(begin),
	transitions(std::forward<std::vector<std::unordered_map<char, int>>>(trans)),
	accept_table(accept_table)
{}

DFA::DFA(int begin,
	const std::vector<std::unordered_map<char, int>>& trans, 
	std::vector<char>&& accept_table)
	:begin(begin),
	transitions(trans),
	accept_table(std::forward<std::vector<char>>(accept_table))
{}

DFA::DFA(int begin, std::vector<std::unordered_map<char, int>>&& trans, std::vector<char>&& accept_table)
	:begin(begin),
	transitions(std::forward<std::vector<std::unordered_map<char, int>>>(trans)),
	accept_table(std::forward<std::vector<char>>(accept_table))
{}

void DFA::Minization()
{
	/*std::vector<std::unordered_set<int>> accept_states(1);
	std::vector<std::unordered_set<int>> unaccept_states(1);
	for (size_t i=0,e=accept_table.size();i!=e;++i)
	{
		if (accept_table[i] == 0)
		{
			unaccept_states.begin()->insert(i);
		}
		else
		{
			accept_states.begin()->insert(i);
		}
	}*/

}

bool DFA::Check(const std::string& input) const
{
	int trace = begin;
	for (auto i :input)
	{
		auto& vertex=transitions[trace];
		auto t = vertex.find(i);
		if (t == vertex.end())
		{
			return false;
		}
		trace = t->second;
	}
	if (accept_table[trace] != 0)
	{
		return true;
	}
	return false;
}
