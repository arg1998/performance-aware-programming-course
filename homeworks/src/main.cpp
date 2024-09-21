#define _CRT_SECURE_NO_WARNINGS // to ignore deprication warnings 

#include <cstdio>
#include <cstdlib>

#define DEBUG 1

#if (DEBUG == 1)
#define printd(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define printd(fmt, ...)
#endif

#define clear_console() printf("\e[1;1H\e[2J")
#define REG_NAME(name) (char*)name  // to get rid of ISO C++ 11 complainign about string should not be converted to char * implicitly :)
#define IS_BIT_SET(bitfield, mask) (bitfield & mask) == mask

typedef unsigned char u8;    // 1 byte
typedef unsigned short u16;  // 2 bytes
typedef unsigned int u32;    // 4 bytes

typedef signed char i8;    // 1 byte
typedef signed short i16;  // 2 bytes
typedef signed int i32;    // 4 bytes

enum inst_mask {
    BIT_MASK_OPCODE = 0b11111100,
    BIT_MASK_DIRECTION = 0b00000010,
    BIT_MASK_WORD = 0b00000001,
    BIT_MASK_MOD = 0b11000000,
    BIT_MASK_REG = 0b00111000,
    BIT_MASK_RM = 0b00000111
};

enum register_part {
    REG_SECTION_NULL = '\0',
    REG_SECTION_X = 'x',
    REG_SECTION_LOW = 'l',
    REG_SECTION_HIGH = 'h',
};

enum inst_opcode {
    INST_MOV = 0b100010 << 2  // register-to-register mov
};

typedef struct {
    char* name;
    union {
        u16 x;
        struct {
            u8 low;
            u8 high;
        };
    };
} register_16b;


void debug_print_register(const register_16b* reg);

struct processor {
    char* name;

    // W == 0 -> byte operation
    // W == 1 -> word (2 bytes) operation
    register_16b reg_A{.name = REG_NAME("a"), .x = 0x00};  // 00
    register_16b reg_C{.name = REG_NAME("c"), .x = 0x00};  // 01
    register_16b reg_D{.name = REG_NAME("d"), .x = 0x00};  // 10
    register_16b reg_B{.name = REG_NAME("b"), .x = 0x00};  // 11

    // Only for W == 1 becuase of word-size operation
    register_16b reg_SP{.name = REG_NAME("sp"), .x = 0x00};  // 00
    register_16b reg_BP{.name = REG_NAME("bp"), .x = 0x00};  // 01
    register_16b reg_SI{.name = REG_NAME("si"), .x = 0x00};  // 10
    register_16b reg_DI{.name = REG_NAME("di"), .x = 0x00};  // 11

    void debug_print(){
        printd("\nProcessor: %s\n", this->name);
        debug_print_register(&this->reg_A);
        debug_print_register(&this->reg_B);
        debug_print_register(&this->reg_C);
        debug_print_register(&this->reg_D);

        debug_print_register(&this->reg_SP);
        debug_print_register(&this->reg_BP);
        debug_print_register(&this->reg_SI);
        debug_print_register(&this->reg_DI);
        printd("\n\n");

    }
};

register_part get_register(processor* p, u8 DW, u8 reg_index, register_16b** out_register) {
    // check if the W bit is set
    bool is_word_size = IS_BIT_SET(DW, BIT_MASK_WORD);
    bool is_reg_destination = IS_BIT_SET(DW, BIT_MASK_DIRECTION);

    if ((reg_index & BIT_MASK_REG) > 0) {
        // the indexing table for REG and R/M fields are exactly the same.
        // REG values are shifted by 3 bits. We want register indecis to be the lowest 3 bits of the register adderess.
        // R/M values are in the first 3 bits.
        reg_index = reg_index >> 3;
    }

    // TODO: Im sure there is another way of coding this logic.
    register_part section;
    if (is_word_size) {
        section = REG_SECTION_X;
        switch (reg_index) {
            case 0b000:
                *out_register = &p->reg_A;
                break;
            case 0b001:
                *out_register = &p->reg_C;
                break;
            case 0b010:
                *out_register = &p->reg_D;
                break;
            case 0b011:
                *out_register = &p->reg_B;
                break;
            case 0b100:
                *out_register = &p->reg_SP;
                section = REG_SECTION_NULL;
                break;
            case 0b101:
                *out_register = &p->reg_BP;
                section = REG_SECTION_NULL;
                break;
            case 0b110:
                *out_register = &p->reg_SI;
                section = REG_SECTION_NULL;
                break;
            case 0b111:
                *out_register = &p->reg_DI;
                section = REG_SECTION_NULL;
                break;
        }

    } else {
        switch (reg_index) {
            case 0b000:
                *out_register = &p->reg_A;
                section = REG_SECTION_LOW;
                break;
            case 0b001:
                *out_register = &p->reg_C;
                section = REG_SECTION_LOW;
                break;
            case 0b010:
                *out_register = &p->reg_D;
                section = REG_SECTION_LOW;
                break;
            case 0b011:
                *out_register = &p->reg_B;
                section = REG_SECTION_LOW;
                break;
            case 0b100:
                *out_register = &p->reg_A;
                section = REG_SECTION_HIGH;
                break;
            case 0b101:
                *out_register = &p->reg_C;
                section = REG_SECTION_HIGH;
                break;
            case 0b110:
                *out_register = &p->reg_D;
                section = REG_SECTION_HIGH;
                break;
            case 0b111:
                *out_register = &p->reg_B;
                section = REG_SECTION_HIGH;
                break;
        }
    }

    return section;
}

