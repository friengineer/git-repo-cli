
#include "hci0.h"

std::ostream& HCI::out(){
	return std::cout;
}
std::istream& HCI::in(){
	return std::cin;
}
HCI_BEEP hci_beep;
HCI_QUIT hci_quit;
HCI_UP hci_up;
HCI_ESCAPE hci_esc;
HCI_LEAVE hci_leave;
