#include "Event.h"  

#include <iostream>

struct Accountant {
	void OnMoneyPaidMemberFunc(int& amount) 
	{ 
		std::cout << "OnMoneyPaid called with param " << amount << std::endl; 
		amount = 100;
	}
};

void OnShiftStarted()
{
	std::cout << "OnShiftStarted called" << std::endl;
}

int main()
{
	Event<void> ShiftStarted;
	Event<int&> MoneyPaid;
	Accountant accountant;
	int value = 200;

	std::cout << "Starting" << std::endl;

	ShiftStarted += EventHandler::Bind(&OnShiftStarted);
	MoneyPaid += EventHandler::Bind(&Accountant::OnMoneyPaidMemberFunc, &accountant);

	std::cout << "Calling" << std::endl;
	MoneyPaid(value);
	ShiftStarted();

	std::cout << "Calling with changed Value" << std::endl;
	MoneyPaid(value);
	
	MoneyPaid -= EventHandler::Bind(&Accountant::OnMoneyPaidMemberFunc, &accountant);
	ShiftStarted -= EventHandler::Bind(&OnShiftStarted);

	std::cout << "Calling removed" << std::endl;
	MoneyPaid(value);
	ShiftStarted();

	std::cout << "Ending" << std::endl;
}