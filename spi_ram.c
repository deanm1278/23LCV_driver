#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_device.h>
#include <linux/kobject.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/spi/spi.h>    // Required for SPI stuff
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dean Miller");
MODULE_DESCRIPTION("Linux Driver for MB85RS SPI ram device.");
MODULE_VERSION("0.1");

#define SPI_MAX_SPEED 400000

struct MB85RS {
    struct  spi_device      *myspi;
    unsigned char           datawidth;
    unsigned int            holdpin;
    unsigned char           addr;
};

//show the current address pointer
static ssize_t addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len, ret;
    struct MB85RS *drv = dev_get_drvdata(dev);
    
    #ifdef MB85RS_DEBUG
        printk(KERN_INFO "RAM: addr show command received.\n");
    #endif

    return ret;
}

//set the current address pointer
static ssize_t addr_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    #ifdef MB85RS_DEBUG
        printk(KERN_INFO "RAM: addr store command received.\n");
    #endif
    
   return count;
}

//Print all stored data
static ssize_t data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len, ret;
    struct MB85RS *drv = dev_get_drvdata(dev);
    
    #ifdef MB85RS_DEBUG
        printk(KERN_INFO "RAM: data show command received.\n");
    #endif

    return ret;
}

//write to RAM
static ssize_t data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
    #ifdef MB85RS_DEBUG
        printk(KERN_INFO "RAM: data store command received.\n");
    #endif
    
   return count;
}

//declare attributes
static DEVICE_ATTR(data, 0660, data_show, data_store);
static DEVICE_ATTR(addr, 0660, addr_show, addr_store);

static struct attribute *attrs[] = {
    &dev_attr_data.attr,
    &dev_attr_addr.attr,
    NULL,
};

static struct attribute_group attr_group = { .attrs = attrs };

static const struct file_operations fifo_fops = {
    .owner      = THIS_MODULE,
};

static int MB85RS_spi_probe(struct spi_device *spi){
    int err = 0;
    struct MB85RS *drv;
    struct device *dev;
    
#ifdef RAM_DEBUG
    printk(KERN_INFO "RAM: probing device...\n");
#endif
    
    drv = kzalloc(sizeof(struct MB85RS), GFP_KERNEL);
    if(!drv){
        printk(KERN_ERR "RAM: could not allocate servodrv struct.\n");
        return -1;
    }
    
    //initialize SPI interface
    drv->myspi = spi;
    drv->myspi->max_speed_hz = SPI_MAX_SPEED;
    drv->myspi->bits_per_word = drv->datawidth * 8;
    spi_setup(drv->myspi);
    spi_set_drvdata(spi, drv);
    
    dev = &spi->dev;
    dev_set_drvdata(dev, drv);
    
    gpio_request(drv->holdpin, "sysfs");
    gpio_direction_output(drv->holdpin, 0);
    gpio_export(drv->holdpin, false);          // export the hold pin
    
    err = sysfs_create_group(&dev->kobj, &attr_group);
    if(err){
        printk(KERN_ERR "RAM: registration failed.\n");
        goto faildreg;
    }
    
    printk(KERN_INFO "RAM: sucessfully loaded!\n");
    
    return err;
    
faildreg:
    gpio_unexport(drv->holdpin);
    gpio_free(drv->holdpin);
    
    return -1;
}

static int MB85RS_spi_remove(struct spi_device *spi){
    int err = 0;
    struct MB85RS *drv = spi_get_drvdata(spi);
    struct device *dev = &spi->dev;
    
    printk(KERN_INFO "DRV: removing device...\n");
    
    gpio_unexport(drv->holdpin);
    gpio_free(drv->holdpin);
    
    /* Remove the sysfs attributes */
    sysfs_remove_group(&dev->kobj, &attr_group);

    kfree(drv);
    return err;
}

static const struct of_device_id MB85RS_of_match[] = {
	{ .compatible = "MB85RS", },
	{ }
};
MODULE_DEVICE_TABLE(of, MB85RS_of_match);

static struct spi_driver MB85RS_spi_driver = {
	.driver = {
                .owner  = THIS_MODULE,
		.name	= "spi_ram",
		.of_match_table	= MB85RS_of_match,
	},
	.probe		= MB85RS_spi_probe,
	.remove		= MB85RS_spi_remove,
};

static int __init MB85RS_init(void){
    int err = 0;
    
    printk(KERN_INFO "RAM: initializing module...\n");
    err = spi_register_driver(&MB85RS_spi_driver);
    return err;
}

static void __exit MB85RS_exit(void){
    printk(KERN_INFO "RAM: exiting module...\n");
    spi_unregister_driver(&MB85RS_spi_driver);
}

module_init(MB85RS_init);
module_exit(MB85RS_exit);