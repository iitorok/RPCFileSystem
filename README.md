# File System
A filesystem operated with remote procedural calls. Has multi-user capability and error-checking protections that prevent users from creating files or directories under branches they do not own.

Allows multiple users to access directories or files at the same time with appropriate reader-writer locks on each node (directory or file) to prevent accessing corrupted data. 
