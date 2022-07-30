// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian, `\x7F`,i.e, `0x7F`, represent delete character.

// File header
struct elfhdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint64 entry; // the entry(a va) of the program
  uint64 phoff; // (the first) program section header offset
  uint64 shoff;
  uint flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum; // program section (header) number
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};

// Program section header
struct proghdr {
  uint32 type; // the value of type is below
  uint32 flags;
  uint64 off;
  uint64 vaddr; // the va that this section will place on
  uint64 paddr;
  uint64 filesz; // the actual size that the section hold and should be loaded
  uint64 memsz; // the size of memory should be allocated, memsz >= filesz
  uint64 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
