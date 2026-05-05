# AI Filter Command Usage

## Tool used
Gemini

## Prompts I gave
1. "Given a C struct for city infrastructure reports, generate a function `int parse_condition(const char *input, char *field, char *op, char *value);` which splits a `field:operator:value` string into its three parts for a filter command."
2. "Generate a function `int match_condition(Report *r, const char *field, const char *op, const char *value);` which returns 1 if a record satisfies a condition (fields: severity, category, inspector, timestamp; operators: ==, !=, <, <=, >, >=) and 0 otherwise."

## What was generated
- For `parse_condition`, it provided a function using `sscanf()` with the format string `"%[^:]:%[^:]:%s"`.
- For `match_condition`, it generated a large if/else if block comparing the `field` string using `strcmp`, then converting the `value` string to int (using `atoi`) or checking string fields via `strcmp` again, returning 1 on match.

## What I changed and why
- In `parse_condition`, I changed the `sscanf` format string to `%31[^:]:%7[^:]:%255s`. The AI's version was unsafe and could cause buffer overflows if the command line arguments were too long. Adding limits ensures it fits in my C buffers.
- In `match_condition`, I had to make sure the timestamp was converted using `atol` instead of `atoi` because `time_t` needs a larger type.

## What I learned
I learned that AI is good at writing tedious boilerplate code like if-else chains for parsing commands, but it often forgets basic C memory safety rules like buffer limits in `sscanf`. You always have to double-check its C string handling.
