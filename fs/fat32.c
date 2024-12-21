#include "fat32.h"
#include "printk.h"
#include "virtio.h"
#include "string.h"
#include "mbr.h"
#include "mm.h"

#define min(a, b) ((a) < (b)? (a) : (b))

struct fat32_bpb fat32_header;
struct fat32_volume fat32_volume;

uint8_t fat32_buf[VIRTIO_BLK_SECTOR_SIZE];
uint8_t fat32_table_buf[VIRTIO_BLK_SECTOR_SIZE];

uint64_t cluster_to_sector(uint64_t cluster) {
    return (cluster - 2) * fat32_volume.sec_per_cluster + fat32_volume.first_data_sec;
}

uint64_t sector_to_cluster(uint64_t sector) {
    return (sector - fat32_volume.first_data_sec) / fat32_volume.sec_per_cluster + 2;
}

uint32_t next_cluster(uint64_t cluster) {
    uint64_t fat_offset = cluster * 4;
    uint64_t fat_sector = fat32_volume.first_fat_sec + fat_offset / VIRTIO_BLK_SECTOR_SIZE;
    virtio_blk_read_sector(fat_sector, fat32_table_buf);
    int index_in_sector = fat_offset % (VIRTIO_BLK_SECTOR_SIZE / sizeof(uint32_t));
    return *(uint32_t*)(fat32_table_buf + index_in_sector);
}

void fat32_init(uint64_t lba, uint64_t size) {
    virtio_blk_read_sector(lba, (void*)&fat32_header);
    fat32_volume.first_fat_sec = lba + fat32_header.rsvd_sec_cnt;
    fat32_volume.sec_per_cluster = fat32_header.sec_per_clus;
    fat32_volume.first_data_sec = fat32_volume.first_fat_sec + fat32_header.num_fats * fat32_header.fat_sz32;
    fat32_volume.fat_sz = fat32_header.fat_sz32;
    virtio_blk_read_sector(fat32_volume.first_fat_sec, (void*)fat32_buf);
    // expect: "F8 FF FF 0F"
    for (int i = 0; i < 4; i++)
        printk("%x ", fat32_buf[i]);
    printk("\nLBA: 0x%llx\nFIRST_FAT: 0x%llx\nFIRST_DATA: 0x%llx\n", lba*512, fat32_volume.first_fat_sec*512, fat32_volume.first_data_sec*512);
}

int is_fat32(uint64_t lba) {
    virtio_blk_read_sector(lba, (void*)&fat32_header);
    if (fat32_header.boot_sector_signature != 0xaa55) {
        return 0;
    }
    return 1;
}

int next_slash(const char* path) {  // util function to be used in fat32_open_file
    int i = 0;
    while (path[i] != '\0' && path[i] != '/') {
        i++;
    }
    if (path[i] == '\0') {
        return -1;
    }
    return i;
}

void to_upper_case(char *str) {     // util function to be used in fat32_open_file
    int i;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 32;
        }
    }
    for (; i < 8; i++) // spaces is used for fat32 name
        str[i] = ' ';
}

struct fat32_file fat32_open_file(const char *path) {
    struct fat32_file file;
    memset(&file, 0, sizeof(struct fat32_file));
    // judge if path is started with '/fat32/'
    uint64_t first_slash = next_slash(path);
    uint64_t second_slash = first_slash + 1 + next_slash(path + first_slash + 1);
    printk("first_slash: %d, second_slash: %d\n", first_slash, second_slash);
    if (memcmp((path + first_slash + 1), "fat32", second_slash - first_slash - 1) != 0)
        printk("Not a valid fat32 path\n");
    // get the file name
    char file_name[9]; // ignore the extension name
    memset(file_name, 0, 9);
    for (int i = 0; *(path + second_slash + 1 + i) != '\0' && i < 8; i++) {
        file_name[i] = *(path + second_slash + 1 + i);
    }
    to_upper_case(file_name);
    printk("file_name: %s\n", file_name);
    // see the root cluster
    uint64_t root_cluster = sector_to_cluster(fat32_volume.first_data_sec);
    virtio_blk_read_sector(cluster_to_sector(root_cluster), (void*)fat32_buf);
    for (int i = 0; i < VIRTIO_BLK_SECTOR_SIZE / sizeof(struct fat32_dir_entry); i++) {
        struct fat32_dir_entry* current_dir_entry = (struct fat32_dir_entry*)(fat32_buf + i * sizeof(struct fat32_dir_entry));
        if (memcmp(current_dir_entry->name, file_name, 8) != 0) continue;
        printk("found file at index %d\n", i);
        file.dir.cluster = root_cluster;
        file.dir.index = i;
        file.cluster = (uint32_t)current_dir_entry->startlow & 0x0000FFFF | ((uint32_t)current_dir_entry->starthi << 16) & 0xFFFF0000;
        break;
    }
    printk("cluster: 0x%x\n", file.cluster);
    return file;
}

