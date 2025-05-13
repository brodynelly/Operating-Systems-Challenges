#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "../include/error_handling.h"
// Allocates and zeros all the fields in the array of records
// \param records a NULL Record_t pointer that will be allocated to the size
//	of num_records
// \param num_records the number of records that will be created
// \return 0 for successful allocation and zero initialization
// else returns -1 for bad parameters, -2 for unsuccessful allocation
int create_blank_records(Record_t **records, const size_t num_records)
{
	// check for bad parameters 
	if ( *records != NULL || num_records == 0 ) {return -1; }

	// allocation for records
	*records = (Record_t*) malloc(num_records* sizeof(Record_t));
	// check for bad allocation 
	if ( *records == NULL ) { return -2; }
	memset(*records,0,sizeof(Record_t) * num_records);
	return 0;	
}

// Reads an array of structures from a binary file
// \param input_filename the file that contains the binary array of structures
// \param records an allocated Records array
// \param num_records the number of records to read from the file
// \return 0 for successfully filled records
// else returns -1 for bad parameters, -2 for could not open file, -3 for read any error that prevent readings
int read_records(const char *input_filename, Record_t *records, const size_t num_records) {

	// bad params
	if ( input_filename == NULL || records == NULL || num_records == 0 ) { return -1;  }

	// couldn't open file
	int fd = open(input_filename, O_RDONLY);
	if ( fd == -1 ) { return -2; }

	ssize_t data_read = 0;
	for (size_t i = 0; i < num_records; ++i) {
		data_read = read(fd,&records[i], sizeof(Record_t));	
		// error for something preventing readings
		if ( data_read == -1 || data_read == 0 ) {
			return -3; 
		}

	}
	return 0;
}

// Create an alloacted and filled record with the passed name and age
// \param new_record a NULL poiner Record_t struct
// \param name the name to be stored in the new_record
// 	restriction: name must be less than 50 characters
// \param age the age to be stored in the new_record
//	restriction: age must be between 1 and 200
// \return 0 for successful allocation and zero initialization
// else returns -1 for bad parameters, -2 for unsuccessful allocation
int create_record(Record_t **new_record, const char* name, int age)
{
	// check for bad parameters 
	if ( *new_record != NULL || name == NULL || name == "\0" || strlen(name) >= 50 || age > 199 || age < 2 ) {return -1; }
	
	*new_record = (Record_t*) malloc(sizeof(Record_t));
	// check for bad push
	if ( *new_record == NULL ) { return -2; }
	memcpy((*new_record)->name,name,sizeof(char) * strlen(name));
	
	(*new_record)->name[MAX_NAME_LEN - 1] = 0;	
	(*new_record)->age = age;
	return 0;

}
