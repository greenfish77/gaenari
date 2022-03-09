#ifndef HEADER_GAENARI_SUPUL_SUPUL_SUPUL_IMPL_HPP
#define HEADER_GAENARI_SUPUL_SUPUL_SUPUL_IMPL_HPP

// footer included from supul.hpp
namespace supul {
namespace supul {

// it's static.
inline void supul_t::create_project(_in const std::string& base_dir) {
	// clear directory.
	if (std::filesystem::exists(base_dir)) {
		gaenari::logger::info("remove all files: {0}.", {base_dir});
		std::filesystem::remove_all(base_dir);
		std::filesystem::create_directories(base_dir);
	}

	// one object.
	supul_t supul;
	supul.init(base_dir);
}

inline void supul_t::init(_in const std::string& base_dir) {
	auto& version = gaenari::common::version();	// current version.
	bool create_mode = false;					// create or open.
	int ver = -1;
	int property_update = false;

	gaenari::logger::info("start to supul init(version: {0}).", {version});
	gaenari::logger::info("base_dir: {0}", {base_dir});

	// clear.
	deinit();

	// get paths.
	common::get_paths(base_dir, paths);

	// read property version.
	if (not prop.read(paths.property_txt, true)) create_mode = true;
	ver = prop.get("ver", -1);
	if (ver <= -1) create_mode = true;
	else if (ver < std::stoi(version)) property_update = true;

	// check valid.
	if (not common::check_valid_property(prop)) THROW_SUPUL_ERROR("invalid property file.");
	
	// comments.
	auto comment1 = "if the treenode is less accurate(<=) than this value, it is weak. "
					"the higher value, the more aggresive rebuild, and the more complex the tree.";
	auto comment2 = "it is weak when the number of treenode's instances is greater(>=) than this. "
					"the lower value, the more aggresive rebuild, and the more complex the tree.";

	// set default property with comment.
	if (create_mode or property_update) {
		prop.set_default({{"ver",										version.c_str(),		"supul configuration."}});
		prop.set_default({{"db.type",									"{choose one}",			"supported db type : sqlite."}});
		prop.set_default({{"db.dbname",									"supul",				"set default database name."}});
		prop.set_default({{"db.tablename.prefix",						"",						"set table name prefix."}});
		prop.set_default({{"model.weak_treenode_condition.accuracy",	"0.8",					comment1}});
		prop.set_default({{"model.weak_treenode_condition.total_count",	"5",					comment2}});
		if (not prop.save()) THROW_SUPUL_ERROR("fail to save property file.");
	}

	// save default attributes.json if file is not existed.
	common::create_empty_attributes_json_if_not_existed(paths.attributes_json);

	if (create_mode) {
		gaenari::logger::info("it is create_mode, exit.");
		return;
	}

	// read attributes.json.
	common::read_attributes(paths.attributes_json, attributes);

	// set db schema.
	schema.set(attributes, prop.get("db.tablename.prefix", ""));

	// create and initialize db.
	auto& db_type = prop.get("db.type");
	if (db_type == "sqlite") {
		db = new db::sqlite::sqlite_t{schema};
	} else {
		THROW_SUPUL_ERROR("invalid db_type: " + db_type);
	}
	db->init(prop, paths);
	db->lock_timeout(0);	// no block.
	gaenari::logger::info("sqlite initialized.");

	// get global row count.
	if (db->get_global_row_count() == 0) {
		on_first_initialized();
	}

	// initialize category.
	string_table.init();
	model.init();
}

inline void supul_t::deinit(void) {
	// deinitialize category.
	model.deinit();
	string_table.deinit();

	// db
	if (db) {
		db->deinit();
		delete db;
		db = nullptr;
	}
}

// called when the database is first created.
inline void supul_t::on_first_initialized(void) {
	// welcome log.
	gaenari::logger::info("welcome, gaenari.");

	// add global variables and initialize them.
	// since increment is not possible in a NULL value state, the initialize the values.
	db->add_global_one_row();
	db->set_global({{"schema_version",			schema.version()}, 
					{"instance_count",			static_cast<int64_t>(0)},
					{"updated_instance_count",	static_cast<int64_t>(0)},
					{"instance_correct_count",	static_cast<int64_t>(0)},
					{"instance_accuracy",		0.0},
					{"acc_weak_instance_count",	static_cast<int64_t>(0)}});
}

} // supul
} // supul

#endif // HEADER_GAENARI_SUPUL_SUPUL_SUPUL_IMPL_HPP
