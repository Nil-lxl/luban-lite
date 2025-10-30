#ifndef __MAC80211_DEBUGFS_H
#define __MAC80211_DEBUGFS_H

#ifdef CONFIG_MAC80211_DEBUGFS
void debugfs_hw_add(struct ieee80211_local *local);
int __printf(4, 5) mac80211_format_buffer(char __user *userbuf, size_t count,
					  loff_t *ppos, char *fmt, ...);
#else
#define debugfs_hw_add(...)
#define cfg80211_debugfs_rdev_add(...)
#define debugfs_remove_recursive(...)
#endif

#endif /* __MAC80211_DEBUGFS_H */
