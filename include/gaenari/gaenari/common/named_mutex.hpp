#ifndef HEADER_GAENARI_GAENARI_COMMON_NAMED_MUTEX_HPP
#define HEADER_GAENARI_GAENARI_COMMON_NAMED_MUTEX_HPP

namespace gaenari {
namespace common {

// supports named mutex between processes.
// on windows it works as intended.
// however, linux was not.
// some linux issues,
//
// 1) does not support nested same name locks in the same process.
//    ex)
//    named_mutex m0, m1;
//    m0.open("my_lock");
//    m1.open("my_lock");
//    m0.lock();
//    m1.lock(); // <-- blocks on linux !
//    m0.unlock();
//    m1.unlock();
//
//    it is necessary to perfect global shared count management,
//    but it's too expensive.
//    so, do not use nested same name locking.
//    is there an easier way to implement it?
//
// 2) /tmp/glocks/*.lck files remained.
//    once named_mutex opened, empty lock files(*.lck) are stored in /tmp/glocks/.
//    if the lck file is deleted during locked state, after the first lock() is not blocked.
//    so, protect the /tmp/glocks/*.lck files as much as possible from remove.
//    (this is possible by creating an immutable file using `chattr +i`,
//     however, root privileges are required.)
//    there is no problem with removing during unlocked state, so design the lock period as short as possible anyway.
//
// 3) not all combinations are tested.
//    multi-process, multi-thread, process crash, ...

struct named_mutex {
#ifdef _WIN32
	// windows version
	named_mutex()  = default;
	~named_mutex() {
		close();
	}
	void open(_in const std::string& name) { 
		if (not valid_name(name)) THROW_GAENARI_ERROR("invalid parameter: " + name);
		close();
		h = CreateMutexA(NULL, FALSE, name.c_str());
		if (not h) THROW_GAENARI_ERROR("fail to ::CreateMutexA, GetLastError: " + std::to_string(GetLastError()));
	}
	void close(void) { 
		if (not h) return;
		// remark)
		// if you close() without unlock(), the waiting process will be blocked until this process ends.
		// this conflicts with the way linux works. in linux, it triggers unlock when close.
		// for the same linux operation, call unlock before close.
		// that is, `open -> lock -> unlock -> close` and `open -> lock -> close` are the same.
		// if you coded explicitly unlock() and close(),
		// ReleaseMutex fails, and ERROR_NOT_OWNER is returned.
		ReleaseMutex(h);
		CloseHandle(h);
		h = NULL;
	}
	void lock(void) { 
		DWORD r = 0;		
		if (not h) THROW_GAENARI_INTERNAL_ERROR0;
		r = WaitForSingleObject(h, INFINITE);
		if ((r == WAIT_OBJECT_0) or (r == WAIT_ABANDONED)) return;
		THROW_GAENARI_ERROR("fail to ::WaitForSingleObject, return: " + std::to_string(r));
	}
	bool try_lock(void) {
		DWORD r = 0;
		if (not h) THROW_GAENARI_INTERNAL_ERROR0;
		r = WaitForSingleObject(h, 0);
		if ((r == WAIT_OBJECT_0) or (r == WAIT_ABANDONED)) return true;
		return false;
	}
	void unlock(void) {
		if (not h) THROW_GAENARI_INTERNAL_ERROR0;
		ReleaseMutex(h);
	}

protected:
	HANDLE h = NULL;
#else
	// linux version.
	named_mutex()  = default;
	~named_mutex() {close();}
	void open(_in const std::string& name) { 
		std::string path;
		if (not valid_name(name)) THROW_GAENARI_INTERNAL_ERROR0;
		close();
		path = "/tmp/glcks/" + name + ".lck";
		f = ::open(path.c_str(), O_CREAT, 0777);
		if (errno == ENOENT) {
			mkdir("/tmp/glcks", 0777);
			f = ::open(path.c_str(), O_CREAT, 0777);
		}
		if (f < 0) THROW_GAENARI_INTERNAL_ERROR0;
	}
	void close(void) { 
		// remark)
		// call ::close(f), unlock automatically.
		if (f < 0) return;
		::close(f);
		f = -1;
	}
	void lock(void) { 
		if (f < 0) THROW_GAENARI_INTERNAL_ERROR0;
		int rc = flock(f, LOCK_EX);
		if (rc < 0) {}
	}
	bool try_lock(void) {
		if (f < 0) THROW_GAENARI_INTERNAL_ERROR0;
		int rc = flock(f, LOCK_EX | LOCK_NB);
		if (rc < 0) {
			if (errno == EWOULDBLOCK) return false;
			return false;
		}
		return true;
	}
	void unlock(void) {
		if (f < 0) THROW_GAENARI_INTERNAL_ERROR0;
		int rc = flock(f, LOCK_UN);
		if (rc < 0) {}
	}
protected:
	int f = -1;
#endif
	// common
protected:
	// alpha, number, _, 
	bool valid_name(_in const std::string& name) {
		size_t i = 0;
		size_t len = name.length();
		const char* s = name.c_str();
		for (i=0; i<len; i++) {
			if (std::isalnum(s[i])) continue;
			if (s[i] == '_') continue;
			return false;
		}
		return true;
	}
};

// simple utility
// ex)
//     ... some codes ...
//     {
//         lock_guard_named_mutex lock("my_app");
//         ... safe ...
//     }
struct lock_guard_named_mutex {
	lock_guard_named_mutex(_in const std::string& name) {
		m.open(name);
		m.lock();
	} 

	~lock_guard_named_mutex() {
		m.unlock();
		m.close();
	}

protected:
	named_mutex m;
};

} // namespace common
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_NAMED_MUTEX_HPP
