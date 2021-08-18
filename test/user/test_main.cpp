#include <iostream>

#include "test_user.h"

int main()
{
	std::string res = test_range();
	if (!res.empty())
		std::cout << "FAIL: " << res << std::endl;

	res = test_spinlock();
	if (!res.empty())
		std::cout << "FAIL: " << res << std::endl;

	std::cout << "// zeptux USER  test run complete" << std::endl;

	return EXIT_SUCCESS;
}
