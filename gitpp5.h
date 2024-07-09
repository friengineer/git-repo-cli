#ifndef GITPP_H
#define GITPP_H

/* changelog
 *
 * gittp0
 * - initial release
 *
 * gitpp1
 * - add access to commit message
 *
 * gitpp2
 * - fix a branch iterator bug
 *
 * gitpp3
 * - allow iterating commits in empty repository
 *     (this breaks the convention in other git tools...)
 *
 * gitpp4
 * - COMMITS::create()
 * - REPO constructor take path argument
 *
 * gitpp5
 * - remove bogus trace.h include.
 */

#include <git2/repository.h>
#include <git2/annotated_commit.h>
#include <git2/errors.h>
#include <git2/global.h>
#include <git2/revwalk.h>
#include <git2/revparse.h>
#include <git2/object.h>
#include <git2/commit.h>
#include <git2/branch.h>
#include <git2/config.h>
#include <git2/refs.h>
#include <git2/checkout.h>
#include <git2/signature.h>
#include <git2/index.h>

#include <assert.h>
#include <string> // std::to_string
#include <vector> // std::to_string

#define untested()
#define incomplete() ( \
    std::cerr << "@@#\n@@@\nincomplete:" \
              << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n" )
#define unreachable() ( \
    std::cerr << "@@#\n@@@\nunreachable:" \
              << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n" )

/* -------------------------------------------------------------------------- */

// libgit2 wrapper
//
// iterators are one pass libgit2 does not allow normal iteration

namespace GITPP {

class EXCEPTION : public std::exception {
public:
	explicit EXCEPTION(std::string const& s) : _what(s) { }
	~EXCEPTION() noexcept {}
public:
	virtual const char* what() const noexcept{
		return _what.c_str();
	}
private:
	std::string _what;
};
class EXCEPTION_CANT_FIND : public EXCEPTION {
public:
	explicit EXCEPTION_CANT_FIND(std::string const& s) :
	  EXCEPTION("cant find "+ s) {}
};
class EXCEPTION_INVALID : public EXCEPTION {
public:
	explicit EXCEPTION_INVALID(std::string const& s) :
	  EXCEPTION("invalid "+ s) {}
};

class SIGNATURE{
public:
	SIGNATURE(git_commit const* c){
		assert(c);
		const git_signature *sig;
		if (!(sig = git_commit_author(c))) {
			throw EXCEPTION("something wrong in sig");
		}else{
			_s = std::make_pair(std::string(sig->name), std::string(sig->email));
		}
	}
public:
	std::string const& name() const{ return _s.first; }
	std::string const& email() const{ return _s.second; }
private:
	std::pair<std::string, std::string> _s;
};

class COMMIT{
public:
	explicit COMMIT(git_oid i, git_repository* r): _id(i) {
		if( git_commit_lookup(&_c, r, &_id) ){
			throw "lookup error\n";
		}else{
		}
	}

public:
	bool operator==(COMMIT const& x) const{ untested();
		return git_oid_equal(&_id, &x._id);
	}
	bool operator!=(COMMIT const&x){ untested();
		return !(*this==x);
	}
	std::string id() const{
		char buf[GIT_OID_HEXSZ+1];
		git_oid_fmt(buf, &_id);
		buf[GIT_OID_HEXSZ] = '\0';
		return std::string(buf);
	}
	std::ostream& print(std::ostream& o) const{
		return o << id();
	}
	std::string author() const{
		return signature().name();
	}
	std::string message() {
		return git_commit_message(_c);
   }
	std::string time(unsigned len=99) const {
		git_time_t seconds=git_commit_time(_c);

		char *a = ctime(&seconds);
		std::string ret(a);
      ret[ret.size()-1]='\0'; // chop off newline
		return ret;
	}
	SIGNATURE signature() const{
		return SIGNATURE(_c);
	}
	std::string show() const{
		incomplete();
		return "the diff (incomplete)";
	}
	std::string commit_message() const{
		incomplete();
		return "incomplete";
	}

private:
	const git_oid _id;
	git_commit* _c;
}; // COMMIT

std::ostream& operator<< (std::ostream& o, COMMIT const& c)
{
	return c.print(o);
}

class REPO;

class CONFIG {
public: // types	
	class ITEM {
	public:
		explicit ITEM(git_config_entry* e, git_config_iterator* p, CONFIG& c)
		    : _cfg(c), _entry(e) // , _r(c._repo)
		{
		//	if( git_config_next(&_entry, p)){
		//		throw EXCEPTION("can't get");
		//	}else{
		//	}
		}
		explicit ITEM(CONFIG& p, std::string const& what)
		    : _cfg(p) {
			if(/*int f=*/git_config_get_entry(&_entry, p._cfg, what.c_str())){
				throw EXCEPTION_CANT_FIND(what);
			}else{
			}
		}
		ITEM& operator=(const std::string& v){
			if(!_cfg._cfg){ untested();
			}else if(int error=git_config_set_string(_cfg._cfg, name().c_str(), v.c_str())){
				throw EXCEPTION("can't set " + std::to_string(error));
			}else{
				// ok
			}
			return *this;
		}

// 		operator std::string() const{
// 			return std::string(_value);
// 		}
		std::string value() const{
			assert(_entry);
			return _entry->value;
		}
		std::string name() const{
			assert(_entry);
			return _entry->name;
		}
		std::ostream& print(std::ostream& o) const{
			return o << name() << " = " << value();
		}

