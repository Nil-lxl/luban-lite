
#ifndef _HGICS_PROCFS_H_
#define _HGICS_PROCFS_H_
#ifdef __RTOS__
struct hgics_procfs {};
#define hgics_create_procfs(hg)
#define hgics_delete_procfs(hg)

#else
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

struct hgics_procfs {
    struct proc_dir_entry *rootdir;
    struct proc_dir_entry *status;
    struct proc_dir_entry *fwevnt;
    struct proc_dir_entry *iwpriv;
    u8                     iwpriv_buf[4096];
    u32                    iwpriv_result;
};

void hgics_create_procfs(struct hgics_wdev *hg);
void hgics_delete_procfs(struct hgics_wdev *hg);

#endif
#endif


