#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "tp5_gpio"
#define CLASS_NAME "dual_gpio"

static int gpio_a = 17;
static int gpio_b = 27;
static char *gpio_chip_label = "pinctrl-rp1";
static dev_t dual_gpio_devno;
static struct cdev dual_gpio_cdev;
static struct class *dual_gpio_class;
static struct device *dual_gpio_device;
static struct gpio_desc *gpio_a_desc;
static struct gpio_desc *gpio_b_desc;

module_param(gpio_a, int, 0444);
MODULE_PARM_DESC(gpio_a, "GPIO BCM usado para la primera senal de entrada");

module_param(gpio_b, int, 0444);
MODULE_PARM_DESC(gpio_b, "GPIO BCM usado para la segunda senal de entrada");

module_param(gpio_chip_label, charp, 0444);
MODULE_PARM_DESC(gpio_chip_label, "Etiqueta del gpiochip que expone los GPIO BCM");

static ssize_t dual_gpio_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	char output[64];
	int value_a = gpiod_get_value(gpio_a_desc);
	int value_b = gpiod_get_value(gpio_b_desc);
	int len;

	len = scnprintf(output, sizeof(output),
			"gpio_a=%d value=%d\ngpio_b=%d value=%d\n",
			gpio_a, value_a, gpio_b, value_b);
	printk(KERN_INFO "Leidos valores: gpio_a=%d, gpio_b=%d\n", value_a, value_b);
	return simple_read_from_buffer(buf, count, ppos, output, len);
}

static const struct file_operations dual_gpio_fops = {
	.owner = THIS_MODULE,
	.read = dual_gpio_read,
	.llseek = noop_llseek,
};

static struct gpio_desc *request_input_gpio_desc(int offset, const char *label)
{
	struct gpio_device *gdev;
	struct gpio_chip *gc;
	struct gpio_desc *desc;

	gdev = gpio_device_find_by_label(gpio_chip_label);
	if (!gdev)
		return ERR_PTR(-ENODEV);

	gc = gpio_device_get_chip(gdev);
	if (!gc) {
		gpio_device_put(gdev);
		return ERR_PTR(-ENODEV);
	}

	if (offset < 0 || offset >= gc->ngpio) {
		gpio_device_put(gdev);
		return ERR_PTR(-EINVAL);
	}

	desc = gpiochip_request_own_desc(gc, offset, label,
					 GPIO_LOOKUP_FLAGS_DEFAULT, GPIOD_IN);
	gpio_device_put(gdev);

	return desc;
}

static void release_input_gpio_desc(struct gpio_desc *desc)
{
	if (!IS_ERR_OR_NULL(desc))
		gpiochip_free_own_desc(desc);
}

static int __init dual_gpio_input_init(void)
{
	int ret;

	if (gpio_a == gpio_b) {
		pr_err("Los GPIO de entrada deben ser distintos\n");
		return -EINVAL;
	}

	gpio_a_desc = request_input_gpio_desc(gpio_a, "dual_gpio_signal_a");
	if (IS_ERR(gpio_a_desc)) {
		ret = PTR_ERR(gpio_a_desc);
		gpio_a_desc = NULL;
		pr_err("No se pudo configurar gpio_a=%d en chip %s: %d\n",
		       gpio_a, gpio_chip_label, ret);
		return ret;
	}

	gpio_b_desc = request_input_gpio_desc(gpio_b, "dual_gpio_signal_b");
	if (IS_ERR(gpio_b_desc)) {
		ret = PTR_ERR(gpio_b_desc);
		gpio_b_desc = NULL;
		pr_err("No se pudo configurar gpio_b=%d en chip %s: %d\n",
		       gpio_b, gpio_chip_label, ret);
		release_input_gpio_desc(gpio_a_desc);
		gpio_a_desc = NULL;
		return ret;
	}

	ret = alloc_chrdev_region(&dual_gpio_devno, 0, 1, DEVICE_NAME);
	if (ret) {
		pr_err("No se pudo reservar un major/minor: %d\n", ret);
		release_input_gpio_desc(gpio_b_desc);
		release_input_gpio_desc(gpio_a_desc);
		gpio_b_desc = NULL;
		gpio_a_desc = NULL;
		return ret;
	}

	cdev_init(&dual_gpio_cdev, &dual_gpio_fops);
	dual_gpio_cdev.owner = THIS_MODULE;

	ret = cdev_add(&dual_gpio_cdev, dual_gpio_devno, 1);
	if (ret) {
		pr_err("No se pudo registrar el char device: %d\n", ret);
		unregister_chrdev_region(dual_gpio_devno, 1);
		release_input_gpio_desc(gpio_b_desc);
		release_input_gpio_desc(gpio_a_desc);
		gpio_b_desc = NULL;
		gpio_a_desc = NULL;
		return ret;
	}

	dual_gpio_class = class_create(CLASS_NAME);
	if (IS_ERR(dual_gpio_class)) {
		ret = PTR_ERR(dual_gpio_class);
		pr_err("No se pudo crear la clase del device: %d\n", ret);
		cdev_del(&dual_gpio_cdev);
		unregister_chrdev_region(dual_gpio_devno, 1);
		release_input_gpio_desc(gpio_b_desc);
		release_input_gpio_desc(gpio_a_desc);
		gpio_b_desc = NULL;
		gpio_a_desc = NULL;
		return ret;
	}

	dual_gpio_device = device_create(dual_gpio_class, NULL, dual_gpio_devno,
					 NULL, DEVICE_NAME);
	if (IS_ERR(dual_gpio_device)) {
		ret = PTR_ERR(dual_gpio_device);
		pr_err("No se pudo crear /dev/%s: %d\n", DEVICE_NAME, ret);
		class_destroy(dual_gpio_class);
		cdev_del(&dual_gpio_cdev);
		unregister_chrdev_region(dual_gpio_devno, 1);
		release_input_gpio_desc(gpio_b_desc);
		release_input_gpio_desc(gpio_a_desc);
		gpio_b_desc = NULL;
		gpio_a_desc = NULL;
		return ret;
	}

	pr_info("Modulo cargado: chip=%s gpio_a=%d value=%d, gpio_b=%d value=%d\n",
		gpio_chip_label, gpio_a, gpiod_get_value(gpio_a_desc),
		gpio_b, gpiod_get_value(gpio_b_desc));
	pr_info("Leer estados en /dev/%s\n", DEVICE_NAME);

	return 0;
}

static void __exit dual_gpio_input_exit(void)
{
	device_destroy(dual_gpio_class, dual_gpio_devno);
	class_destroy(dual_gpio_class);
	cdev_del(&dual_gpio_cdev);
	unregister_chrdev_region(dual_gpio_devno, 1);
	release_input_gpio_desc(gpio_b_desc);
	release_input_gpio_desc(gpio_a_desc);
	pr_info("Modulo dual_gpio_input descargado\n");
}

module_init(dual_gpio_input_init);
module_exit(dual_gpio_input_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Modulo para leer dos senales GPIO de entrada");
MODULE_AUTHOR("GRUPO RAMNT");
