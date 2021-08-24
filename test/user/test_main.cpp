#include <iostream>

#include "test_user.h"

int main()
{
	const auto check = [] (std::string res) {
		if (!res.empty())
			std::cout << "FAIL: " << res << std::endl;
	};

	check(test_range());
	check(test_spinlock());

	std::cout << "// zeptux USER  test run complete" << std::endl;

	return EXIT_SUCCESS;
}
