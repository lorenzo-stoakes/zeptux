#include <iostream>

#include "test_user.h"

int main()
{
	const std::string res = test_range();
	if (!res.empty())
		std::cerr << res << std::endl;

	std::cout << "// zeptux USER  test run complete" << std::endl;

	return EXIT_SUCCESS;
}
