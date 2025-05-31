#define _CRT_SECURE_NO_WARNINGS
#define MAX_LABELS 1024                                                 // max number of labels in a file (used to make a finite label table)
#define MAX_LABEL_LEN 50                                                    // max number of characters in a label (given)
#define MAX_LINE_LEN 500                                                // max number of characters in a line (given)
#define MEM_SIZE 4096                                                   // total 32-bit words in memin.txt
#define NUM_OPCODES 22
#define NUM_REGS 16

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "assembler.h"

// -----------------------------------------------------------------------
//    --- Opcode and Register tables ---
// -----------------------------------------------------------------------

// Opcode table (in order - 0 to 21)
static const char* opcode_table[] = {
    "add", "sub", "mul", "and", "or", "xor", "sll", "sra", "srl", "beq",
    "bne", "blt", "bgt", "ble", "bge", "jal", "lw", "sw", "reti", "in",
    "out", "halt"
};

// List of registers
static const char* reg_table[16] = {
    "$zero", "$imm", "$v0", "$a0", "$a1", "$a2", "$a3", "$t0",
    "$t1",   "$t2",  "$s0", "$s1", "$s2", "$gp", "$sp", "$ra"
};

// -----------------------------------------------------------------------
//    --- Label Struct definition ---
// -----------------------------------------------------------------------

typedef struct {
    char name[MAX_LABEL_LEN];                                               // label name
    int address;                                                        // address to convert to in second pass
} Label;

static Label label_table[MAX_LABELS];                                   // table containing all labels, of type Label

