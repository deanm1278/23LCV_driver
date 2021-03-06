#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_device.h>
#include <linux/kobject.h>
#include <linux/gpio.h>       // Required for the GPIO functions
#include <linux/spi/spi.h>    // Required for SPI stuff
#include <linux/proc_fs.h>

#define R_23LCV_DEBUG

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dean Miller");
MODULE_DESCRIPTION("Linux Driver for 23LCVxxx SPI ram device.");
MODULE_VERSION("0.1");

#define DATAWIDTH       8
#define BYTES_AVAILABLE 65536 //change for 23LCV1024

#define OPCODE_READ     0x03        /* Read Memory */
#define OPCODE_WRITE    0x02        /* Write Memory */
#define OPCODE_RSTIO    0xFF        /* Reset dual IO access */
#define OPCODE_EDIO     0x3B        /* Enter Dual IO access */
#define OPCODE_RDMR     0x05        /* Read mode register */
#define OPCODE_WRMR     0x01        /* Write mode register */

struct R_23LCV {
    struct  spi_device      *myspi;
    unsigned int            listenstate;
    u16                     addr;
    u16                     size;
};

static unsigned int cspin = 14;       ///< Default cs pin is 16
module_param(cspin, uint, S_IRUGO);    ///< Param desc. S_IRUGO can be read/not changed
MODULE_PARM_DESC(cspin, "alternate chip select pin (default=14)");

static unsigned int SPI_MAX_SPEED = 4000000; // Default SPI max speed is 4000000
module_param(SPI_MAX_SPEED, uint, S_IRUGO);
MODULE_PARM_DESC(SPI_MAX_SPEED, "SPI max speed (default=4000000)");

//show the current address pointer
static ssize_t addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct R_23LCV *drv = dev_get_drvdata(dev);
    
    return sprintf(buf, "0x%x\n", drv->addr);
}

//set the current address pointer
static ssize_t addr_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
   unsigned int addr;                     // Using a variable to validate the data sent
   struct R_23LCV *drv = dev_get_drvdata(dev);
   
   sscanf(buf, "%x", &addr);             // Read in the size as an unsigned int
   if (addr <=BYTES_AVAILABLE){        // dont allow address past max capacity
      drv->addr = addr;                
   }
   return addr;
}

//set the read size
static ssize_t size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
   unsigned int size;                     // Using a variable to validate the data sent
   struct R_23LCV *drv = dev_get_drvdata(dev);
   
   sscanf(buf, "%du", &size);             // Read in the size as an unsigned int
   if ((size>0)&&(size<=BYTES_AVAILABLE)){        // Must be 1 byte or greater, less than the max size of the chip
      drv->size = size;                
   }
   return size;
}

//Print read size
static ssize_t size_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct R_23LCV *drv = dev_get_drvdata(dev);
    
    return sprintf(buf, "%d\n", drv->size);
}

//show the current listen state
static ssize_t listen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct R_23LCV *drv = dev_get_drvdata(dev);
    
    return sprintf(buf, "%d\n", drv->listenstate);
}

//set the listen state
static ssize_t listen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
   unsigned int state, status;                     // Using a variable to validate the data sent
   struct R_23LCV *drv = dev_get_drvdata(dev);
   unsigned char txbuf[3];
   
   sscanf(buf, "%du", &state);             // Read in the size as an unsigned int
   if ((state == 0 || state == 1) && state != drv->listenstate){        // Must be 1 byte or greater, less than the max size of the chip
      drv->listenstate = state;
      if(state == 1){
            txbuf[0] = OPCODE_WRITE;
            txbuf[1] = (u8)(drv->addr >> 8);
            txbuf[2] = (u8)(drv->addr & 0xFF);
            
            gpio_set_value(cspin, 0);
            status = spi_write(drv->myspi, txbuf, 3);
      }
      else{
          //stop listening
           gpio_set_value(cspin, 1);
      }
   }
   return state;
}


//copy stored data to userspace
static ssize_t data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct R_23LCV *drv = dev_get_drvdata(dev);
    
    unsigned char txbuf[] = {OPCODE_READ, (u8)(drv->addr >> 8), (u8)(drv->addr & 0xFF)};
    int status;
    
    gpio_set_value(cspin, 0);
    status = spi_write_then_read(drv->myspi, txbuf, 3, buf, drv->size);
    gpio_set_value(cspin, 1);
    
    return drv->size;
}

