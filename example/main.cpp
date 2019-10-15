#include <stdio.h>
#include <string>
#include "md5.h"

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return 1;
	}

	const std::string msg(argv[1]);
	std::string str;
	
	// benchmark
	for (int i = 0; i < 1000000; i++) {
		str = md5::MD5::GetDigest(msg);
	}

	printf("%s", str.data());
	puts("");

	return 0;
}
