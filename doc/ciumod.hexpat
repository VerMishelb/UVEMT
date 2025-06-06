#pragma magic [ 49 41 4D 46 ] @ 0x4
struct IamtModFileEntry {
    char file_path[88];
    u32 file_offset;
    u32 file_size;
};

struct IamtModHeader {
    char game[0x04];
    char iamf_magic[0x04];
    u32 mod_version;
    u32 entries_amount;
    char mod_name[80];
    char mod_description[256];
    IamtModFileEntry entries[entries_amount];
    // u32 crc @ 0x4;
    // I do not know how to force it to read it at the last 4 bytes.
};

IamtModHeader iamt_table @ 0x0;