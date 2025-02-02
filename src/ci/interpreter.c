#include "interpreter.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "command_type.h"
#include "mem.h"
#include <stdlib.h>

static bool    cond_holds(Interpreter *intr, BranchCondition cond);
static int64_t fetch_number_value(Interpreter *intr, Operand *op, bool is_im);
static bool    print_base(Interpreter *intr, Command *cmd);

void interpreter_init(Interpreter *intr, LabelMap *map) {
    if (!intr) {
        return;
    }

    intr->had_error  = false;
    intr->label_map  = map;
    intr->is_greater = false;
    intr->is_equal   = false;
    intr->is_less    = false;
    intr->the_stack  = NULL;

    for (size_t i = 0; i < NUM_VARIABLES; i++) {
        intr->variables[i] = 0;
    }
}

void interpret(Interpreter *intr, Command *commands) {
    if (!intr || !commands) {
        return;
    }

    Command *current = commands;
    while (current && !intr->had_error) {
        switch (current->type) {
            // STUDENT TODO: process the commands and take actions as appropriate
            case CMD_MOV: {
                intr->variables[current->destination.num_val] = current->val_a.num_val;
                break;
            }
            case CMD_ADD: {
                int64_t add = intr->variables[current->val_a.num_val];
                if (current->is_b_immediate)
                    add += current->val_b.num_val;
                else
                    add += intr->variables[current->val_b.num_val];
                intr->variables[current->destination.num_val] = add;
                break;
            }
            case CMD_SUB: {
                int64_t sub = intr->variables[current->val_a.num_val];
                if (current->is_b_immediate)
                    sub -= current->val_b.num_val;
                else
                    sub -= intr->variables[current->val_b.num_val];
                intr->variables[current->destination.num_val] = sub;
                break;
            }
            case CMD_CMP: {
                intr->is_equal = false;
                intr->is_greater = false;
                intr->is_less = false;
                int64_t temp;
                if (current->is_b_immediate) 
                    temp = current->val_b.num_val;
                else
                    temp = intr->variables[current->val_b.num_val];
                if (intr->variables[current->val_a.num_val] > temp) 
                    intr->is_greater = true;
                else if (intr->variables[current->val_a.num_val] < temp)
                    intr->is_less = true;
                else
                    intr->is_equal = true;
                break;
            }
            case CMD_CMP_U: {
                intr->is_equal = false;
                intr->is_greater = false;
                intr->is_less = false;
                uint64_t val_a = (uint64_t) intr->variables[current->val_a.num_val];
                uint64_t val_b;
                if (current->is_b_immediate) 
                    val_b = current->val_b.num_val;
                else
                    val_b = intr->variables[current->val_b.num_val];
                if (val_a > val_b)
                    intr->is_greater = true;
                else if (val_a < val_b)
                    intr->is_less = true;
                else
                    intr->is_equal = true;
                break;
            }
            case CMD_PRINT:
                print_base(intr, current);
                break;
            case CMD_AND:
                intr->variables[current->destination.num_val] = intr->variables[current->val_a.num_val] & intr->variables[current->val_b.num_val];
                break;
            case CMD_ASR:
                intr->variables[current->destination.num_val] = intr->variables[current->val_a.num_val] >> current->val_b.num_val;
                break;
            case CMD_EOR:
                intr->variables[current->destination.num_val] = intr->variables[current->val_a.num_val] ^ intr->variables[current->val_b.num_val];
                break;
            case CMD_LSL:
                intr->variables[current->destination.num_val] = ((uint64_t)intr->variables[current->val_a.num_val]) << ((uint64_t) current->val_b.num_val);
                break;
            case CMD_LSR:
                intr->variables[current->destination.num_val] = ((uint64_t)intr->variables[current->val_a.num_val]) >> ((uint64_t) current->val_b.num_val);
                break;
            case CMD_ORR:
                intr->variables[current->destination.num_val] = intr->variables[current->val_a.num_val] | intr->variables[current->val_b.num_val];
                break;
            case CMD_STORE:  {
                if (current->is_a_immediate) {
                    if (!mem_store((uint8_t*)&intr->variables[current->destination.num_val], current->val_a.num_val, current->val_b.num_val)) {
                        intr->had_error = true;
                        return;
                    }
                }
                else {
                    if (!mem_store((uint8_t*)&intr->variables[current->destination.num_val], intr->variables[current->val_a.num_val], current->val_b.num_val)) {
                        intr->had_error = true;
                        return;
                    }
                }
                break;
            }
            case CMD_PUT: {
                char* str = current->val_a.str_val; 
                size_t length = strlen(str) + 1;   
                size_t start;

                if (current->is_b_immediate) {
                    start = current->val_b.num_val; 
                } else {
                    start = intr->variables[current->val_b.num_val]; 
                }

                for (size_t i = 0; i < length; i++) {
                    if (!mem_store((uint8_t*)&str[i], start + i, 1)) {
                        free(current->val_a.str_val);
                        intr->had_error = true;
                        return;
                    }
                }

                if (current->val_a.str_val != NULL) {
                    free(current->val_a.str_val); 
                    current->val_a.str_val = NULL; 
                }
                break;
            }
            case CMD_LOAD: {
                size_t start;
                if (current->is_b_immediate)
                    start = (size_t)current->val_b.num_val;
                else
                    start = (size_t)intr->variables[current->val_b.num_val];
                size_t num = (size_t)current->val_a.num_val;
                if (!(num == 1 || num == 2 || num == 4 || num == 8)) {
                    intr->had_error = true;
                    return;
                }
                intr->variables[current->destination.num_val] = 0;
                int64_t value = 0;
                mem_load((uint8_t*)&value, start, num);
                intr->variables[current->destination.num_val] = value;
                break;
            }
            case CMD_BRANCH: {
                bool run = true;
                switch (current->branch_condition) {
                    case BRANCH_EQUAL:
                        if (!intr->is_equal)
                            run = false;
                        break;
                    case BRANCH_GREATER_EQUAL:
                        if (!intr->is_equal && !intr->is_greater)
                            run = false;
                        break;
                    case BRANCH_GREATER:
                        if (!intr->is_greater)
                            run = false;
                        break;
                    case BRANCH_LESS:
                        if (!intr->is_less)
                            run = false;
                        break;
                    case BRANCH_LESS_EQUAL:
                        if (!intr->is_less && !intr->is_equal)
                            run = false;
                        break;
                    case BRANCH_NOT_EQUAL:
                        if (intr->is_equal)
                            run = false;
                        break;
                    // case BRANCH_NONE:
                    //     run = false;
                    //     break;
                    default:
                        run = true;
                        break;
                }
                if (run) {            
                    Entry* e = get_label(intr->label_map, current->val_a.str_val);
                    if (e == NULL) {
                        printf("Label not found: %s\n", current->val_a.str_val);
                        intr->had_error = true;
                        return;
                    }
                    current->next = e->command;
                }
                break;
            }
            case CMD_RET: {
                if (intr->the_stack == NULL) {
                    current->next = NULL;
                    return;
                }
                StackEntry* temp = intr->the_stack;
                for (int i = 1; i < NUM_VARIABLES; i++) {
                    intr->variables[i] = temp->variables[i];
                }
                current->next = temp->command;
                intr->the_stack = temp->next;
                free(temp);
                break;  
            }
            case CMD_CALL: {
                StackEntry* temp = malloc(sizeof(StackEntry));
                temp->command = current->next;
                for (int i = 0; i < NUM_VARIABLES; i++) {
                    temp->variables[i] = intr->variables[i];
                }
                temp->next = intr->the_stack;
                intr->the_stack = temp;
                Entry* e = get_label(intr->label_map, current->val_a.str_val);
                if (e == NULL) {
                    printf("Label not found: %s\n", current->val_a.str_val);
                    intr->had_error = true;
                    return;
                }
                current->next = e->command;
                break;
            }
            default:
                break;
        }
        current = current->next;
    }
    // Week 4: free the stack at the end
}

