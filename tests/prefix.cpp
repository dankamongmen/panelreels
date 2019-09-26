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

const char suffixes[] = "\0KMGTPEY";

TEST(OutcursesPrefix, PowersOfTen) {
	char buf[PREFIXSTRLEN + 1];
	uintmax_t val = 1;
	char str1[] = "1.00 ";
	char str2[] = "10.00 ";
	char str3[] = "100.00 ";
	for(int i = 0 ; i < sizeof(suffixes) * 3 ; ++i){
		genprefix(val, 1, buf, sizeof(buf), 0, 1000, '\0');
		int sidx = i / 3;
		switch(i % 3){
			case 0:
			str1[4] = suffixes[sidx];
			EXPECT_STREQ(str1, buf);
			break;
			case 1:
			str2[5] = suffixes[sidx];
			EXPECT_STREQ(str2, buf);
			break;
			case 2:
			str3[6] = suffixes[sidx];
			EXPECT_STREQ(str3, buf);
			break;
		}
		val *= 10;
	}
}

TEST(OutcursesPrefix, PowersOfTenNoDec) {
	char buf[PREFIXSTRLEN + 1];
	uintmax_t val = 1;
	char str1[] = "1 ";
	char str2[] = "10 ";
	char str3[] = "100 ";
	for(int i = 0 ; i < sizeof(suffixes) * 3 ; ++i){
		genprefix(val, 1, buf, sizeof(buf), 1, 1000, '\0');
		int sidx = i / 3;
		switch(i % 3){
			case 0:
			str1[1] = suffixes[sidx];
			EXPECT_STREQ(str1, buf);
			break;
			case 1:
			str2[2] = suffixes[sidx];
			EXPECT_STREQ(str2, buf);
			break;
			case 2:
			str3[3] = suffixes[sidx];
			EXPECT_STREQ(str3, buf);
			break;
		}
		val *= 10;
	}
}
