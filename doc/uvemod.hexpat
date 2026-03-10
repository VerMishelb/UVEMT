/*
UVEMod v0.3 hex pattern v1

todo: find the way to force little endian.
*/

import std.io;
import std.mem;

fn getPadding(u8 pad_by, u64 base_addr = 0) {
    return (pad_by - ($-base_addr) % pad_by) % pad_by;
};

bool gLookForNextMod = true;

// Primary header
struct UvemodVersionMinorMajor {
    u16 version_minor [[comment("A change insignificant enough to be correctly handled with older parsers."), name("Minor version")]];
    u16 version_major [[comment("Major version upgrade makes files incompatible with old tools."), name("Major version")]];
};

union UvemodVersion {
    u32 version_u32 [[comment("Integer representation, directly comparable."), name("u32 version")]];
    UvemodVersionMinorMajor;
};

struct UvemodPrimaryHeader {
    char uvemod_identifier[8];
    if (uvemod_identifier != "UVEMod\0\0") { std::print("UVEMod check failed"); gLookForNextMod = false; break; }
    UvemodVersion;
    u32 chunks_amount;
    u64 file_size [[comment("Includes everything, from \"UVEMod\" to the end of the file. Does not include padding.")]];
    u8 alignment;
    u8 reserved[7] [[comment("Must be 0. For possible future use."), color("888888")]];
};

// Metadata
/*
Possible metadata chunks:
auth u32 char[{1}]
mnam u32 char[{1}] *
desc u32 char[{1}]
mver u32 char[{1}]
game u32 char[{1}]
tool u32 char[{1}]
date time_t
pswd u32 [ u32 char[{2}] u32 char[{3}] ]
icon u32 char[4] char[{1}]
trgt u32 char[{1}]
*/

struct MetadataChunk {
    char meta_id[4] [[color("d1ebb1")]];
    
    if (meta_id == "auth") {
        u32 str_size [[color("9174c0")]];
        char author[str_size] [[color("d6b4ff")]];
    }
    else if (meta_id == "mnam") {
        u32 str_size [[color("9174c0")]];
        char mod_name[str_size] [[color("d6b4ff")]];
    }
    else if (meta_id == "desc") {
        u32 str_size [[color("9174c0")]];
        char description[str_size] [[color("d6b4ff")]];
    }
    else if (meta_id == "mver") {
        u32 str_size [[color("9174c0")]];
        char mod_version[str_size] [[color("d6b4ff")]];
    }
    else if (meta_id == "game") {
        u32 str_size [[color("9174c0")]];
        char game_name[str_size] [[color("d6b4ff")]];
    }
    else if (meta_id == "tool") {
        u32 str_size [[color("9174c0")]];
        char tool_name[str_size] [[color("d6b4ff")]];
    }
    else if (meta_id == "date") {
        s64 created_timestamp;
    }
    else if (meta_id == "pswd") {
        u32 chunk_data_size;
        if (chunk_data_size != 0) {
            u32 algo_str_size;
            char algo_str[algo_str_size];
            u32 hash_size;
            char hash[hash_size];
        }
    }
    else if (meta_id == "icon") {
        char icon_format[4];
        u32 icon_file_size;
        char icon_data[icon_file_size];
    }
    else if (meta_id == "trgt") {
        u32 str_size;
        char target_file_name[str_size] [[color("d6b4ff")]];
    }
    else { u32 assumed_size; padding[assumed_size]; }
    
    // Align by 4.
    // This is cursed, there must be a better way.
    // The tripple parent thing is also cursed but we can't simply make it global
    // because there might be several mod files in one.
    if (parent.parent.parent.header_primary.alignment != 0) {
        padding[getPadding(4, parent.parent.parent.mod_start_ptr)];
    }
};

struct Metadata {
    u32 metadata_entries_amount;
    MetadataChunk chunks[metadata_entries_amount];
};

// Entries
bitfield EntryFlags {
    SKIP_IF_MISSING : 1;
    SKIP_IF_EXISTS : 1;
    padding : 2;
    USER_FLAGS : 4;
};

struct Entry {
    u64 file_ptr [[color("ff0000")]];// + parent.parent.mod_start_ptr!!!
    u64 file_size [[color("ff6000")]];
    if (file_ptr + file_size < sizeof($) && file_ptr > 0) {
        char file_data[file_size] @ file_ptr + parent.parent.mod_start_ptr;
    }
    EntryFlags flags [[color("cccc00")]];
    u8 path_string_size [[color("00dd00")]];
    char path_string[path_string_size] [[color("6087c7")]];
    if (parent.parent.header_primary.alignment != 0) {
        padding[getPadding(4, parent.parent.mod_start_ptr)];
    }
};

// Mod file
enum MOD_TYPE : u8 {
    MOD = 0,
    PART = 1
};

/*
Possible chunks:
meta u32 u64* *
tool u32 char[{1}]
cksm u32 [ u32 char[{2}] u32 char[{3}] ]
entr u32 u64* *
type u8 [ u8 u16 char[{4}] ]
*/

struct UvemodChunk {
    char chunk_id[4] [[color("cccc00")]];
    u32 chunk_size [[color("aa7700")]];
    
    if (chunk_id == "meta") {
        u64 meta_ptr;
        Metadata metadata @ meta_ptr + parent.mod_start_ptr;
    }
    else if (chunk_id == "tool") {
        char tool_name[chunk_size];
    } 
    else if (chunk_id == "entr") {
        u32 entries_amount;
        u64 entries_ptr;
        Entry entries[entries_amount] @ entries_ptr + parent.mod_start_ptr;
    }
    else if (chunk_id == "cksm") {
        if (chunk_size != 0) {
            u32 algo_str_size;
            char algo_str[algo_str_size];
            u32 hash_size;
            char hash[hash_size];
        }
    }
    else if (chunk_id == "type") {
        MOD_TYPE mod_type;
        if (chunk_size > 1) {
            if (mod_type == MOD_TYPE::MOD) {
                u8 total_parts;
            }
            else if (mod_type == MOD_TYPE::PART) {
                u8 part_number;
            }
            u16 group_id_size;
            char group_id[group_id_size];
        }
    }
    else { padding[chunk_size]; }
    if (parent.header_primary.alignment != 0) {
        padding[getPadding(4, parent.mod_start_ptr)];
    }
};

struct Uvemod {
    std::print("UVEMod check at {:#x}", $);
    u64 mod_start_ptr = $;
    
    UvemodPrimaryHeader header_primary;
    UvemodChunk chunk[header_primary.chunks_amount];
    
    $ = mod_start_ptr + header_primary.file_size;
    if (header_primary.alignment != 0 && $ < sizeof($)-4){
        padding[getPadding(4, mod_start_ptr)];
    }
};

// Don't check if the next uvemod exists if there is not enough bytes even for the next format id.
// If we don't -7, the pattern will crash on read.
Uvemod uvemod[while ($ < sizeof($)-7)] @ 0x00;