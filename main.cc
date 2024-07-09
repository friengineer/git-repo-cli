#include <iostream>
#include "hci0.h"
#include "gitpp5.h"

using namespace std;
using namespace GITPP;

bool exists = false;

// creates a new Git repository
class HCI_CREATE : public HCI_ACTION{
public:
  HCI_CREATE() : HCI_ACTION("yes") {}
private:
  void do_it(){
    out() << "Creating repository\n";
    REPO r(REPO::_create);
    out() << "Repository created\n\n";
		out() << "Press 'escape' to go to the main menu\n";
  }
};
HCI_CREATE hci_create;

// makes a new variable in the config file
class MAKE_VARIABLE : public HCI_ACTION{
public:
  MAKE_VARIABLE() : HCI_ACTION("create new variable") {}
private:
  void do_it(){
    string input;

    out() << "Input the value you would like the variable to have and press enter";
    out() << "(maximum 30 characters, no spaces):\n\n";
    in() >> input;
    out() << "Input: " << input << "\n\n";

    REPO r;
    auto c = r.config();

    c.create("user.test");
    c["user.test"] = input;

    out() << "\n\nVariable added\n\n";
    out() << "(Press 'b' to go to the main menu)\n";
  }
};
MAKE_VARIABLE make_variable;

// list config page
class LISTCONFIG_PAGE : public HCI_PAGE{
	using HCI_PAGE::HCI_PAGE;
public:
	void show(){
		out() << "-------------------------\n";
    out() << "List Config\n\n";

    REPO r;
    auto c = r.config();

  	CONFIG::ITEM N = c["user.name"];

  	out() << "Hello " << N.value() << "\n\n";

  	out() << "Your commits\n";
  	for(auto i : r.commits()){
  		out() << i << " " << i.signature().name() << "\n";
  	}

  	out() << "\n";
  	out() << "These are your variables\n";

  	for(auto i : r.config()){
  		out() << i << "\n";
  	}

    out() << "\nPress any key to leave\n";
		out() << "-------------------------\n";
	}
};

// list commits page
class LISTCOMMIT_PAGE : public HCI_PAGE{
	using HCI_PAGE::HCI_PAGE;
public:
	void show(){
		out() << "-------------------------\n";
    out() << "List Commits\n\n";
		out() << "This is the list commits page\n\n";
		out() << "Press any key to leave\n";
		out() << "-------------------------\n";
	}
};

// menu to create a new repository
class NOREPO_MENU : public HCI_MENU {
public:
	explicit NOREPO_MENU(HCI_APPLICATION& ctx)
	    : HCI_MENU(ctx, "norepo") {
		add(0x1b, &hci_esc);
		add('y', &hci_create);
		add('n', &hci_leave);
	}

public:
	void show(){
		out() << "-------------------------\n";
		out() << "No repository found\n";
		out() << "-------------------------\n\n";
		HCI_MENU::show();
    out() << "\n";
	}
	void enter(){
		while(true){
			try{
				HCI_MENU::enter();
				break;
			}catch( HCI_UP const&){
				continue;
			}
		}
	}
private:
};

// configure repository menu
class EDIT_MENU : public HCI_MENU {
public:
  explicit EDIT_MENU(HCI_APPLICATION& ctx)
      : HCI_MENU(ctx, "configure repository") {
    add(0x1b, &hci_esc);
    add('a', &make_variable);
    add('b', &hci_up);
  }

public:
  void show(){
    out() << "-------------------------\n";
    out() << "Configure Repository\n";
    out() << "-------------------------\n\n";
    out() << "Your Git repository in <CWD>\n\n";

    REPO r;
    int count = 1;

    for(auto i : r.config()){
  		out() << to_string(count) << ". " << i << "\n";
      count += 1;
  	}

    out() << "\nWould you like to create a new configuration variable?\n";
    out() << "(Press 'a' to create one and 'b' to go back to the previous menu";
    out() << "\n\n";
    HCI_MENU::show();
    out() << "\n";
  }
  void enter(){
    while(true){
      try{
        HCI_MENU::enter();
        break;
      }catch( HCI_UP const&){
        continue;
      }
    }
  }
private:
  MAKE_VARIABLE make_variable;
};

// main menu
class ISREPO_MENU : public HCI_MENU {
public:
	explicit ISREPO_MENU(HCI_APPLICATION& ctx)
	    : HCI_MENU(ctx, "isrepo"), _list_config("list config"),
      _edit_menu(ctx), _list_commit("list commits") {
		add(0x1b, &hci_esc);
		add('c', &_list_config);
		add('e', &_edit_menu);
		add('l', &_list_commit);
		add('q', &hci_quit);
	}

public:
	void show(){
		out() << "-------------------------\n";
		out() << "Your Git repository\n";
		out() << "-------------------------\n\n";
		HCI_MENU::show();
    out() << "\n";
	}
	void enter(){
		while(true){
			try{
				HCI_MENU::enter();
				break;
			}catch( HCI_UP const&){
				continue;
			}
		}
	}
private:
	LISTCONFIG_PAGE _list_config;
	EDIT_MENU _edit_menu;
	LISTCOMMIT_PAGE _list_commit;
};

class APPLICATION : public HCI_APPLICATION{
public:
	APPLICATION() : HCI_APPLICATION(), _main_menu(*this), _side_menu(*this) {
	}
public:
	void show(){
    // shows a menu to create a repository if a repository is not present
		if (exists == false){
			try{
				_side_menu.enter();
			}
			catch(HCI_ESCAPE const&){
				// escape pressed
			}
		}

    REPO r;

    // shows the main menu
		try{
			_main_menu.enter();
		}
		catch(HCI_ESCAPE const&){
			// escape pressed
		}
	}

private:
	NOREPO_MENU _side_menu;
	ISREPO_MENU _main_menu;
};

// main program
int main(){
  // tries to retrieve a repository
	try{
		REPO r;
		exists = true;
	}
	catch (EXCEPTION_CANT_FIND& e){

	}

  // starts the application
	APPLICATION application;
	return application.exec();
}
