# FAT32-Reader
Reader for files formatted in FAT32. It is implemented in C. You just need to tell to the SO to use this File System. For example,
you can format a USB memory to FAT32 and tell the SO that new partition (USB) will be managed by this FAT32 Reader. It is important
to remember that this file system is only for reading, you need to create/write the files into the partition using a writer for 
FAT32 files.
