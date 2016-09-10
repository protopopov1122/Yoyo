#ifndef YOYO_RUNTIME_COLLECTIONS_COLLECTION_H
#define YOYO_RUNTIME_COLLECTIONS_COLLECTION_H

#include "yoyo-runtime.h"

typedef struct AbstractYoyoCollection {
	YoyoObject o;

	size_t (*size)(struct AbstractYoyoCollection*, YThread*);
} AbstractYoyoCollection;

YArray* newList(YThread*);

typedef struct AbstractYoyoSet AbstractYoyoSet;
typedef struct AbstractYoyoMap AbstractYoyoMap;

#include "set.h"
#include "map.h"

AbstractYoyoSet* newMapBasedSet(AbstractYoyoMap*, YThread*);


#endif
