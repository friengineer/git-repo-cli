#ifndef HCI_H
#define HCI_H

#include <iostream>
#include <map>
#include <unistd.h>
#include <termios.h>
#include <stdio.h> // getchar
#include <stdlib.h> // exit

// #include "platform.h"
#if __cplusplus >= 201103L
#else
#define noexcept throw()
#endif

// #include "trace.h"
#define untested()
#define incomplete() ( \
	std::cerr << "@@#\n@@@\nincomplete:" \
			  << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n" )
#define unreachable() ( \
	std::cerr << "@@#\n@@@\nunreachable:" \
			  << __FILE__ << ":" << __LINE__ << ":" << __func__ << "\n" )

/* -------------------------------------------------------------------------- */
class HCI_EXCEPTION : public std::exception	{
	public:
		explicit HCI_EXCEPTION(std::string const& s) : _what(s) {
		}
		~HCI_EXCEPTION() noexcept { }
	public:
		virtual const char* what() const noexcept {
			return _what.c_str();
		}
	private:
		std::string _what;
};

/* -------------------------------------------------------------------------- */
//input ends...
class HCI_EOI : public HCI_EXCEPTION {
	public:
		explicit HCI_EOI() : HCI_EXCEPTION("end of input") { }
};

/* -------------------------------------------------------------------------- */
// human interaction base class
class HCI {
	protected:
		HCI(std::string const& n="unnamed") : _name(n) { }
		virtual ~HCI() { }
	protected:
		virtual void show() {
			out() << "HCI " << name() << "\n";
		}
		void clear() const {
			out() << std::string(40, '\n');
		}
	public:
		// void set_name(std::string const& n) { incomplete(); } // needed?
		std::string name() const {
			return _name;
		}

	protected: // I/O
		static std::istream& in();
		static std::ostream& out();

		bool getstring(std::string& s) {
			out() << s;
			char a = getkey();

			for ( ; ; ) {
				if (a == '\n') {
					break;
				} else if (a != '\b' && a != 127) {
					// backspace
					s += std::string(1, a);
					out() << a;
				} else if (s.size()) { untested();
					s.resize(s.size() - 1);
					out() << "\b \b";
				} else { untested();
				}
				a = getkey();
			}

			return true; // ok.
		}

		int getkey() const {
			int c = 0;
			set_tty_attributes();
			c = getchar();

			if (c == '\b') { untested();
			} else if (!c) {
				incomplete();
				// reachable?
				throw "weird kind of eof\n";
			} else if (c == EOF) {
				throw HCI_EOI();
			}

			restore_tty_attributes();
			return c;
		}

		void beep() {
			// this is system dependent. you may not hear it.
			// use screen & turn on visible bell. then you can see it.
			out() << '\a';
		}
	private:
		// getkey. neet to bypass terminal buffer
		// http://www.cplusplus.com/forum/beginner/33529/
		void set_tty_attributes() const {
			if( !isatty(fileno(stdin))) {
				return;
			}

			if (tcgetattr(STDIN_FILENO, &_tty_attr) < 0) {
				throw HCI_EXCEPTION("tty error 1");
			}

			_backup_c_lflag = _tty_attr.c_lflag;
			_tty_attr.c_lflag &= ~ICANON;
			_tty_attr.c_lflag &= ~ECHO;

			if (tcsetattr(STDIN_FILENO, 0, &_tty_attr) < 0) {
				throw HCI_EXCEPTION("tty error 2");
			}
		}

		void restore_tty_attributes() const {
			if (!isatty(fileno(stdin))) {
				return;
			}
			_tty_attr.c_lflag = _backup_c_lflag;

			if (tcsetattr(STDIN_FILENO, 0, &_tty_attr) < 0) {
				throw HCI_EXCEPTION("tty error 3");
			}
		}
		
		mutable struct termios _tty_attr;
		mutable tcflag_t _backup_c_lflag;
	private:
		std::string _name;
};

// do things
class HCI_ACTION : public HCI {
	public:
		explicit HCI_ACTION(std::string const& s = "action")
			: HCI(s) {}
	public:
		virtual void do_it()=0;
};

// quit program
class HCI_QUIT : public HCI_ACTION {
	public:
		HCI_QUIT() : HCI_ACTION("quit") {}
	private:
		void do_it() {
			out() << " goodbye\n";
			sleep(1);
			exit(0);
		}
};
extern HCI_QUIT hci_quit;

// just beep
class HCI_BEEP : public HCI_ACTION {
	public:
		HCI_BEEP() : HCI_ACTION("beep") {}
	private:
		void do_it() {
			beep();
		}
};
extern HCI_BEEP hci_beep;

// quits the application when a repository has not been created
class HCI_LEAVE : public HCI_ACTION {
	public:
		HCI_LEAVE() : HCI_ACTION("no, quit") {}
	private:
		void do_it() {
			out() << "\n\nRepository not created, exiting program\n";
			sleep(1);
			exit(0);
		}
};
extern HCI_LEAVE hci_leave;

