# The filter Command and AI-Assisted Condition Matching

The filter command accepts one (or optionally more) condition(s). If more conditions are supported, they are given as distinct arguments separated by spaces. The command prints all reports that satisfy all of them (conditions are implicitly joined by AND). A condition is a single string of the form:
`field:operator:value`
Supported fields: `severity`, `category`, `inspector`, `timestamp`. Supported operators: `==`, `!=`, `<`, `<=`, `>`, `>=`.

---

## Tool Used
Gemini 3.1 Pro (Preview)

## Prompts Used
1. "Generate a C function `int parse_condition(const char *input, char *field, char *op, char *value);` which splits a `field:operator:value` string into its three parts."
2. "Generate a C function `int match_condition(Report *r, const char *field, const char *op, const char *value);` which returns 1 if the record satisfies the condition and 0 otherwise."

## What was generated & Changed
The AI generated `parse_condition` using `sscanf`. I modified it to handle potential buffer overflows by specifying field widths such as `%31[^:]:%7[^:]:%255s` to securely store string conditions internally. 
The AI generated `match_condition` using `strcmp` for fields and `atoi` / `atol` for integer/time data correctly mapping to the supported operators `==`, `!=`, `<`, `<=`, `>`, `>=`.

## What was learned
I learned how simple AI prompts can accelerate generating boilerplate parsing loops and matching cases for string mapping, and how we must review the generated string formatters to ensure array boundaries are strictly respected in C.
