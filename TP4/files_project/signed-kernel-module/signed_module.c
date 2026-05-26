#include <linux/module.h>	/* Requerido por todos los módulos */
#include <linux/kernel.h>	/* Definición de KERN_INFO */
MODULE_LICENSE("GPL"); 	/*  Licencia del modulo */
MODULE_DESCRIPTION("Modulo de kernel firmado");
MODULE_AUTHOR("GRUPO RAMNT");

/* Función que se invoca cuando se carga el módulo en el kernel */
int modulo_lin_init(void)
{
	printk(KERN_INFO "Modulo cargado en el kernel por el grupo RAMNT.\n");
	
	/* Devolver 0 para indicar una carga correcta del módulo */
	return 0;
}

/* Función que se invoca cuando se descarga el módulo del kernel */
void modulo_lin_clean(void)
{
	printk(KERN_INFO "Modulo descargado del kernel por el grupo RAMNT.\n");
}

/* Declaración de funciones init y exit */
module_init(modulo_lin_init);
module_exit(modulo_lin_clean);

