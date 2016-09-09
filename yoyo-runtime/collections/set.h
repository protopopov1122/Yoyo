#ifndef YOYO_RUNTIME_COLLECTIONS_SET_H
#define YOYO_RUNTIME_COLLECTIONS_SET_H

#include "yoyo-runtime.h"
#include "collection.h"

typedef struct AbstractYoyoSet {
	AbstractYoyoCollection col;

	bool (*has)(struct AbstractYoyoSet*, YValue*, YThread*);
	bool (*add)(struct AbstractYoyoSet*, YValue*, YThread*);
	bool (*remove)(struct AbstractYoyoSet*, YValue*, YThread*);
	YoyoIterator* (*iter)(struct AbstractYoyoSet*, YThread*);
} AbstractYoyoSet;

#endif
