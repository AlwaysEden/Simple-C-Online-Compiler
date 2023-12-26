# Simple-C-Online-Compiler

## Development Enviroment
- Ubuntu OS
- C language
- Vim

## Needs
When you want to implement Online Coding Webstie or check your student's homework, It could be Skeleton code you can develop.

## Project Description
This code has Server-Client network structure. Client send the code to server, and then server compile the code and send the result to client. 
Server work in multi-thread way and Server use popen() to execute the code which received from the client.
Server can detect the compile error and Client can recevie the error content, because Server send the error in detail.

## Note
This is just the Skeleton code to compile the code online. You need to develop and care the Input code(Client-sending code), for Server can't distinguish between normal code and malicious code. If I have a opportunity to develop this code, I will implement the Server-protecting function.
