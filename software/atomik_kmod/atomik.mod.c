#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x6bb013d4, "pin_user_pages_fast" },
	{ 0x1cae9a42, "filp_open" },
	{ 0x7e2232fb, "ioread32" },
	{ 0xa61fd7aa, "__check_object_size" },
	{ 0xd5ad82a1, "misc_deregister" },
	{ 0x5244a5dc, "idr_find" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0x79fd4f39, "devm_kmalloc" },
	{ 0xd272d446, "__rcu_read_lock" },
	{ 0xfad8f384, "iowrite32" },
	{ 0xd710adbf, "__kmalloc_noprof" },
	{ 0x4695bf9b, "platform_driver_unregister" },
	{ 0x40a621c5, "snprintf" },
	{ 0xa1dacb42, "class_destroy" },
	{ 0x40a621c5, "scnprintf" },
	{ 0xcbee6320, "unpin_user_pages" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xe1e1f979, "_raw_spin_lock_irqsave" },
	{ 0x5ae9ee26, "__per_cpu_offset" },
	{ 0xd272d446, "__fentry__" },
	{ 0xdd6830c7, "sysfs_emit" },
	{ 0x026921de, "crypto_destroy_tfm" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x9479a1e8, "strnlen" },
	{ 0x38c8be28, "idr_get_next" },
	{ 0x296b9459, "strrchr" },
	{ 0x9b1de7cb, "_dev_info" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0xbd03ed67, "page_offset_base" },
	{ 0xd70733be, "sized_strscpy" },
	{ 0x684812dd, "__bitmap_set" },
	{ 0x197474e5, "compat_ptr_ioctl" },
	{ 0x44decd6f, "hugetlb_optimize_vmemmap_key" },
	{ 0x07d50c57, "idr_remove" },
	{ 0xf0d1e02d, "crypto_shash_setkey" },
	{ 0xb82edfb3, "idr_alloc" },
	{ 0xd272d446, "__rcu_read_unlock" },
	{ 0xb8193352, "platform_get_resource" },
	{ 0x653aa194, "class_create" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0xf46d5bf3, "mutex_lock" },
	{ 0x2435d559, "strncmp" },
	{ 0x1418bda3, "unregister_kretprobe" },
	{ 0x2719b9fa, "const_current_task" },
	{ 0x5735319c, "crypto_shash_digest" },
	{ 0x86632fd6, "_find_next_bit" },
	{ 0xc1e6c71e, "__mutex_init" },
	{ 0xe54e0a6b, "__fortify_panic" },
	{ 0xc3a36f61, "bitmap_find_next_zero_area_off" },
	{ 0x81a1a811, "_raw_spin_unlock_irqrestore" },
	{ 0x255dfd5a, "idr_destroy" },
	{ 0x5373d78a, "kstrtobool" },
	{ 0xb5c51982, "__cpu_possible_mask" },
	{ 0x27683a56, "memset" },
	{ 0x9b1de7cb, "_dev_warn" },
	{ 0xaca12394, "misc_register" },
	{ 0x4b5cc7c5, "kernel_read" },
	{ 0x0040afbe, "param_ops_charp" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0xf296206e, "nr_cpu_ids" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0x4a2b4431, "__crypto_memneq" },
	{ 0x888b8f57, "strcmp" },
	{ 0x223cc85c, "__platform_driver_register" },
	{ 0x7a6661ca, "ktime_get_real_seconds" },
	{ 0x684812dd, "__bitmap_clear" },
	{ 0x8bc8f40f, "crc32c" },
	{ 0x3cc75249, "device_create_with_groups" },
	{ 0xbd03ed67, "vmemmap_base" },
	{ 0xf46d5bf3, "mutex_unlock" },
	{ 0xd0bc10a2, "devm_ioremap_resource" },
	{ 0x9d52f437, "filp_close" },
	{ 0x7851be11, "__get_user_4" },
	{ 0x1595e410, "device_destroy" },
	{ 0xc064623f, "__kmalloc_cache_noprof" },
	{ 0x546c19d9, "validate_usercopy_range" },
	{ 0x23b00505, "cgroup_path_ns" },
	{ 0x1c489eb6, "register_kprobe" },
	{ 0x7c898386, "register_kretprobe" },
	{ 0x7a8e92c6, "unregister_kprobe" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0x43a349ca, "strlen" },
	{ 0xf1de9e85, "kvfree" },
	{ 0x57cf3421, "crypto_alloc_shash" },
	{ 0xf52f8b44, "__kvmalloc_node_noprof" },
	{ 0xfaabfe5e, "kmalloc_caches" },
	{ 0xdbb4ec87, "kernel_write" },
	{ 0xbebe66ff, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x6bb013d4,
	0x1cae9a42,
	0x7e2232fb,
	0xa61fd7aa,
	0xd5ad82a1,
	0x5244a5dc,
	0x092a35a2,
	0x79fd4f39,
	0xd272d446,
	0xfad8f384,
	0xd710adbf,
	0x4695bf9b,
	0x40a621c5,
	0xa1dacb42,
	0x40a621c5,
	0xcbee6320,
	0xcb8b6ec6,
	0xe1e1f979,
	0x5ae9ee26,
	0xd272d446,
	0xdd6830c7,
	0x026921de,
	0xe8213e80,
	0xbd03ed67,
	0xd272d446,
	0x9479a1e8,
	0x38c8be28,
	0x296b9459,
	0x9b1de7cb,
	0x90a48d82,
	0xbd03ed67,
	0xd70733be,
	0x684812dd,
	0x197474e5,
	0x44decd6f,
	0x07d50c57,
	0xf0d1e02d,
	0xb82edfb3,
	0xd272d446,
	0xb8193352,
	0x653aa194,
	0xbd03ed67,
	0xf46d5bf3,
	0x2435d559,
	0x1418bda3,
	0x2719b9fa,
	0x5735319c,
	0x86632fd6,
	0xc1e6c71e,
	0xe54e0a6b,
	0xc3a36f61,
	0x81a1a811,
	0x255dfd5a,
	0x5373d78a,
	0xb5c51982,
	0x27683a56,
	0x9b1de7cb,
	0xaca12394,
	0x4b5cc7c5,
	0x0040afbe,
	0xd272d446,
	0xf296206e,
	0x092a35a2,
	0x4a2b4431,
	0x888b8f57,
	0x223cc85c,
	0x7a6661ca,
	0x684812dd,
	0x8bc8f40f,
	0x3cc75249,
	0xbd03ed67,
	0xf46d5bf3,
	0xd0bc10a2,
	0x9d52f437,
	0x7851be11,
	0x1595e410,
	0xc064623f,
	0x546c19d9,
	0x23b00505,
	0x1c489eb6,
	0x7c898386,
	0x7a8e92c6,
	0xe4de56b4,
	0x43a349ca,
	0xf1de9e85,
	0x57cf3421,
	0xf52f8b44,
	0xfaabfe5e,
	0xdbb4ec87,
	0xbebe66ff,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"pin_user_pages_fast\0"
	"filp_open\0"
	"ioread32\0"
	"__check_object_size\0"
	"misc_deregister\0"
	"idr_find\0"
	"_copy_from_user\0"
	"devm_kmalloc\0"
	"__rcu_read_lock\0"
	"iowrite32\0"
	"__kmalloc_noprof\0"
	"platform_driver_unregister\0"
	"snprintf\0"
	"class_destroy\0"
	"scnprintf\0"
	"unpin_user_pages\0"
	"kfree\0"
	"_raw_spin_lock_irqsave\0"
	"__per_cpu_offset\0"
	"__fentry__\0"
	"sysfs_emit\0"
	"crypto_destroy_tfm\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"__stack_chk_fail\0"
	"strnlen\0"
	"idr_get_next\0"
	"strrchr\0"
	"_dev_info\0"
	"__ubsan_handle_out_of_bounds\0"
	"page_offset_base\0"
	"sized_strscpy\0"
	"__bitmap_set\0"
	"compat_ptr_ioctl\0"
	"hugetlb_optimize_vmemmap_key\0"
	"idr_remove\0"
	"crypto_shash_setkey\0"
	"idr_alloc\0"
	"__rcu_read_unlock\0"
	"platform_get_resource\0"
	"class_create\0"
	"random_kmalloc_seed\0"
	"mutex_lock\0"
	"strncmp\0"
	"unregister_kretprobe\0"
	"const_current_task\0"
	"crypto_shash_digest\0"
	"_find_next_bit\0"
	"__mutex_init\0"
	"__fortify_panic\0"
	"bitmap_find_next_zero_area_off\0"
	"_raw_spin_unlock_irqrestore\0"
	"idr_destroy\0"
	"kstrtobool\0"
	"__cpu_possible_mask\0"
	"memset\0"
	"_dev_warn\0"
	"misc_register\0"
	"kernel_read\0"
	"param_ops_charp\0"
	"__x86_return_thunk\0"
	"nr_cpu_ids\0"
	"_copy_to_user\0"
	"__crypto_memneq\0"
	"strcmp\0"
	"__platform_driver_register\0"
	"ktime_get_real_seconds\0"
	"__bitmap_clear\0"
	"crc32c\0"
	"device_create_with_groups\0"
	"vmemmap_base\0"
	"mutex_unlock\0"
	"devm_ioremap_resource\0"
	"filp_close\0"
	"__get_user_4\0"
	"device_destroy\0"
	"__kmalloc_cache_noprof\0"
	"validate_usercopy_range\0"
	"cgroup_path_ns\0"
	"register_kprobe\0"
	"register_kretprobe\0"
	"unregister_kprobe\0"
	"__ubsan_handle_load_invalid_value\0"
	"strlen\0"
	"kvfree\0"
	"crypto_alloc_shash\0"
	"__kvmalloc_node_noprof\0"
	"kmalloc_caches\0"
	"kernel_write\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Catomik,delta-engine");
MODULE_ALIAS("of:N*T*Catomik,delta-engineC*");

MODULE_INFO(srcversion, "16849124BF50ABEDF720F7C");
