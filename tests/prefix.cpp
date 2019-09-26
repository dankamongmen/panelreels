#include "main.h"
#include <iostream>

TEST(OutcursesPrefix, EasyInts) {
	char buf[PREFIXSTRLEN];

	genprefix(0, 1, buf, sizeof(buf), 1, 1000, '\0');
	ASSERT_STREQ("0", buf);
	genprefix(0, 1, buf, sizeof(buf), 1, 1000, 'i');
	ASSERT_STREQ("0", buf); // no suffix on < mult
	genprefix(1, 1, buf, sizeof(buf), 1, 1000, '\0');
	ASSERT_STREQ("1", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1000, '\0');
	ASSERT_STREQ("1K", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1000, 'i');
	ASSERT_STREQ("1Ki", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1024, 'i');
	ASSERT_STREQ("1000", buf); // FIXME should be 0.977Ki
	genprefix(1024, 1, buf, sizeof(buf), 1, 1024, 'i');
	ASSERT_STREQ("1Ki", buf);
	genprefix(INTMAX_MAX, 1, buf, sizeof(buf), 1, 1000, '\0');
	// FIXME
	genprefix(UINTMAX_MAX, 1, buf, sizeof(buf), 1, 1000, '\0');
	// FIXME
	// FIXME
}