int64_t fat32_lseek(struct file* file, int64_t offset, uint64_t whence) {
    if (whence == SEEK_SET) {
        file->cfo = offset;
        printk("fat32_lseek: SEEK_SET to %lld\n", offset);
    } else if (whence == SEEK_CUR) {
        file->cfo = file->cfo + offset;
    } else if (whence == SEEK_END) {
        /* Calculate file length */
        // read the directory entry of the file
        uint64_t file_length = 0;
        uint32_t cluster = file->fat32_file.dir.cluster;
        virtio_blk_read_sector(cluster_to_sector(cluster), (void*)fat32_buf);
        // get the file size
        struct fat32_dir_entry* current_dir_entry = (struct fat32_dir_entry*)(fat32_buf + file->fat32_file.dir.index * sizeof(struct fat32_dir_entry));
        file->cfo = current_dir_entry->size;
    } else {
        printk("fat32_lseek: whence not implemented\n");
        while (1);
    }
    return file->cfo;
}

uint64_t fat32_table_sector_of_cluster(uint32_t cluster) {
    return fat32_volume.first_fat_sec + cluster / (VIRTIO_BLK_SECTOR_SIZE / sizeof(uint32_t));
}

int64_t fat32_read(struct file* file, void* buf, uint64_t len) {
    // read the directory entry of the file
    uint64_t file_length = 0;
    uint32_t cluster = file->fat32_file.dir.cluster;
    virtio_blk_read_sector(cluster_to_sector(cluster), (void*)fat32_buf);
    // get the file size
    struct fat32_dir_entry* current_dir_entry = (struct fat32_dir_entry*)(fat32_buf + file->fat32_file.dir.index * sizeof(struct fat32_dir_entry));
    file_length = current_dir_entry->size;
    uint64_t cluster_size = fat32_volume.sec_per_cluster * VIRTIO_BLK_SECTOR_SIZE;
    uint64_t current_cluster = file->fat32_file.cluster;
    for (uint64_t i = 0; i < file->cfo / cluster_size; i++)
        current_cluster = next_cluster(current_cluster);
    virtio_blk_read_sector(cluster_to_sector(current_cluster), (void*)fat32_buf);
    uint64_t ret = 0;
    for (int i = 0; file -> cfo < file_length && i < min(len, cluster_size); i++, file -> cfo++, ret++) {
        ((char*)buf)[i] = fat32_buf[file->cfo % cluster_size];
        if (file->cfo % cluster_size == cluster_size - 1) { // reach the end of the cluster
            current_cluster = next_cluster(current_cluster);
            virtio_blk_read_sector(cluster_to_sector(current_cluster), (void*)fat32_buf);
        }
    }
    Log("fat32_read: expected %lld bytes, read %lld bytes\n", len, ret);
    return ret;
}

int64_t fat32_write(struct file* file, const void* buf, uint64_t len) {
    // read the directory entry of the file
    uint64_t file_length = 0;
    uint32_t cluster = file->fat32_file.dir.cluster;
    virtio_blk_read_sector(cluster_to_sector(cluster), (void*)fat32_buf);
    // get the file size
    struct fat32_dir_entry* current_dir_entry = (struct fat32_dir_entry*)(fat32_buf + file->fat32_file.dir.index * sizeof(struct fat32_dir_entry));
    file_length = current_dir_entry->size;
    uint64_t cluster_size = fat32_volume.sec_per_cluster * VIRTIO_BLK_SECTOR_SIZE;
    uint64_t current_cluster = file->fat32_file.cluster;
    for (uint64_t i = 0; i < file->cfo / cluster_size; i++)
        current_cluster = next_cluster(current_cluster);
    virtio_blk_read_sector(cluster_to_sector(current_cluster), (void*)fat32_buf);
    uint64_t ret = 0;
    for (int i = 0; file -> cfo < file_length && i < min(len, cluster_size); i++, file -> cfo++, ret++) {
        fat32_buf[file->cfo % cluster_size] = ((char*)buf)[i];
        if (file->cfo % cluster_size == cluster_size - 1) { // reach the end of the cluster
            virtio_blk_write_sector(cluster_to_sector(current_cluster), (void*)fat32_buf);
            current_cluster = next_cluster(current_cluster);
            virtio_blk_read_sector(cluster_to_sector(current_cluster), (void*)fat32_buf);
        }
    }
    virtio_blk_write_sector(cluster_to_sector(current_cluster), (void*)fat32_buf);
    Log("fat32_write: expected %lld bytes, write %lld bytes\n", len, ret);
    return ret;
}