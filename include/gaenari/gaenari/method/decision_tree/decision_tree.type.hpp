#ifndef HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_TYPE_HPP
#define HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_TYPE_HPP

namespace gaenari {
namespace method {
namespace decision_tree {

// split strategy.
enum class split_strategy {
	split_strategy_unknown = 0,
	split_strategy_default = 1, // nominal(all distinct value splits, = rule), numeric(two splits, <= or > rule)
};

// tree node rule.
class rule_t {
public:
	rule_t()  = default;
	~rule_t() = default;
public:
	// rule description:
	//
	// ----------------+----------+---------------+----------------------------------------------
	// feature_indexes | type     | args          | desc
	// ----------------+----------+---------------+----------------------------------------------
	//             {3} | cmp_equ  | {(double)5.0} | if (3th feature value == 5.0)        then ...
	//             {3} | cmp_equ  | {(size_t)5  } | if (3th feature value == strings[5]) then ...
	// ----------------+----------+---------------+----------------------------------------------
	std::vector<size_t> feature_indexes;	// target related feature indexes. usally, size is one.
	enum class rule_type {
		unknown  = 0,
		cmp_equ  = 1, // ${feature_indexes[0]} == args[0]
		cmp_lte  = 2, // ${feature_indexes[0]} <= args[0]
		cmp_lt   = 3, // ${feature_indexes[0]} <  args[0]
		cmp_gt   = 4, // ${feature_indexes[0]} >  args[0]
		cmp_gte  = 5, // ${feature_indexes[0]} >= args[0]
	} type = rule_type::unknown;
	std::vector<type::value> args;		// arguments for rule. usally, size is one.
};

// the predicted instance number per classes in the tree node for the training data.
struct predicted_count_t {
	// (label_index -> count) map
	std::map<size_t,size_t> count;
};

struct leaf_info_t {
	size_t label_string_index = 0;
	size_t correct_count = 0;
	size_t incorrect_count = 0;
};

class tree_node {
public:
	tree_node()  = default;
	~tree_node() = default;	// do not auto clear.

public:
	// id.
	// remark) ids are initially all generated consecutively,
	//         but not usually contiguous after prune.
	int id = 0;

	// rule
	rule_t rule;

	// predicted instance count.
	predicted_count_t predicted_count;

	// link
	tree_node* parent = nullptr;

	// use `childs` instead of `children` to express the meaning of vector.
	std::vector<tree_node*> childs;

	// leaf node?
	// if true,  the actual classify information.
	// if false, classify information to the current tree node from root.
	bool leaf = false;
	leaf_info_t leaf_info;

public:
	void destroy_all_child_node(_option_in bool delete_me) {
		std::set<tree_node*> p;

		if (delete_me and this->parent) THROW_GAENARI_ERROR("destroy link from parent.");

		struct _inner {
			static void _traverse(_in tree_node* node, _out std::set<tree_node*>& p) {
				if (not node) return;
				for (auto& n: node->childs) {
					p.insert(n);
					_traverse(n, p);
				}
			}
		};

		// get all child node pointers.
		_inner::_traverse(this, p);
		
		// delete all child pointers.
		for (auto& ptr: p) {
			if (ptr) delete ptr;
		}

		// clear childs
		this->childs.clear();

		// delete me?
		if (delete_me) delete this;
	}

public:
	static inline tree_node* build_root_node(_in _out int& treenode_id) {
		tree_node* r = new tree_node;
		r->id = treenode_id;
		treenode_id++;
		return r;
	}

	static inline std::vector<tree_node*> build_child_nodes(_in tree_node& parent, _in size_t count, _in _out int& treenode_id) {
		std::vector<tree_node*> r(count);
		int count_int = static_cast<int>(count);
		int base_treenode_id = treenode_id;
		// the treenode id is not n, n+1, n+2, ..., but in reverse order as treenode_id+count-0-1, treebnode_id+count-1-1, treenode_id+count-2, ..., treenode_id.
		// after that, they are output in order.
		for (int i=0; i<count; i++) {
			auto c = new tree_node;
			c->id = base_treenode_id + count_int - i - 1;
			treenode_id++;
			c->parent = &parent;
			c->parent->childs.push_back(c);
			r[i] = c;
		}
		return r;
	}
};

// decision_tree do not use recursive function call.
// it uses manual stl stack.
// nodes pushed onto the stack for training.
class stack_train_node {
public:
	stack_train_node(_in tree_node& node, _in std::vector<size_t>& row_selections): node{node}, row_selections{row_selections} {}
	~stack_train_node() = default;
public:
	tree_node& node;					// just reference.
	std::vector<size_t> row_selections;	// not reference, vector copied.
};

// nodes pushed onto the stack for traversing.
template <typename T=tree_node>
class stack_traverse_node {
public:
	stack_traverse_node(_in T& node, _in int depth): node{node}, depth{depth} {}
	~stack_traverse_node() = default;

public:
	T& node;		// just reference.
	int depth = 0;	// depth.
};

// tree split info.
// choose one of maximum information gain ratio and,
// it's split info.
// split info has one rule and it's stat.
class split_info {
public:
	split_info() = default;
	split_info(const rule_t& rule, const predicted_count_t& predicted_count): rule{rule}, predicted_count{predicted_count} {}
public:
	rule_t rule;
	predicted_count_t predicted_count;
};

// one feature's information gain calculation result.
// after calculation, it passes split_info.
struct igr_result {
	bool stop_criteria = false;
	double igr_value = 0.0;
	std::vector<split_info> split_infos;
};

} // decision_tree
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_DECISION_TREE_DECISION_TREE_TYPE_HPP
