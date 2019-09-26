#include "main.h"
#include <iostream>

TEST(OutcursesPrefix, CornerInts) {
	char buf[PREFIXSTRLEN];

	genprefix(0, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("0", buf);
	genprefix(0, 1, buf, sizeof(buf), 1, 1000, 'i');
	EXPECT_STREQ("0", buf); // no suffix on < mult
	genprefix(1, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1K", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1000, 'i');
	EXPECT_STREQ("1Ki", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1024, 'i');
	EXPECT_STREQ("1000", buf); // FIXME should be 0.977Ki
	genprefix(1024, 1, buf, sizeof(buf), 1, 1024, 'i');
	EXPECT_STREQ("1Ki", buf);
	// FIXME these will change based on the size of intmax_t and uintmax_t
	genprefix(INTMAX_MAX - 1, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("9.22E", buf);
	genprefix(INTMAX_MAX, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("9.22E", buf);
	genprefix(UINTMAX_MAX - 1, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("18.44E", buf);
	genprefix(UINTMAX_MAX, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("18.44E", buf);
}

const char suffixes[] = "\0KMGTPE";

TEST(OutcursesPrefix, PowersOfTen) {
	char buf[PREFIXSTRLEN + 1];
	uintmax_t val = 1;
	char str1[] = "1.00 ";
	char str2[] = "10.00 ";
	char str3[] = "100.00 ";
	int i = 0;
	do{
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
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST(OutcursesPrefix, PowersOfTenNoDec) {
	char buf[PREFIXSTRLEN + 1];
	uintmax_t val = 1;
	char str1[] = "1 ";
	char str2[] = "10 ";
	char str3[] = "100 ";
	int i = 0;
	do{
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
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}
