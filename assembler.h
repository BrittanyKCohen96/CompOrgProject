#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LABELS 1024
#define LABEL_LEN 64
#define ASM_LINE_LEN 512
#define MEM_SIZE 4096                                                   // total 32-bit words in memin.txt
#define NUM_OPCODES 22
#define NUM_REGS 16

// Convert opcode string to its opcode number (0-21), return -1 if not found
int convertInstruction(const char* opcode);

// Convert register name to it's number (0-15), return -1 if not found 
int convertReg(const char* reg);

// Add label to label_table
void addLabel(const char* lab_name, int addr);

// Get label_name address - return address or −1 if not found
int getLabelAddr(const char* lab_name);

// Get rid of comments
void remComment(char* line);

void trim(char* s);

// Split on whitespace or comma to get different parts of the instruction from assembly code
int tokenizeInst(char* line, char* tokens[]);

// Check whether a string contains a decimal value
int isDecimal(const char* s);

// Check whether a decimal value fits in signed 8b
int fitsInSigned8(int value);

void printBinaryWord(FILE* file_name, uint32_t word);

int processWordDirective(
    char* tokens[],      // array of token strings
    int      ntok,         // number of tokens found
    int      lineNo,       // current line number (for error messages)
    uint32_t memory_image[] // the 4096‐word memory image to write into
);

#endif // ASSEMBLER_H
