#include "yoyo.h"

YOYO_FUNCTION(YSTD_TIME_CLOCK) {
	clock_t cl = clock();
	return newInteger((int64_t) cl, th);
}

YOYO_FUNCTION(YSTD_TIME_CLOCKS_PER_SEC) {
	return newInteger((int64_t) CLOCKS_PER_SEC, th);
}

YOYO_FUNCTION(YSTD_TIME_TIME) {
	time_t t = time(NULL);
	return newInteger((int64_t) t, th);
}

YOYO_FUNCTION(YSTD_TIME_DATE) {
	time_t t = (time_t) getInteger(args[0]);
	struct tm* ctm = localtime(&t);
	YObject* ytm = th->runtime->newObject(NULL, th);
	((YoyoObject*) ytm)->linkc++;

	OBJECT_NEW(ytm, L"day", newInteger(ctm->tm_mday, th), th);
	OBJECT_NEW(ytm, L"month", newInteger(ctm->tm_mon + 1, th), th);
	OBJECT_NEW(ytm, L"year", newInteger(ctm->tm_year + 1900, th), th);
	OBJECT_NEW(ytm, L"second", newInteger(ctm->tm_sec, th), th);
	OBJECT_NEW(ytm, L"minute", newInteger(ctm->tm_min, th), th);
	OBJECT_NEW(ytm, L"hour", newInteger(ctm->tm_hour, th), th);
	OBJECT_NEW(ytm, L"time", args[0], th);

	((YoyoObject*) ytm)->linkc--;
	return (YValue*) ytm;
}

YOYO_FUNCTION(YSTD_TIME_GMDATE) {
	time_t t = (time_t) getInteger(args[0]);
	struct tm* ctm = gmtime(&t);
	YObject* ytm = th->runtime->newObject(NULL, th);
	((YoyoObject*) ytm)->linkc++;

	OBJECT_NEW(ytm, L"day", newInteger(ctm->tm_mday, th), th);
	OBJECT_NEW(ytm, L"month", newInteger(ctm->tm_mon + 1, th), th);
	OBJECT_NEW(ytm, L"year", newInteger(ctm->tm_year + 1900, th), th);
	OBJECT_NEW(ytm, L"second", newInteger(ctm->tm_sec, th), th);
	OBJECT_NEW(ytm, L"minute", newInteger(ctm->tm_min, th), th);
	OBJECT_NEW(ytm, L"hour", newInteger(ctm->tm_hour, th), th);
	OBJECT_NEW(ytm, L"time", args[0], th);

	((YoyoObject*) ytm)->linkc--;
	return (YValue*) ytm;
}
