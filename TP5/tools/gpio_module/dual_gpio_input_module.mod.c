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
	{ 0xdcb764ad, "memset" },
	{ 0x68cc1b47, "gpiod_get_value" },
	{ 0x96848186, "scnprintf" },
	{ 0x92997ed8, "_printk" },
	{ 0x619cb7dd, "simple_read_from_buffer" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x2be25b67, "gpio_device_find_by_label" },
	{ 0x55a5605b, "gpio_device_get_chip" },
	{ 0xae54ffa6, "gpio_device_put" },
	{ 0xad8b34e2, "gpiochip_request_own_desc" },
	{ 0x161f1916, "gpiochip_free_own_desc" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xa01f13a6, "cdev_init" },
	{ 0x3a6d85d3, "cdev_add" },
	{ 0x59c02473, "class_create" },
	{ 0x26961511, "device_create" },
	{ 0x6775d5d3, "class_destroy" },
	{ 0x27271c6b, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xe8b509ca, "device_destroy" },
	{ 0xf33b3302, "noop_llseek" },
	{ 0x305a49b8, "param_ops_charp" },
	{ 0xdf42cd46, "param_ops_int" },
	{ 0x474e54d2, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "15A01BF23E718D2448902E0");