//write to RAM
static ssize_t data_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
        int status;
        struct R_23LCV *drv = dev_get_drvdata(dev);
        unsigned char txbuf[count + 3];
        
        txbuf[0] = OPCODE_WRITE;
        txbuf[1] = (u8)(drv->addr >> 8);
        txbuf[2] = (u8)(drv->addr & 0xFF);
        memcpy(&txbuf[3], buf, count);
        
        //write the data
        gpio_set_value(cspin, 0);
        status = spi_write(drv->myspi, txbuf, count + 3);
        gpio_set_value(cspin, 1);
        
        return count;
}

//declare attributes
static DEVICE_ATTR(data, 0660, data_show, data_store);
static DEVICE_ATTR(addr, 0660, addr_show, addr_store);
static DEVICE_ATTR(size, 0660, size_show, size_store);
static DEVICE_ATTR(listen, 0660, listen_show, listen_store);

static struct attribute *attrs[] = {
    &dev_attr_data.attr,
    &dev_attr_addr.attr,
    &dev_attr_size.attr,
    &dev_attr_listen.attr,
    NULL,
};

static struct attribute_group attr_group = { .attrs = attrs };

static const struct file_operations fifo_fops = {
    .owner      = THIS_MODULE,
};

static int R_23LCV_spi_probe(struct spi_device *spi){
    int err = 0;
    struct R_23LCV *drv;
    struct device *dev;
    
#ifdef RAM_DEBUG
    printk(KERN_INFO "RAM: probing device...\n");
#endif
    
    drv = kzalloc(sizeof(struct R_23LCV), GFP_KERNEL);
    if(!drv){
        printk(KERN_ERR "RAM: could not allocate R_23LCV struct.\n");
        return -1;
    }
    
    //initialize SPI interface
    drv->myspi = spi;
    drv->myspi->max_speed_hz = SPI_MAX_SPEED;
    drv->myspi->bits_per_word = DATAWIDTH;
    spi_setup(drv->myspi);
    spi_set_drvdata(spi, drv);
    
    dev = &spi->dev;
    dev_set_drvdata(dev, drv);
    
    gpio_request(cspin, "sysfs");
    gpio_direction_output(cspin, 1);
    gpio_export(cspin, false);          // export the cs pin
    
    err = sysfs_create_group(&dev->kobj, &attr_group);
    if(err){
        printk(KERN_ERR "RAM: registration failed.\n");
        goto faildreg;
    }
    
    printk(KERN_INFO "RAM: sucessfully loaded!\n");
    
    return err;
    
faildreg:
    gpio_unexport(cspin);
    gpio_free(cspin);
    
    return -1;
}

static int R_23LCV_spi_remove(struct spi_device *spi){
    int err = 0;
    struct R_23LCV *drv = spi_get_drvdata(spi);
    struct device *dev = &spi->dev;
    
    printk(KERN_INFO "DRV: removing device...\n");
    
    gpio_unexport(cspin);
    gpio_free(cspin);
    
    /* Remove the sysfs attributes */
    sysfs_remove_group(&dev->kobj, &attr_group);

    kfree(drv);
    return err;
}

static const struct of_device_id R_23LCV_of_match[] = {
	{ .compatible = "mcp,23lcv", },
	{ }
};
MODULE_DEVICE_TABLE(of, R_23LCV_of_match);

static struct spi_driver R_23LCV_spi_driver = {
	.driver = {
                .owner  = THIS_MODULE,
		.name	= "spi_ram",
		.of_match_table	= R_23LCV_of_match,
	},
	.probe		= R_23LCV_spi_probe,
	.remove		= R_23LCV_spi_remove,
};

static int __init R_23LCV_init(void){
    int err = 0;
    
    printk(KERN_INFO "RAM: initializing module...\n");
    err = spi_register_driver(&R_23LCV_spi_driver);
    return err;
}

static void __exit R_23LCV_exit(void){
    printk(KERN_INFO "RAM: exiting module...\n");
    spi_unregister_driver(&R_23LCV_spi_driver);
}

module_init(R_23LCV_init);
module_exit(R_23LCV_exit);
