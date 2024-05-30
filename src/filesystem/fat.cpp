# include <filesystem/fat.h>


using namespace myos;
using namespace myos::common;
using namespace myos::filesystem;
using namespace myos::drivers;

void printf(char*);
void printfHex(uint8_t);

void myos::filesystem::ReadBiosBlock (myos::drivers::AdvancedTechnologyAttachment *hd, common::uint32_t partitiononOffset) {
    BiosParameterBlock32 bpb;
    //hd->Read28(partitiononOffset, (uint8_t*)&bpb, sizeof(BiosParameterBlock32));
    hd->Read28(partitiononOffset, sizeof(BiosParameterBlock32));

    /* need to know how large the file must be to span multiple clusters */
    /*printf("sectors per cluster: ");
    printfHex(bpb.sectorsPerCluster);
    printf("\n");*/

    /* fat start at the partition offset + the reserved count pointer in the bpb */
    uint32_t fatStart = partitiononOffset + bpb.reservedSectors;

    uint32_t fatSize = bpb.tableSize;
    uint32_t dataStart = fatStart + fatSize*bpb.fatCopies;
    uint32_t rootStart = dataStart + bpb.sectorsPerCluster*(bpb.rootCluster - 2); // minus the offset of 2 which this clusters have 

    DirectoryEntryFat32 dirent[16];

    /* we have space for 16 entries and we read the first sector of the root directory */
    //hd->Read28(rootStart, (uint8_t*)&dirent[0], 16*sizeof(DirectoryEntryFat32));
    hd->Read28(rootStart, 16*sizeof(DirectoryEntryFat32));

    for (int i = 0; i < 16; i++) {
        /* if the name of an entry starts with a backslash 0 then there are no more entries */
        if (dirent[i].name[0] == 0x00)
            break;

        /* illegal value of the attribute file, skip */
        if ((dirent[i].attributes & 0x0F) == 0x0F)  
            continue;

        char* foo = "       \n";
        for (int j = 0; j < 8; j++)
            /* we copy the file name into the buffer and then we print it */
            foo[j] = dirent[i].name[j];

        printf(foo);

        if ((dirent[i].attributes & 0x10) == 0x10) // directory
            continue;

        /* first sector of the file */
        uint32_t firstFileCluster = ((uint32_t)dirent[i].firstCluster) << 16
                                | ((uint32_t)dirent[i].firstClusterLow);

        int32_t SIZE = dirent[i].size;
        int32_t nextFileCluster = firstFileCluster;
        uint8_t buffer[513];
        uint8_t fatBuffer[513];

        while (SIZE > 0)
        {
            /* sector number where the file starts */
            uint32_t fileSector = dataStart + bpb.sectorsPerCluster * (nextFileCluster - 2);
            
            int sectorOffset = 0;
            

            
            for (; SIZE > 0; SIZE -= 512) {
                hd -> Read28(fileSector + sectorOffset, 512); /* we read the first 512 bytes of the file in the first sector */
                // hd -> Read28(fileSector + sectorOffset, buffer, 512);
                /* in the next iteration we read the next sector */
                //sectorOffset++;

                /* if the file is greater than a cluster we break the loop */
                //if (++sectorOffset >= bpb.sectorsPerCluster)
                //    break;

                /* directory entry size modulus SIZE */
                buffer[SIZE > 512 ? 512 : SIZE] = '\0';

                //buffer[dirent[i].size > 512 ? 512 : dirent[i].size] = '\0';
                printf((char*)buffer);


                if (++sectorOffset > bpb.sectorsPerCluster)
                    break;
            }

            uint32_t fatSectorForCurrentCluster = nextFileCluster / (512/sizeof(uint32_t)); // 512 / sizeof(uint32_t) should give the number of entries per sector 

            //hd->Read28(fatStart + fatSectorForCurrentCluster, fatBuffer, 512);
            hd->Read28(fatStart + fatSectorForCurrentCluster, 512);

            uint32_t fatOffsetInSectorForCurrentCluster = nextFileCluster % (512 / sizeof(uint32_t)); // sector number where we find the entry where the next cluster starts 

            /* now we need the get the next cluster so that in the next iteration of the while loop we go the that cluster (in the first sector) */
            nextFileCluster = ((uint32_t*)&fatBuffer)[fatOffsetInSectorForCurrentCluster] & 0x0FFFFFFF; // we need to find out in which sector the entry for the next cluster is in because also the FAT span multiple sectors

            /* FAT32 uses 28 bits to address the clusters, so the & with 0x0FFFFFF helps to get only the bits needed for the address */
        }

        /*uint8_t buffer[512];

        hd->Read28(fileSector, buffer, 512);

        i read the first sector of the file in the buffer, terminate the buffer and then print
        buffer[dirent[i].size] = '\0';
        printf((char*)buffer);*/
    }
}