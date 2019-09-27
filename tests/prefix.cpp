#include "main.h"
#include <cfenv>
#include <iostream>

TEST(OutcursesPrefix, CornerInts) {
	char buf[PREFIXSTRLEN + 1];
	genprefix(0, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("0", buf);
	genprefix(0, 1, buf, sizeof(buf), 1, 1000, 'i');
	EXPECT_STREQ("0", buf); // no suffix on < mult
	genprefix(1, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1", buf);
	genprefix(999, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("999", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1K", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1000, 'i');
	EXPECT_STREQ("1Ki", buf);
	genprefix(1000, 1, buf, sizeof(buf), 1, 1024, 'i');
	EXPECT_STREQ("1000", buf); // FIXME should be 0.977Ki
	genprefix(1023, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	genprefix(1023, 1, buf, sizeof(buf), 1, 1024, 'i');
	EXPECT_STREQ("1023", buf);
	genprefix(1024, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	genprefix(1024, 1, buf, sizeof(buf), 1, 1024, 'i');
	EXPECT_STREQ("1Ki", buf);
	genprefix(1025, 1, buf, sizeof(buf), 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	genprefix(1025, 1, buf, sizeof(buf), 1, 1024, 'i');
	EXPECT_STREQ("1Ki", buf);
}

TEST(OutcursesPrefix, Maxints) {
	char buf[PREFIXSTRLEN + 1];
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
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	int i = 0;
	do{
		genprefix(val, 1, buf, sizeof(buf), 0, 1000, '\0');
		const int sidx = i / 3;
		snprintf(gold, sizeof(gold), "%ju.00%c", goldval, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if((goldval *= 10) == 1000){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST(OutcursesPrefix, PowersOfTenNoDec) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	int i = 0;
	do{
		genprefix(val, 1, buf, sizeof(buf), 1, 1000, '\0');
		const int sidx = i / 3;
		snprintf(gold, sizeof(gold), "%ju%c", goldval, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if((goldval *= 10) == 1000){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST(OutcursesPrefix, PowersOfTwo) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	int i = 0;
	do{
		genprefix(val, 1, buf, sizeof(buf), 0, 1024, 'i');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%ju.00%ci", goldval, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 2;
		if((goldval *= 2) == 1024){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST(OutcursesPrefix, PowersOfTwoNoDec) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	int i = 0;
	do{
		genprefix(val, 1, buf, sizeof(buf), 1, 1024, 'i');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%ju%ci", goldval, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 2;
		if((goldval *= 2) == 1024){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST(OutcursesPrefix, PowersOfTwoAsTens) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	int i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		genprefix(val, 1, buf, sizeof(buf), 0, 1000, '\0');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%.2f%c",
				 ((double)val) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 2;
		if(i % 10 == 9){
			vfloor *= 1000;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST(OutcursesPrefix, PowersOfTenAsTwos) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	int i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		genprefix(val, 1, buf, sizeof(buf), 0, 1024, 'i');
		const int sidx = (i - 1) / 3;
		snprintf(gold, sizeof(gold), "%.2f%ci",
				 ((double)val) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if(i && i % 3 == 0){
			vfloor *= 1024;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}
