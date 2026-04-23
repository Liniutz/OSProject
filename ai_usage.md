# AI Usage in `city_manager`

## Tool Used
Gemini 3.1 Pro (Preview)

## Prompts Used
1. "Given this C struct for a Report:
   ```c
   typedef struct {
       int id;
       char inspector[32];
       double latitude;
       double longitude;
       char category[32];
       int severity;
       time_t timestamp;
       char description[256];
   } Report;
   ```
   Generate a C function `int parse_condition(const char *input, char *field, char *op, char *value);` which splits a `field:operator:value` string into its three parts."

2. "Generate a C function `int match_condition(Report *r, const char *field, const char *op, const char *value);` which returns 1 if the record satisfies the condition (operators: ==, !=, <, <=, >, >=) and 0 otherwise. Fields are string or int."

## What was generated & Changed
The AI generated `parse_condition` using `sscanf`, which successfully parses strings in the format `%[^:]:%[^:]:%s`. I modified it to handle potential buffer overflows by specifying field widths such as `%31[^:]:%3[^:]:%255s`. Let's assume this is safe for our use.
The AI generated `match_condition` using `strcmp` for fields and `atoi` / `atol` for integer/time data correctly handling relational operators. We adapted the generated comparison code slightly to ensure safe array comparison semantics.

## What was learned
I learned how simple AI prompts can yield boilerplate parsing functions and how we must review code to ensure array boundaries are respected in C.
