#ifndef COLLECTIONMODE_H
#define COLLECTIONMODE_H

// Write mode for collections.
enum CollectionMode {
	STANDARD = 0, // standard collections
	SEQUENTIAL = 1, // sequential order collections, 'dhufs'
	SQUARE = 2, // square collections
	UNCOLLECTION = 3, // uncollections
	ALTSQUARE = 4, // square collections / alt order (even/uneven)
	ALTUNCOLLECTION = 5 // uncollections / alt order (even/uneven)
};

#endif