// display stuff on the screen
class HCI_PAGE : public HCI {
	public:
		explicit HCI_PAGE(std::string const& name="unnamed page")
			: HCI(name)
		{
		}
	public:
		virtual void enter() {
			clear();
			show();
			// set_status(std::string("showing ") + name() + ". Hit any key");
			pause();
			throw(HCI_LEAVE());
		}
		void pause() { untested();
			getkey();
		}
	public:
		void help() {
		}
};

class HCI_APPLICATION;

// display choices to a user and query
class HCI_MENU : public HCI {
	private: // internal types
		class menu_opt_t {
		public:
			menu_opt_t() : _label("incomplete"), _action(NULL) {}
			menu_opt_t(std::string const& l, HCI* h)
				: _label(l), _action(h) {}
		public:
			HCI* action() { return _action; }
			std::string const& label() const { return _label; }
		private:
			std::string _label;
			HCI* _action;
		};
		typedef std::map<char, menu_opt_t> map_type;
	public:
		explicit HCI_MENU(HCI_APPLICATION& ctx, std::string name = "menu") :
			HCI(name), _ctx(ctx) {}
	public:
		void add(char c, HCI* h, std::string label = "") {
			if (_m.find(c) != _m.end()) {
				std::cerr << "WARNING: overwriting " << c << " in " << name() << "\n";
			}

			if (label == "") {
				label = h->name();
			} else {
			}

			menu_opt_t o(label, h);
			// emplace?
			_m[c] = o;
		}

		void help() {}

		void show() {
			for(map_type::const_iterator i = _m.begin(); i != _m.end(); ++i) {
				if (isalnum(i->first)) {
					show_menu_option(i->first, i->second.label());
				} else {
					// escape, backspace etc..
				}
			}
		}

		void enter();
		bool query();
	private:
		void exec(HCI* h) {
			// See Stroustrup 15.4.5
			if (HCI_ACTION* a = dynamic_cast<HCI_ACTION*>(h)) { untested();
				a->do_it();
			} else if (HCI_MENU* m = dynamic_cast<HCI_MENU*>(h)) { untested();
				// submenu?
				m->enter();
			} else if (HCI_PAGE* m = dynamic_cast<HCI_PAGE*>(h)) { untested();
				m->enter(); // display?
			} else {
				unreachable();
			}
		}
	protected:
		virtual void show_menu_option(char c, std::string const& s) {
			out() << " " << c << " " << s << "\n";
		}
		HCI_APPLICATION& ctx() { return _ctx; }
	private:
		HCI_APPLICATION& _ctx;
		map_type _m;
};

class HCI_UP : public HCI_ACTION {
	void do_it() { untested();
		throw *this;
	}
};
extern HCI_UP hci_up;

// can be used to signal escape button
class HCI_ESCAPE : public HCI_ACTION {
	void do_it() {
		throw *this;
	}
};
extern HCI_ESCAPE hci_esc;

// an application
class HCI_APPLICATION : public HCI_PAGE {
	public:
		HCI_APPLICATION(std::string const& name = "unnamed application",
						int argc = 0, char const *argv[] = NULL)
			: HCI_PAGE(name), _status("")
		{
		}
	public: // protect?
		void set_status(std::string const& s, size_t tail = 0);

	private:
		std::string _status;
	public:
		int exec() {
			try {
				show();
				return 0;
			}
			catch (HCI_EOI const&) {
				std::cerr << "broken pipe\n";
				return 0;
			}
			catch (HCI_ESCAPE const&) {
				out() << "unhandled escape event\n";
				return 1;
			}
		}
	private: // not used right now.
		int _argc;
		char const** _argv;
};
/* -------------------------------------------------------------------------- */
inline void HCI_APPLICATION::set_status(std::string const& s, size_t tail /*= 0*/ ) {
	if (tail) {
		// this is a bit of a hack.
	} else if (_status.size() > s.size()) {
		tail = _status.size() - s.size();
	} else {
		tail = 0;
	}

	out() << '\r' << s;

	for (; tail; --tail) { untested();
		out() << ' ';
	}

	_status = s;
}
/* -------------------------------------------------------------------------- */
inline void HCI_MENU::enter()
{
	while (true) {
		clear();
		show();

		try {
			query();
			break;
		}
		catch (HCI_LEAVE const&) {
			continue;
		}
	}
}

/* -------------------------------------------------------------------------- */
// wait for user input, process.
inline bool HCI_MENU::query() {
	bool _insist = true; // for now.
	while (_insist) {
		char i;
		i = getkey();
		map_type::iterator f(_m.find(i));
			// TODO: handle ESC

		if (f != _m.end()) {
			HCI* h = f->second.action();
			exec(h);
		} else if (i == 0x1b) {
			_ctx.set_status("Esc is not assigned");
		} else if (!isalnum(i)) {
			_ctx.set_status("can't do that");
		} else {
			_ctx.set_status("'" + std::string(1, i) + "' not assigned" );
			// beep?!
		}
	}

	incomplete();
	return false;
}
/* -------------------------------------------------------------------------- */
#endif