void decode_instructions(processor* p, u8* inst_stream, i32 size) {
    if (size < 1) {
        printf("Invalis instruction size of %d. ABORTING...\n", size);
        return;
    }
    i32 inst_counter = 0;  // counting the instructions
    i32 byte_cursor = 0;   // indexing each byte in the instruction stream
    while (byte_cursor < size) {
        u8 inst_byte = inst_stream[byte_cursor];
        u8 opcode = BIT_MASK_OPCODE & inst_byte;
        // printf("processing byte 0x%02x\n", inst_byte);
        // printf("opcode found 0x%02x\n", opcode);

        // TODO: Currently we only decode for MOD=11 which means only register-to-register
        switch (opcode) {
            case INST_MOV: {
                u8 second_byte = inst_stream[++byte_cursor];
                u8 DW = inst_byte & (BIT_MASK_WORD | BIT_MASK_DIRECTION);

                register_16b* reg_src;
                register_16b* reg_dest;
                register_part dest_section;
                register_part src_section;

                if (IS_BIT_SET(inst_byte, BIT_MASK_DIRECTION)) {
                    // if D=1 then REG field is destination

                    // get destination register from REG field
                    dest_section = get_register(p, DW, second_byte & BIT_MASK_REG, &reg_dest);
                    // get source register from R/M field
                    src_section = get_register(p, DW, second_byte & BIT_MASK_RM, &reg_src);
                } else {
                    // if D=0 then REG field is source

                    // get source register from REG field
                    src_section = get_register(p, DW, second_byte & BIT_MASK_REG, &reg_src);
                    // get destination register from R/M field
                    dest_section = get_register(p, DW, second_byte & BIT_MASK_RM, &reg_dest);
                }

                printf("mov %s%c, %s%c\n", reg_dest->name, (char)dest_section, reg_src->name, (char)src_section);


                // executing the instruction inside the processor
                if (dest_section == REG_SECTION_X || dest_section == REG_SECTION_NULL) {
                    // word size operation
                    // [16 bit] <- [16 bit]
                    reg_dest->x = reg_src->x;
                } else if (dest_section == REG_SECTION_LOW && src_section == REG_SECTION_LOW) {
                    // byte operation
                    // [8 bit low] <- [8 bit low]
                    reg_dest->low = reg_src->low;
                } else if (dest_section == REG_SECTION_LOW && src_section == REG_SECTION_HIGH) {
                    // byte operation
                    // [8 bit low] <- [8 bit high]
                    reg_dest->low = reg_src->high;
                } else if (dest_section == REG_SECTION_HIGH && src_section == REG_SECTION_LOW) {
                    // byte operation
                    // [8 bit high] <- [8 bit low]
                    reg_dest->high = reg_src->low;
                } else if (dest_section == REG_SECTION_HIGH && src_section == REG_SECTION_HIGH) {
                    // byte operation
                    // [8 bit high] <- [8 bit high]
                    reg_dest->high = reg_src->high;
                }

            } break;
            default:
                break;
        }
        byte_cursor++;
    }

    return;
}

void debug_print_register(const register_16b* reg) {
    printd("REG: %s\t\tx:%02x\t\tlow:%02x\t\thigh:%02x\n", reg->name, reg->x, reg->low, reg->high);
}

void debug_verify_type_sizes() {
    printd("Singed Data Types:    i8(%d bits), i16(%d bits), i32(%d bits)\n", sizeof(i8) * 8, sizeof(i16) * 8, sizeof(i32) * 8);
    printd("Unsinged Data Types:  u8(%d bits), u16(%d bits), u32(%d bits)\n", sizeof(u8) * 8, sizeof(u16) * 8, sizeof(u32) * 8);
}

/// @brief Read an binary file
/// @param path Path of the binary file
/// @param data_out output buffer where the content of the file is stored. This argument is allocated on the heap using malloc.
/// @return Numebr of bytes read from the file. If returen value is negative, file was not read
i32 read_asm_binary_file(const char* path, u8** data_out) {
    FILE* file_ptr = NULL;
    i32 file_size = 0;  // in bytes

    file_ptr = fopen(path, "rb");
    if (file_ptr == NULL) {
        printf("Could not read file: %s\n", path);
        return -1;
    }
    fseek(file_ptr, 0, SEEK_END);
    file_size = ftell(file_ptr);
    rewind(file_ptr);

    *data_out = (u8*)malloc(file_size);
    if (*data_out == NULL) {
        printf("Memory allocation failed\n");
        fclose(file_ptr);
        return -1;
    }

    fread(*data_out, 1, file_size, file_ptr);
    fclose(file_ptr);
    return file_size;
}

void debug_print_bytes(i32 size, u8* data) {
    if (size > 0) {
        printd("%d bytes read: ", size);
        for (int i = 0; i < size; i++) {
            printd("%02x ", data[i]);
        }
        printd("\n");
    }
}

int main(int argc, char** argv) {
    clear_console();

    debug_verify_type_sizes();

    u8* l037_inst;
    i32 count;

    count = read_asm_binary_file("../homeworks/assignments/part1/listing_0038_many_register_mov", &l037_inst);
    debug_print_bytes(count, l037_inst);

    processor x8086{.name = (char*)"Intel x8086"};
    register_16b* reg = &x8086.reg_B;

    reg->x = 0xFFee;
    debug_print_register(reg);

    reg->high = 0x99;
    debug_print_register(reg);

    reg->low = 0x11;
    debug_print_register(reg);



    printf("\n===============================================================\n");
    printf("                      DECODING INSTRUCTION\n");
    printf("===============================================================\n\n");

    decode_instructions(&x8086, l037_inst, count);
    x8086.debug_print();

    return 0;
}