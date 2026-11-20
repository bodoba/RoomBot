# Project Coding Rules

Language:
- ISO C99

Formatting:
- K&R style
- 4 spaces indentation
- UNIX line endings

Control Flow:
- Avoid early return
- Avoid continue
- Avoid complex nesting

Comments:
- Doxygen style comments
- Public APIs always documented

Header:
- Always include a GPLv3 license header tpo new files

Error Handling:
- Always check return values
- Explicit error paths
- No silent failures

Linux:
- Prefer POSIX compliant APIs
- Avoid GNU-specific extensions unless justified

Reviews:
- Be critical
- Prioritize correctness over style
- Highlight risks first