	private:
		CONFIG& _cfg;
		git_config_entry* _entry;
	}; // ITEM

	class ITER {
	public:
		ITER(ITER const& i)
		    : _cfg(i._cfg), _i(i._i), _e(i._e)
		{untested();
		}

	public:
		explicit ITER(CONFIG& c): _cfg(c){
			if( git_config_iterator_new(&_i, c._cfg)){
				throw EXCEPTION("iter error");
			}else{
			}

			operator++();
		}
		explicit ITER(CONFIG& c, int) : _cfg(c), _i(nullptr){
		} // "end"

		~ITER(){
			git_config_iterator_free(_i);
		}

		ITER& operator++(){
			assert(_i); // not end.
			// need to keep a pointer to entry, as we can only fetch it once (?!)
			int status=git_config_next(&_e, _i);
			if (status==GIT_ITEROVER) {
				_i = nullptr;
				_e = nullptr;
				assert(*this==ITER(_cfg, 0));
			}else if (status) {
				// unreachable(); // ?!
				_i = nullptr;
				_e = nullptr;
				assert(*this==ITER(_cfg, 0));
			}else{
			}
			return *this;
		}
		bool operator==(ITER const&x) const{
			return _i==x._i;
		}
		bool operator!=(ITER const&x){
			return !(*this==x);
		}
		ITEM operator*();

	private:
		CONFIG& _cfg;
		git_config_iterator* _i;
		git_config_entry* _e;
	}; // ITER

public: // construct
	CONFIG(REPO& r);
	~CONFIG(){
		// incomplete(); //flush here? flush always? let's see.
	}

public:
	ITEM operator[](std::string const& s){
		return ITEM(*this, s);
	}
	// private:
	ITEM create(const std::string& what){
		if(git_config_set_string(_cfg, what.c_str(), "notyet")){
			throw "incomplete";
		}else{
		}
		return (*this)[what];
	}

public: // iterate
	ITER begin(){
	 	return ITER(*this);
	}
	ITER end(){
		return ITER(*this, 0);
	}

private:
//	REPO& _repo;
	git_config * /*const*/ _cfg;
}; // CONFIG

std::ostream& operator<< (std::ostream& o, CONFIG::ITEM const& c)
{
	return c.print(o);
}

class COMMITS {
public: // types	
	class COMMIT_WALKER {
	public:
		COMMIT_WALKER(COMMITS& c): _c(&c){
			std::fill((char*)&_id, (char*)(&_id)+sizeof(git_oid), 1);
			operator++();
		}
		COMMIT_WALKER(): _c(nullptr){
			std::fill((char*)&_id, (char*)(&_id)+sizeof(git_oid), 0);
		} // "end"

		COMMIT_WALKER& operator++(){
			assert(!git_oid_iszero(&_id));
			if (!_c->_walk || git_revwalk_next(&_id, _c->_walk)) {
				std::fill((char*)&_id, (char*)(&_id)+sizeof(git_oid), 0);
			}else{
			}
			return *this;
		}
		bool operator==(COMMIT_WALKER const&x) const{
			return git_oid_equal(&_id, &x._id);
		}
		bool operator!=(COMMIT_WALKER const&x){
			return !(*this==x);
		}
		COMMIT operator*();
	private:
		COMMITS* _c;
		git_oid _id;
	};

public: // construct
	COMMITS(REPO& r);
	~COMMITS(){
		git_revwalk_free(_walk);
	}

public: // misc
	COMMIT create(std::string const& message);


public: // iterate
	COMMIT_WALKER begin(){
		return COMMIT_WALKER(*this);
	}
	COMMIT_WALKER end(){
		return COMMIT_WALKER();
	}

private:
	REPO& _repo;
	git_revwalk* _walk;
};

class BRANCHES;

// a git repository
class REPO{
public:
	enum create_t{
		_create
	};

public:
	REPO(create_t, std::string path="."){
		if(!git_repository_open(&_repo, path.c_str())){
			throw EXCEPTION("already there. doesn't work");
		}else if(int err=git_repository_init(&_repo, path.c_str(), 0)){
			throw EXCEPTION("internal error creating repo: " + std::to_string(err));
		}else{
		}
	}