void print_interpreter_state(Interpreter *intr) {
    if (!intr) {
        return;
    }

    printf("Error: %d\n", intr->had_error);
    printf("Flags:\n");
    printf("Is greater: %d\n", intr->is_greater);
    printf("Is equal: %d\n", intr->is_equal);
    printf("Is less: %d\n", intr->is_less);

    printf("\n");

    printf("Variable values:\n");
    for (size_t i = 0; i < NUM_VARIABLES; i++) {
        printf("x%zu: %" PRId64 "", i, intr->variables[i]);

        if (i < NUM_VARIABLES - 1) {
            printf(", ");
        }

        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }

    printf("\n");
}

/**
 * @brief Fetches the appropriate value from the given operand.
 *
 * @param intr The pointer to the interpreter holding variable state.
 * @param op The operand used to fetch the value.
 * @param is_im A boolean representing whether this value is an immediate or
 * must be read in from the interpreter state.
 * @return The fetched value.
 */
static int64_t fetch_number_value(Interpreter *intr, Operand *op, bool is_im) {
    // STUDENT TODO: Fetch either a variable from the interpreter's state or directly output a value
    return -1;
}

/**
 * @brief Determines whether a given branch condition holds.
 *
 * @param intr The pointer to the interpreter holding the result of the
 * comparison.
 * @param cond The condition to check.
 * @return True if the given condition holds, false otherwise.
 */
static bool cond_holds(Interpreter *intr, BranchCondition cond) {
    // STUDENT TODO: Determine whether a given condition holds using the interpreter's state
    return false;
}

/**
 * @brief Prints the given command's value in a specified base.
 *
 * @param intr The pointer to the interpreter holding variable state.
 * @param cmd The command being processed.
 * @return True whether the print was successful, false otherwise.
 */
static bool print_base(Interpreter *intr, Command *cmd) {
    // STUDENT TODO: Print the given value respecting the appropriate base
    int64_t temp;
    if (cmd->is_a_immediate)
        temp = cmd->val_a.num_val;
    else 
        temp = intr->variables[cmd->val_a.num_val];
    switch (cmd->val_b.base) {
        case 'd':
            printf("%ld\n", temp);
            return true;
        case 'x':
            printf("0x");
            printf("%lx\n", (uint64_t)temp);
            return true;
        case 'b': {
            printf("0b");
            bool lead = false;
            if (temp < 0) {
                for (int i = 63; i >= 0; i--) {
                    int bit = (temp >> i) & 1;
                    if (bit == 1)
                        lead = true;
                    if (lead)
                        printf("%d", bit);
                }
            }
            else {
                char str[500] = "";
                int i = 0;
                if (temp == i) {
                    str[i] = '0';
                    i++;
                }
                while (temp != 0) {
                    if (temp % 2 == 0) {
                        str[i] = '0';
                    } else {
                        str[i] = '1';
                    }
                    temp /= 2;
                    i++;
                }
                for (int j = i - 1; j >= 0; j--)
                    printf("%c", str[j]);
            }
            printf("\n");
            return true;
        }
        case 's': {
            uint8_t ch;
            while (true) {
                if (!mem_load(&ch, temp, 1)) {
                    intr->had_error = true;
                    return false;
                }
                if (ch == '\0') {
                    break;
                } else
                    putchar(ch);
                temp++;
            }
            printf("\n");
            return true;
        }
        default:
            break;
    }
    printf("\n");
    return false;
}