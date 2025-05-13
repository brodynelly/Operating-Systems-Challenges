#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h> //INT_MIN // INT_MAX
#include <ctype.h> // isspace


#include "../include/sstring.h"

// Verifies if the passed character array is null terminated or not.
// \param str the character array that may be null terminated
// \param length to prevent buffer overflow while checking
// \return true if the character array is a string
bool string_valid(const char *str, const size_t length)
{
    // gaurd statement for invalid params 
    if ( str == NULL || length == 0 ) { return false; }

    // check last digits for ./0
    size_t len = length; 
    for (size_t i = 0; len > 0 && i < len; ++i) 
    {
        if ( str[i] == '\0' ) {
            return true; 
        }
    }

    // no null terminator 
    return false; 
}

// Copy the contents of the passed string upto the given length
// \param str is the string that will be copied into heap allocated string
// \param length the number of characters to copy
// \return an allocated pointer to new string, else NULL
char *string_duplicate(const char *str, const size_t length)
{
    // gaurd statement for invalid params 
    if ( str == NULL || length <= 0 ) { return NULL; }

    // create allocated string 
    char *heap = (char *)malloc(length + 1); 
    if (heap == NULL) {
        return NULL; 
    }

    // actually add string to new allocated string
    for (size_t i = 0; (length > 0) && (i < length); ++i) 
    {
        heap[i] = str[i]; 
    }

    // Null-terminate the new string
    heap[length] = '\0';

    // return heap
    return heap; 
}

// Checks to see if both strings contain the same characters
// \param str_a the string that will be compared against
// \param str_b the string that will be compared with str_a
// \param length the number of characters to be compared
bool string_equal(const char *str_a, const char *str_b, const size_t length)
{
    if ( str_a == NULL || str_b == NULL || length == 0 ) {
        return false; 
    }

    // get the minimum size of the length to make sure we don't get past each other
    size_t len_a = strlen(str_a);
    size_t len_b = strlen(str_b);
    size_t minimum = (len_a <= len_b) ? len_a : len_b;
    // compare the values 
    for (size_t i = 0; i < length && i <= minimum; ++i) {
        if (str_a[i] != str_b[i]) {
            return false; 
        }
    }
    return true; 
}

// Finds the number of characters in the string, not including the null terminator
// \param str the string to count the number of characters in the string
// \param length the max possible string length for the application
// \return the length of the string or -1 for invalid string
int string_length(const char *str, const size_t length)
{
    // gaurd statement against failed params
    if ( str == NULL || length == 0 ) {
        return -1; 
    }
    // go through the string and count the characters
    size_t i = 0; 
    for (; i < length && i < strlen(str); ++i ) {
        // terminate loop if \0 found 
        if ( str[i] == '\0') {
            break; 
        }
    }

    // add i because characters is i+1
    return i; 

}

// Split the incoming string to tokens that are stored in a passed allocated tokens string array
// \param str the string that will be used for tokenization
// \param delims the delimiters that will be used for splitting the str into segments
// \param str_length the lengt of the str
// \param tokens the string array that is pre-allocated and will contain the parsed tokens
// \param max_token_length the max length of a token string in the tokens string array with null terminator
// \param requested_tokens the number of possible strings that tokens string array can contain
// \return returns the number of actual parsed tokens, 0 for incorrect params, and -1 for incorrect token allocation
// Function to tokenize a string
// Function to tokenize a string
int string_tokenize(const char *str, const char *delims, const size_t str_length, char **tokens, const size_t max_token_length, const size_t requested_tokens)
{
    // gaurd statement against invalid Params
    if (str == NULL || delims == NULL || tokens == NULL || max_token_length == 0 || requested_tokens == 0) {
        return 0; 
    }

    // allocate memeory for copied string
    char *str_copy = malloc(str_length + 1);
    if (str_copy == NULL) {
        return -1;
    }

    strncpy(str_copy, str, str_length);
    str_copy[str_length] = '\0';  

    int token_count = 0;
    char *token = strtok(str_copy, delims);  

    // tokenization of the string logic
    while (token != NULL) {
        if (token_count >= requested_tokens) {
            free(str_copy);
            return -1; 
        }

        //check for valid token len
        if (strlen(token) < max_token_length) {
            tokens[token_count] = malloc(max_token_length);
            if (tokens[token_count] == NULL) {
                free(str_copy);  
                return -1; 
            }

            strncpy(tokens[token_count], token, max_token_length - 1);
            tokens[token_count][max_token_length - 1] = '\0'; 
            token_count++;
        }

        // next token
        token = strtok(NULL, delims);
    }

    free(str_copy);  

    // if string is loger then what is requested, tokenization failed 
    if (token_count > requested_tokens) {
        return -1;
    }

    return token_count;  // Return the number of parsed tokens
}

// Converts the passed string into a integer
// \param str The string that contains numbers
// \param converted_value the value converted from the passed string str
// \return true for a successful conversion, else false
bool string_to_int(const char *str, int *converted_value) {
       // gaurd statement for invalid passed params
    if (str == NULL || converted_value == NULL) { return false; }
    if (*str == '\0') { return false; }

    // check for white spaces and skip 
    while (isspace(*str)) { str++; }
    if (*str == '\0') { return false; }

    // im using strtol to convert an int to a long // 
    char *endptr;  
    long result = strtol(str, &endptr, 10);

    /* printf("Debug: str = '%s', endptr = '%s', result = %l ") */ // debug statement 

    // check for inavlid converstion or invalid characters 
    if (endptr == str || *endptr != '\0') { return false; }

    // check for integer  value is within possible range 
    if (result < INT_MIN || result > INT_MAX) { return false; }

    // assgn the temp int to passed 
    *converted_value = (int)result;

    return true;
}