	REPO(std::string path=".") : _repo(nullptr) {
		git_libgit2_init();

		int error=git_repository_open(&_repo, path.c_str());
		if (error < 0) {
			const git_error *e = giterr_last();
			throw EXCEPTION_CANT_FIND(
			          " repository (" + std::string(e->message) + ", "
			          + "error " +  std::to_string(error) + " " + std::to_string(e->klass) + ")");
		}

	}
	~REPO(){
		git_repository_free(_repo);
		git_libgit2_shutdown();
	}

public:
	COMMITS commits(){
		return COMMITS(*this);
	}
	CONFIG config(){
		return CONFIG(*this);
	}
	BRANCHES branches();
	void checkout(std::string const&);

private:
	git_repository* _repo;

public:
	friend class COMMITS;
	friend class CONFIG;
	friend class BRANCHES;
}; // REPO

COMMIT COMMITS::create(std::string const& msg)
{
	git_signature* sig=nullptr;
	git_index* index=nullptr;
	git_oid tree_id;
	const git_oid* parent_id;
	git_oid oid;
	git_tree* tree=nullptr;
	int err=0;
	std::string errstring;
	git_repository* r(_repo._repo);
	char const* m=msg.c_str();

	git_commit* parent=nullptr;
	int parents=0;
	git_reference* pr;
	if(!git_repository_head(&pr, _repo._repo)){
		parent_id = git_reference_target(pr);
		if(!git_commit_lookup(&parent, r, parent_id)){
			parents = 1;
		}else{ untested();
		}
	}else{
		// ignore. perhaps empty repository
	}


	if ((err=git_signature_default(&sig, r)) < 0) {
	}else if ((err=git_repository_index(&index, r)) < 0) {
	}else if ((err=git_index_write_tree(&tree_id, index)) < 0) {
	}else if ((err=git_tree_lookup(&tree, r, &tree_id)) < 0) {
	}else if ((err=git_commit_create_v(&oid, r, "HEAD", sig, sig,
				                          NULL, m, tree, parents, parent)) < 0)
	{
	}else{
	}

	git_index_free(index);
	git_tree_free(tree);
	git_signature_free(sig);

	if(err<0){
		errstring = std::string(giterr_last()->message);
		throw EXCEPTION(errstring + ": " + std::to_string(err));
	}else{
		return COMMIT(oid, r);
	}
}

class BRANCH{
public:
	explicit BRANCH(git_reference* ref):_ref(ref){}

public:
	std::string name() const{
		const char* c;
		git_branch_name(&c, _ref);
		assert(c);
		return std::string(c);
	}
	std::ostream& print(std::ostream& o) const{
		return o << name();
	}	

	//BRANCH& reset(COMMIT&){
	//	incomplete();
	//}

private:
	git_reference* _ref;
};

std::ostream& operator<< (std::ostream& o, BRANCH const& c)
{
	return c.print(o);
}

class BRANCHES{
public:
	class iterator{
	public:
		explicit iterator(BRANCHES& b): _br(b), _e(nullptr){
			if( git_branch_iterator_new(&_i, b._repo._repo, GIT_BRANCH_LOCAL)){
				throw EXCEPTION("branch iter error");
			}else{
			}

			operator++();
		}
		explicit iterator(BRANCHES& b, int): _i(nullptr),  _br(b), _e(nullptr){
		}
		iterator(iterator const& b): _i(b._i), _br(b._br), _e(b._e){
		}

	public:
		bool operator==(iterator const&x) const{
			return _i==x._i;
		}
		bool operator!=(iterator const&x) const{
			return !(*this==x);
		}
		BRANCH operator*(){
			return BRANCH(_e);
		}
		iterator& operator++(){
			assert(_i); // not end.
			// need to keep a pointer to entry, as we can only fetch it once (?!)
			int err=git_branch_next(&_e, &ot, _i);
			if (GIT_ITEROVER==err) {
				_i = nullptr;
				_e = nullptr;
				// assert(*this==iterator(_br, 0));
			}else if (err){
				throw EXCEPTION("branch iter error");
				// unreachable(); // ?!
				_i = nullptr;
				_e = nullptr;
				assert(*this==iterator(_br, 0));
			}else{
			}
			return *this;
		}

