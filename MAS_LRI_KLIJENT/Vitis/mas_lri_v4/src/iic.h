/*
 * FUNCTION PROTOTYPE DECLARATIONS
 */
int iic_OV7670_init(void);
int write_reg(u8 register_address, u8 value);
int read_reg(u8 register_address);
void init_regs(struct regval_list *reg_list);
void init_default_regs(struct regval_list* OV7670_default_regs);
void init_RGB565(struct regval_list* OV7670_RGB565);
void init_YUV(struct regval_list* OV7670_YUV422);
void init_test_bar(struct regval_list* OV7670_test_bar);
void init_image(struct regval_list* OV7670_image);
