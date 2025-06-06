struct IamtTableEntry {
    char entry_path[0x5C];
    u32 entry_djb2;
};

struct IamtTableHeader {
    char iamt_magic[0x08];
    u32 ciu_version;
    u32 entries_amount;
    IamtTableEntry entries[entries_amount];
};

IamtTableHeader iamt_table @ 0x0;