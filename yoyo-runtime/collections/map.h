#ifndef YOYO_RUNTIME_COLLECTIONS_MAP_H
#define YOYO_RUNTIME_COLLECTIONS_MAP_H

#include "yoyo-runtime.h"

#include "collection.h"
#include "set.h"

typedef struct AbstractYoyoMap {
	AbstractYoyoCollection* o;

	void (*put)(struct AbstractYoyoMap*, YValue*, YValue*, YThread*);
	bool (*has)(struct AbstractYoyoMap*, YValue*, YThread*);
	YValue* (*get)(struct AbstractYoyoMap*, YValue*, YThread*);
	void (*remove)(struct AbstractYoyoMap*, YValue*, YThread*);
	AbstractYoyoSet* (*keySet)(struct AbstractYoyoMap*, YThread*);
} AbstractYoyoMap;
#endif