int main(int argc, char** argv) {

    // -----------------------------------------------------------------------
    //    --- Open Assembly file and get contents ---
    // -----------------------------------------------------------------------

    const char* in_filename = argv[1];
    const char* out_filename = argv[2];

    FILE* asm_file = fopen(in_filename, "r");
    if (!asm_file) {
        fprintf(stderr, "Couldn't open the assembly file for first pass!");
        return 1;
    }

    // -----------------------------------------------------------------------
    //    --- First Pass - go over each line, clean it up, store labels ---
    // -----------------------------------------------------------------------

    char asm_line[MAX_LINE_LEN];                                        // initialize new line of assembly code
    int word_address = 0;                                               // address for next instruction in memory
    int line_num = 0;

    while (fgets(asm_line, MAX_LINE_LEN, asm_file)) {                    // loop over lines of assemble code and get each line

        // --- Remove comment and whitespace ---------------------
        line_num++;
        remComment(asm_line);
        trim(asm_line);
        if (asm_line[0] == '\0') {
            continue;                                                   // get rid of lines that are completely blank or just have comments
        }

        // --- Check if label ------------------------------------
        int asm_line_len = strlen(asm_line);                            // get length of asm_line to check for ":"
        if (asm_line[asm_line_len - 1] == ':') {                           // it's a label!
            asm_line[asm_line_len - 1] = '\0';                            // remove colon
            trim(asm_line);
            addLabel(asm_line, word_address);                           // add label to label table with current word address
            continue;                                                   // exit while-loop (bypasses other checks)
        }

        // --- Not label -> must be an instruction ---------------
        char tmp_word_check[MAX_LINE_LEN];                                         // copy asm_line to tmp (we need asm_line later and musn't override)
        strncpy(tmp_word_check, asm_line, MAX_LINE_LEN);
        tmp_word_check[MAX_LINE_LEN - 1] = '\0';
        char* firstToken = strtok_s(tmp_word_check, " \t,", &tmp_word_check);
        if (firstToken && strcmp(firstToken, ".word") == 0) {
            continue;
        }

        char tmp[MAX_LINE_LEN];                                         // copy asm_line to tmp (we need asm_line later and musn't override)
        strncpy(tmp, asm_line, MAX_LINE_LEN);
        tmp[MAX_LINE_LEN - 1] = '\0';

        char* tokens[5];                                                // initialize tokens array
        int token_num = tokenizeInst(tmp, tokens);                      // perform tokenization

        // --- big_imm check ------------------------------------       // DO WE NEED TO DO ALL THE NUMBER CHECKS?
        const char* imm_str = tokens[4];                                // get pointer to imm token in tokens array
        int is_label_imm = 0;
        int imm_val = 0;

        if (imm_str[0] == '0' && imm_str[1] == 'x') {                    // hex check - starts with "0x"
            imm_val = (int)strtol(imm_str, NULL, 16);                  // take string value and convert to hexadecimal number
        }
        else if (isDecimal(imm_str)) {                                   // decimal check - can be negative
            imm_val = (int)strtol(imm_str, NULL, 10);                  // take string value and convert to decimal number (int)
        }
        else {                                                          // must be a label
            is_label_imm = 1;                                           // we won't put the label value in the code yet (only in pass 2)
        }                                                               // here we save space for putting the label address later

        if (is_label_imm || !fitsInSigned8(imm_val)) {                   // if imm_val is too long to be an 8b integer, or is a label
            word_address += 2;                                          // save two rows
        }
        else {                                                          // big_imm = 0 and imm_val fits in 8b
            word_address += 1;
        }
    }

    fclose(asm_file);                                                   // we edited this object during pass 1, so we'll close and reopen
    // the file to get an unedited opject (to rewind the pointer)

// --------------------------------------------------------------------------
//    --- Second Pass - Enter label addresses, convert to machine code ---
// --------------------------------------------------------------------------

    uint32_t memory_image[MEM_SIZE];                                    // initialize memory of size 4096 rows

    for (int i = 0; i < MEM_SIZE; i++) {                                 // initialize to 0
        memory_image[i] = 0;
    }

    asm_file = fopen(in_filename, "r");                                 // reopen the file - we want to see the ":" etc to know
    if (!asm_file) {                                                     // where labels are, etc
        fprintf(stderr, "Couldn't open the assembly file for pass 2!");
        return 1;
    }

    int current_word = 0;                                               // current word in memory
    line_num = 0;

    while (fgets(asm_line, MAX_LINE_LEN, asm_file)) {                    // go over each row a second time

        // --- Remove comments and trim whitespace again ----------
        line_num++;
        remComment(asm_line);
        trim(asm_line);

        if (asm_line[0] == '\0') {                                       // ignore blank lines
            continue;
        }

        // --- Check if label ------------------------------------
        size_t asm_line_len_2 = strlen(asm_line);
        if (asm_line[asm_line_len_2 - 1] == ':') {                         // don't need to do anything, we already saved all labels
            continue;
        }

        // --- Retokenize instruction ----------------------------
        char tmp2[MAX_LINE_LEN];                                        // need to make another copy
        strncpy(tmp2, asm_line, MAX_LINE_LEN);
        tmp2[MAX_LINE_LEN - 1] = '\0';

        char* tokens[5];
        int token_num_2 = tokenizeInst(tmp2, tokens);

        if (strcmp(tokens[0], ".word") == 0) {
            if (processWordDirective(tokens, token_num_2, line_num, memory_image)) {
                // processWordDirective already printed an error
                fclose(asm_file);
                return 1;
            }
            continue;
        }

        // --- Convert opcode to 8b value -------------------------
        int opcode = convertInstruction(tokens[0]);

        // --- Convert registers to 4b value ----------------------
        int rd = convertReg(tokens[1]);
        int rs = convertReg(tokens[2]);
        int rt = convertReg(tokens[3]);

        // --- Check if big_imm == 1 or 0 -------------------------
        const char* imm_str2 = tokens[4];
        int is_label = 0;                                               // check if a value is a label or not for handling
        int imm_val2 = 0;                                               // will hold the imm value (hex, dec or label address)

        if (imm_str2[0] == '0' && imm_str2[1] == 'x') {                  // now we convert the values to actually use them
            imm_val2 = (int)strtol(imm_str2, NULL, 16);
        }
        else if (isDecimal(imm_str2)) {
            imm_val2 = (int)strtol(imm_str2, NULL, 10);
        }
        else {
            is_label = 1;                                               // must be a label - turn this to 1 for future handling
            imm_val2 = getLabelAddr(imm_str2);                          // get the label address according to the label's name
        }

        int use_bigimm;                                                 // whether to use one or two rows for the instruction

        if (is_label || !fitsInSigned8(imm_val2)) {
            use_bigimm = 1;                                             // labels are always with big_imm == 1, and if the integer is too big
        }
        else {
            use_bigimm = 0;                                             // not a label, integer fits in 8b
        }

        // --- Construct first instruction ------------------------
        uint32_t first_instruction = 0;

        uint32_t opcode_field = opcode << 24;                           // shift all fields to the right positions in the instruction
        first_instruction |= opcode_field;                              // do OR to add them to the instruction

        uint32_t rd_field = rd << 20;
        first_instruction |= rd_field;

        uint32_t rs_field = rs << 16;
        first_instruction |= rs_field;

        uint32_t rt_field = rt << 12;
        first_instruction |= rt_field;

        if (use_bigimm) {
            first_instruction |= (1 << 8);                              // add big_imm == 1 at the 8th bit
        }
        else {
            uint8_t short_imm = (uint8_t)(imm_val2 & 0xFF);            // cast imm_val2 to 8b, taking only the LSBs
            first_instruction |= ((uint32_t)short_imm);                // add short_imm to LSBs
        }

        // --- Add first instruction to memory --------------------
        memory_image[current_word] = first_instruction;
        current_word++;                                                 // increment to go to next word

        // --- Construct second instruction (if needed) -----------
        if (use_bigimm) {
            memory_image[current_word] = (uint32_t)imm_val2;           // just put in the 32b value (int type)
            current_word++;
        }
    }

    fclose(asm_file);                                                   // close file after finishing second pass

    // -----------------------------------------------------------------------
    //    --- Create machine code file and transfer the data ---
    // -----------------------------------------------------------------------

    FILE* machine_code_file = fopen(out_filename, "w");
    if (!machine_code_file) {
        fprintf(stderr, "Couldn't open machine code file for output!");
        return 1;
    }

    for (int i = 0; i < MEM_SIZE; i++) {                                 // print each 32b instruction to the machine code file
        printBinaryWord(machine_code_file, memory_image[i]);
    }

    fclose(machine_code_file);

    printf("Assembled program: used %d words out of %d.\n", current_word, MEM_SIZE);
    return 0;
}

