# include <filesystem/msdospart.h>
# include <filesystem/fat.h>

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::filesystem;

void printf(char*);
void printfHex(uint8_t);

void MSDOSPartitionTable::ReadPartition(myos::drivers::AdvancedTechnologyAttachment *hd) {

    MasterBootRecord mbr;
    /* read from the harddrive into that */
    /* sector number 0 */
    //hd->Read28(0, (uint8_t*)&mbr, sizeof(MasterBootRecord));
    hd->Read28(0, sizeof(MasterBootRecord));

    /* printf the file system entries */
    printf("MBR: ");
    for (int i = 446; i < 446 + 4*16; i++) {
        printfHex(((uint8_t*)&mbr)[i]);
        printf(" ");
    }
    printf("\n");

    if (mbr.magicnumber != 0xAA55) {
        printf("illegal MBR");
        return;
    }

    /* iterate partition tables */
    for (int i = 0; i < 4; i++) {

        if (mbr.primaryPartition[i].partition_id == 0x00) {
            continue;
        }
        printf("Partition ");
        printfHex(i);
        if (mbr.primaryPartition[i].bootable == 0x80)
            printf("bootable. Type");
        else
            printf("not bootable. Type");

        

        printfHex(mbr.primaryPartition[i].partition_id);

        // pass the offset of the partition to the readbiosblock
        ReadBiosBlock(hd, mbr.primaryPartition[i].start_lba);
    }
};