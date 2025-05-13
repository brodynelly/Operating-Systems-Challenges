#include "../include/structures.h"
#include <stdio.h>


/*  IMPORTED FROM STRUCTURES.H --> ONLY FOR REFERENCE 
// The size of this struct is less than ideal
// Data is in poor order resulting in padding for Data Alignment
// (Assuming typical data sizes)
typedef struct bad
{
	char a;			// this variable is at offset 0
	long long b;	// this variable is at offset 1, requires alignment 8, 7 bytes of padding added in order to comply
	char c;			// this variable is at offset 16, requires no additional padding.
	int d;			// this variable is at offset 17, requires alignment 4, 3 bytes of additional padding in order to comply
}bad_t;
// Assuming the data size is 14 bytes, the padding is 10 bytes. So sizeof(BAD) would be 24 bytes. Almost doubled in size.

// Generally best to do largest -> smallest members
typedef struct good
{
	long long b;	// this variable is at offset 0
	int d;			// this variable is at offset 8, no padding required
	char a;			// this variable is at offset 16, no padding required as char is 1-byte aligned
	char c;			// this variable is at offset 17, no padding required as char is 1-byte aligned
}good_t;
// Assuming the data size is 14 bytes (same data types). No padding so sizeof(GOOD) would be 14 bytes. We saved 10 bytes!
// Since this padding is repeated per struct we can fit many more structs in the same space in comparison to the BAD struct.
*/

// Purpose: Simple function to show the alignment of different variables on standard output.
// Receives: Nothing
// Returns: Nothing
int compare_structs(sample_t* a, sample_t* b)
{
	
	if(a->a != b->a || a->b != b->b || a->c != b->c) return 0;
	return 1;
}

// Purpose: Compares two SAMPLE structs member-by-member
// Receives: struct_a - first struct to compare
//			 struct_b - second struct to compare
// Returns: 1 if the structs match, 0 otherwise.
void print_alignments()
{
	printf("Alignment of int is %zu bytes\n",__alignof__(int));
	printf("Alignment of double is %zu bytes\n",__alignof__(double));
	printf("Alignment of float is %zu bytes\n",__alignof__(float));
	printf("Alignment of char is %zu bytes\n",__alignof__(char));
	printf("Alignment of long long is %zu bytes\n",__alignof__(long long));
	printf("Alignment of short is %zu bytes\n",__alignof__(short));
	printf("Alignment of structs are %zu bytes\n",__alignof__(fruit_t));
}

// Purpose: Initializes an array of fruits with the specified number of apples and oranges
// Receives: fruit_t* a - pointer to an array of fruits
//						int apples - the number of apples
//						int oranges - the number of oranges
// Returns: -1 if there was an error, 0 otherwise.
int sort_fruit(const fruit_t* a,int* apples,int* oranges, const size_t size)
{
	if ( a == NULL || apples == 0 || oranges == 0 || size == 0 ) return -1; 

	// iterate through the array 
	size_t i = 0; 
	while ( i < size ){
		if ( a[i].type == 1 ) {
			apples[0]++;
		}
		else {
			oranges[0]++; 
		}
		i++; 
	} 
	return 0; 
}

// Purpose: Initializes an array of fruits with the specified number of apples and oranges
// Receives: fruit_t* a - pointer to an array of fruits
//						int apples - the number of apples
//						int oranges - the number of oranges
// Returns: -1 if there was an error, 0 otherwise.
int initialize_array(fruit_t* a, int apples, int oranges)
{
	if ( a == NULL || apples == 0 || oranges == 0 ) return -1; 

	int total = apples + oranges; 

	// create apples array 
    for (int i = 0; i < apples; i++)
    {
		apple_t* apple_ptr = (apple_t*) &a[i];
        apple_ptr->type = 1;  // Set type to 1 for apple.
        if (initialize_apple(apple_ptr) != 0){
            return -1;
		}
    }

	// create apples array 
    for (int i = apples; i < total; i++)
    {
        orange_t* orange_ptr = (orange_t*) &a[i];
        orange_ptr->type = 2;  // Set type to 2 for orange.
        if (initialize_orange(orange_ptr) != 0)
            return -1;
    }

	return 0; 
}

// Purpose: Initializes an orange_t struct
// Receives: orange_t* - pointer to an orange_t struct
// Returns: -1 if there was an error, 0 otherwise.
int initialize_orange(orange_t* a)
{
	if (a == NULL) return -1;

    a->type = 2;      
    a->weight = 150;  
    a->peeled = 0;    

    return 0;

}

// Purpose: Initializes an apple_t struct
// Receives: apple_t* - pointer to an apple_t struct
// Returns: -1 if there was an error, 0 otherwise.
int initialize_apple(apple_t* a)
{
	if (a == NULL) return -1;

    a->type = 1;      
    a->weight = 120;  
	a->worms = 0;    

    return 0;

}