// ----------------------------------------------------------------
//      --- Opcode and Register Table Functions ---
// ----------------------------------------------------------------

// Convert opcode string to its opcode number (0-21), return -1 if not found
int convertInstruction(const char* opcode) {
    for (int i = 0; i < NUM_OPCODES; i++) {                              // compare input opcode with i'th entry in opcode_table
        if (strcmp(opcode, opcode_table[i]) == 0) {
            return i;
        }
    }
    return -1;
}

// Convert register name to it's number (0-15), return -1 if not found (get rid of warning)
int convertReg(const char* reg) {
    for (int i = 0; i < NUM_REGS; i++) {
        if (strcmp(reg, reg_table[i]) == 0) {                            // compare input register with i'th entry in reg_table
            return i;
            if (i == 3) printf("%s", reg);
        }
    }
    return -1;
}

// ----------------------------------------------------------------
//      --- First Pass function to create Label table ---
// ----------------------------------------------------------------

static int label_count = 0;

// Add label to label_table
void addLabel(const char* lab_name, int addr) {
    if (label_count >= MAX_LABELS) {
        fprintf(stderr, "Label table has too many labels!\n");
        exit(1);
    }
    strncpy(label_table[label_count].name, lab_name, MAX_LABEL_LEN - 1);    // add lab_name to label_table name field in current label_count position
    label_table[label_count].name[MAX_LABEL_LEN - 1] = '\0';                // close the string w/ '\0'
    label_table[label_count].address = addr;                                // save address in label_table address field
    label_count++;                                                          // increment to wait for next label
}

// Get label_name's address - return address or −1 if not found (get rid of warning)
int getLabelAddr(const char* lab_name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(label_table[i].name, lab_name) == 0) {
            return label_table[i].address;
        }
    }
    return -1;
}

// ----------------------------------------------------------------
//      --- Remove comments and trim white spaces ---
// ----------------------------------------------------------------

void remComment(char* line) {
    char* first_hash = strchr(line, '#');                               // get pointer to first occurance of '#' in line
    if (first_hash) {                                                   // if this exists
        *first_hash = '\0';                                             // end the string here
    }
}

void trim(char* s) {
    // Trim leading whitespace
    char* start = s;
    while (*start && isspace((unsigned char)*start)) {                  // check if isspace() and move pointer if true
        start++;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }

    // Trim trailing whitespace
    char* end = s + strlen(s) - 1;                                      // get last non-'\0' character
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';                                                    // cut end of string off by replacing w/ '\0'
        end--;
    }
}