	private:
		git_branch_iterator* _i;
		BRANCHES& _br;
		git_reference* _e;
		git_branch_t ot;
	}; // iterator
public:
	explicit BRANCHES(REPO& r) : _repo(r)
	{
	}

public:
	iterator begin(){
	 	return iterator(*this);
	}
	iterator end(){
		return iterator(*this, 0);
	}
	BRANCH create(std::string const& name){
		git_reference* out;
		git_reference* r;
		if(git_repository_head(&r, _repo._repo)){ untested();
			throw EXCEPTION("cant obtain reference to HEAD");
		}else{
		}
		const git_oid* h=git_reference_target(r);
		git_commit* c;
		if(git_commit_lookup(&c, _repo._repo, h)){ untested();
			throw EXCEPTION("cannot lookup HEAD commit");
		}else{
		}
		// strdup required?
		if( git_branch_create( &out, _repo._repo, name.c_str(), c, 0/*force*/)){
			free(c);
			throw EXCEPTION_INVALID(name);
		}else{
		}

		return BRANCH(out);
	}
	void erase(std::string const& name){
		git_reference* r;
		if( git_reference_dwim(&r, _repo._repo, name.c_str())){ untested();
			throw EXCEPTION_CANT_FIND(name);
		}else if( git_branch_delete(r)){
			throw EXCEPTION("error deleting branch " + name);
		}else{
		}
	}

	// BRANCH& operator[](string const& branchname){
	//     maybe later.
	// }

private:
	REPO& _repo;
}; // BRANCHES

inline BRANCHES REPO::branches()
{
	return BRANCHES(*this);
}

inline CONFIG::CONFIG(REPO& r) // : _repo(r)
{
	if(git_repository_config(&_cfg, r._repo)){
		throw EXCEPTION("can't open config for repo");
	}else{
	}
}
// ---------------------------------------------------------------------------- //
inline COMMITS::COMMITS(REPO& r)
	: _repo(r)
{
	git_revwalk_new(&_walk, _repo._repo);

	int error;
	git_object *obj;

	if ((error = git_revparse_single(&obj, _repo._repo, "HEAD")) < 0){
		// cannot resolve HEAD.
		_walk = nullptr;
	}else{

		error = git_revwalk_push(_walk, git_object_id(obj));
		git_object_free(obj);
		if(error){ untested();
			throw error;
		}else{
		}
	}
}

inline COMMIT COMMITS::COMMIT_WALKER::operator*()
{
	return COMMIT(_id, _c->_repo._repo);
}

inline CONFIG::ITEM CONFIG::ITER::operator*()
{
	return CONFIG::ITEM(_e, _i, _cfg);
}

static int resolve_refish(git_annotated_commit **commit, git_repository *repo, const char *refish)
{
	git_reference *ref;
	git_object *obj;
	int err = 0;

	assert(commit != NULL);

	err = git_reference_dwim(&ref, repo, refish);
	assert(ref);

	if (err == GIT_OK) {
		git_annotated_commit_from_ref(commit, repo, ref);
//		git_reference_free(ref); memory leak
		return 0;
	}else{
	}

	err = git_revparse_single(&obj, repo, refish);
	if (err == GIT_OK) {
		err = git_annotated_commit_lookup(commit, repo, git_object_id(obj));
//		git_object_free(obj); memory leak
	}else{
	}

	return err;
}

//inline void REPO::checkout(COMMIT const& refname)
//inline void REPO::checkout(BRANCH const& refname)
inline void REPO::checkout(std::string const& refname)
{
	git_checkout_options opts=GIT_CHECKOUT_OPTIONS_INIT;
	git_commit *target_commit=nullptr;
	git_annotated_commit *target=nullptr;

	opts.checkout_strategy = GIT_CHECKOUT_SAFE;

	if (resolve_refish(&target, _repo, refname.c_str())){
		git_annotated_commit_free(target);

		throw EXCEPTION_CANT_FIND(refname);
		//		giterr_last()->message
	}else{
	}

	if(git_commit_lookup(&target_commit, _repo, git_annotated_commit_id(target))){
		throw EXCEPTION("cant lookup commit for " + refname);
	}else if(git_checkout_tree(_repo, (const git_object *)target_commit, &opts)){
		git_annotated_commit_free(target);
		throw EXCEPTION("error during checkout "+refname+": " + std::string(giterr_last()->message));
	}else if ( 1 ) { //  } auto ref=git_annotated_commit_ref(target)) {
		if(git_repository_set_head(_repo, ("refs/heads/"+refname).c_str())){
			git_annotated_commit_free(target);
			throw EXCEPTION("can't update HEAD to " + refname + ": "
					 + std::string(giterr_last()->message));
		}else{
		}
	}else{
		git_annotated_commit_free(target);
	}
} // checkout

} // GITPP

#endif
