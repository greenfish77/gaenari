#ifndef HEADER_GAENARI_SUPUL_DB_TRANSACTION_GUARD_HPP
#define HEADER_GAENARI_SUPUL_DB_TRANSACTION_GUARD_HPP

namespace supul {
namespace db {

// manages the life-time of a transaction.
// calls begin, commit and rollback.
// - begin is executed in constructor implictly.
// - commit, and rollback can be executed explicitly.
// - commit, and rollback can be executed implicitly (by destructor).
//		. by default, rollback is executed automatically.
//		. auto-commit is set in the constructor.
//		. note) this implicit call does not raise an exception,
//				so the success or failure of commit/rollback execution cannot be known.
//		. in the case of a simple read-operation, there is no problem.
//		  but if a write operation is involved, an explicit commit call is preferred.
// - set exclusive when a write operation is scheduled and locking is required in advance.
//
// ex)
// void func(void) {
//		transaction_guard transaction{get_db(), true};
//		... db operation ...
//		// automatic rollback. rollback error can not be catched.
// }
//
// void func(void) {
//		transaction_guard transaction{get_db(), true, true};
//		... db operation ...
//		// automatic commit. commit error can not be catched.
// }
//
// void func(void) {
//		transaction_guard transaction{get_db(), true, true};
//		... db operation ...
//		transaction.commit();	// explicit commit. commit error can be catched.
//		...
// }

class transaction_guard {
public:
	transaction_guard() = delete;
	inline transaction_guard(_in db::base& db, _in bool exclusive, _in bool auto_commit = false): db{db}, auto_commit{auto_commit} {
		db.begin(exclusive);	// can be throw-able.
		begun = true;
	};
	inline ~transaction_guard() noexcept {
		try {
			if (begun) {
				if (not auto_commit) {
					// implicit rollback.
					rollback();
				} else {
					// implicit commit.
					commit();
				}
				begun = false;
			}
		} catch(...) {
		}
	}
	inline void commit(void) {
		if (not begun) return;
		try {
			db.commit();
			begun = false;
		} catch(...) {
			// commit failed.
			// rollback and throw.
			db.rollback();
			begun = false;
			THROW_SUPUL_INTERNAL_ERROR0;
		}
	}
	inline void rollback(void) {
		if (not begun) return;
		try {
			db.rollback();
			begun = false;
		} catch(...) {
			// rollback failed.
			begun = false;
			THROW_SUPUL_INTERNAL_ERROR0;
		}
	}

protected:
	bool begun = false;
	bool auto_commit = false;
	db::base& db;
};

} // db
} // supul

#endif // HEADER_GAENARI_SUPUL_DB_TRANSACTION_GUARD_HPP