// ----------------------------------------------------------------
//      --- Get Instruction Fields ---
// ----------------------------------------------------------------

// Split on whitespace or comma
// int tokenizeInst(char *line, char *tokens[]) {
//     int count = 0;
//     char *token = strtok(line, " ,");                                   // divide input string up by spaces and commas
//     while (token != NULL && count < 5) {
//         tokens[count++] = token;
//         token = strtok(line, " ,");
//     }
//     return count;                                                       // used to check if we got all the fields
// }

int tokenizeInst(char* line, char* tokens[]) {
    int count = 0;
    char* saveptr = NULL;
    char* p = strtok_s(line, " \t,", &saveptr);
    while (p && count < 5) {
        tokens[count++] = p;
        p = strtok_s(NULL, " \t,", &saveptr);
    }
    return count;
}

// ----------------------------------------------------------------
//      --- Number conversion functions ---
// ----------------------------------------------------------------

// Check if the string is a decimal number
int isDecimal(const char* str_num) {
    if (*str_num == '+' || *str_num == '-') str_num++;                  // skip "+/-" just for the check
    while (*str_num) {
        if (!isdigit((unsigned char)*str_num)) return 0;               // check each char to see if its a digit
        str_num++;
    }
    return 1;                                                           // all char's in string are digits
}

// Check if a value can fit into 8b (signed)
int fitsInSigned8(int value) {
    return (value >= -128 && value <= 127);
}

// ----------------------------------------------------------------
//      --- Function to print 32b machine code to file ---
// ----------------------------------------------------------------


void printBinaryWord(FILE* file_name, uint32_t word) {
    for (int bit = 31; bit >= 0; bit--) {
        uint32_t shifted_word = word >> bit;
        char bit_val = (shifted_word & 0x1) ? '1' : '0';               // right shifts word until the required bit is the LSB
        fputc(bit_val, file_name);                                     // then does & with 1b = 1 to zero all other remaining bits
    }
    fputc('\n', file_name);                                            // at the end of the word, put a new line
}

int processWordDirective(
    char* tokens[],      // array of token strings
    int      ntok,         // number of tokens found
    int      lineNo,       // current line number (for error messages)
    uint32_t memory_image[] // the 4096‐word memory image to write into
) {

    // Parse address (tokens[1]) as either hex, decimal, or label→number
    int32_t addrVal = 0;
    if (tokens[1][0] == '0' && tokens[1][1] == 'x') {
        // Hex literal
        addrVal = (int32_t)strtol(tokens[1], NULL, 16);
    }
    else if (isDecimal(tokens[1])) {
        // Decimal literal (possibly negative, but negative addresses are invalid)
        addrVal = (int32_t)strtol(tokens[1], NULL, 10);
    }
    else {
        // Must be a label
        int lbl = getLabelAddr(tokens[1]);
        if (lbl < 0) {
            fprintf(stderr,
                "Error (line %d): unknown label or address `%s`\n",
                lineNo, tokens[1]
            );
            return 1;
        }
        addrVal = lbl;
    }

    // 3) Check address range
    if (addrVal < 0 || addrVal >= MEM_SIZE) {
        fprintf(stderr,
            "Error (line %d): `.word` address %d out of range [0..%d]\n",
            lineNo, addrVal, MEM_SIZE - 1
        );
        return 1;
    }

    // 4) Parse data (tokens[2]) as hex, decimal, or label→number
    int32_t dataVal = 0;
    if (tokens[2][0] == '0' && tokens[2][1] == 'x') {
        // Hex literal
        dataVal = (int32_t)strtol(tokens[2], NULL, 16);
    }
    else if (isDecimal(tokens[2])) {
        // Decimal literal
        dataVal = (int32_t)strtol(tokens[2], NULL, 10);
    }
    else {
        // Must be a label for data
        int lbl2 = getLabelAddr(tokens[2]);
        if (lbl2 < 0) {
            fprintf(stderr,
                "Error (line %d): unknown label or data `%s`\n",
                lineNo, tokens[2]
            );
            return 1;
        }
        dataVal = lbl2;
    }

    // 5) Finally, write the 32-bit dataVal into memory_image[addrVal]
    memory_image[(int)addrVal] = (uint32_t)dataVal;

    // 6) Success
    return 0;
}
