#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>



#define	GANYE_DEBUG_INFO(dev, fmt,...)\
		dev_info(dev, "%s,%d"fmt"", __func__, __LINE__,##__VA_ARGS__)

#define	LT8912B_INCLUDE_I2C_DEVICE_NUM		3

struct lt8912b_resolution_info{
	unsigned int Hsync_L;
	unsigned int Hsync_H;
	unsigned int Vsync_L;
	unsigned int Vsync_H;
};

struct lt8912b_pclk_info{
	unsigned int reg_0x920c;
	unsigned int reg_0x920d;
	unsigned int reg_0x920e;
	unsigned int reg_0x920f;
};

struct lt8912b_dev{
	unsigned int chip_id;	
	/*
		[0]	0x90
		[1]	0x92
		[2]	0x94
	*/
	struct regmap *my_regmap[LT8912B_INCLUDE_I2C_DEVICE_NUM];
	struct i2c_client* my_client[LT8912B_INCLUDE_I2C_DEVICE_NUM];
	struct task_struct* my_thread;
	struct lt8912b_resolution_info resolution_info;
	struct lt8912b_pclk_info pclk_info;
	struct gpio_desc* reset_gpiod;
};

struct video_timing{
	unsigned short hfp;
	unsigned short hs;
	unsigned short hbp;
	unsigned short hact;
	unsigned short htotal;
	unsigned short vfp;
	unsigned short vs;
	unsigned short vbp;
	unsigned short vact;
	unsigned short vtotal;
	unsigned short pclk_khz;
};

static const struct video_timing video_640x480_60Hz     ={ 8, 96,  40, 640,   800, 33,  2,  10, 480,   525};
static const struct video_timing video_720x480_60Hz     ={16, 62,  60, 720,   858,  9,  6,  30, 480,   525};
static const struct video_timing video_1280x720_60Hz    ={110,40, 220,1280,  1650,  5,  5,  20, 720,   750};
static const struct video_timing video_1920x1080_60Hz   ={88, 44, 148,1920,  2200,  4,  5,  36, 1080, 1125};
static const struct video_timing video_3840x1080_60Hz   ={176,88, 296,3840,  4400,  4,  5,  36, 1080, 1125};
static const struct video_timing video_3840x2160_30Hz   ={176,88, 296,3840,  4400,  8,  10, 72, 2160, 2250};
static const struct video_timing video_1280x800_30Hz    ={40, 40,  80,1280,  1440,  5,   5, 13,  800,   823,7110};



static const struct regmap_config lt8912b_regmap_config = {
						.reg_bits = 8,
						.val_bits = 8,
						.max_register = 0xff,
};

static int lt8921b_i2c_init(struct lt8912b_dev*lt8912b, struct i2c_adapter *adapter)
{
	
	const struct i2c_board_info info[] = {
		{ I2C_BOARD_INFO("lt8912bp0", 0x48), },
		{ I2C_BOARD_INFO("lt8912bp1", 0x49), },
		{ I2C_BOARD_INFO("lt8912bp2", 0x4a), }	
	};
	struct regmap *regmap;
	int tmp_count = 0;
	int ret = -1;
	for(tmp_count = 0; tmp_count <	LT8912B_INCLUDE_I2C_DEVICE_NUM; ++tmp_count){
		lt8912b->my_client[tmp_count] = i2c_new_device(adapter, &info[tmp_count]);
		printk("%s, %d:tmp_count=%d,1\n", __func__, __LINE__, tmp_count);
		if (!lt8912b->my_client[tmp_count]){
			printk("client create error!!tmp_count=%d\n", tmp_count);
			return -ENODEV;
		}
		printk("%s, %d:tmp_count=%d,2\n", __func__, __LINE__, tmp_count);

		regmap = devm_regmap_init_i2c(lt8912b->my_client[tmp_count],	&lt8912b_regmap_config);
		printk("%s, %d:tmp_count=%d,3\n", __func__, __LINE__, tmp_count);
		if(IS_ERR(regmap)){
			ret = PTR_ERR(regmap);
			printk("devm_regmap_init_i2c error!!, tmp_count=%d\n", tmp_count);
			return ret;
		}
		printk("%s, %d:tmp_count=%d,4\n", __func__, __LINE__, tmp_count);
		lt8912b->my_regmap[tmp_count] = regmap;
	}
	return 0;
	
}
#if 0
static void 	lt8912b_Hdmi_print_test_pattern(struct lt8912b_dev*lt8912b)
{

	/* 0x90 */
	regmap_write(lt8912b->my_regmap[0], 0x08,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x09,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x0a,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x0b,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x0c,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x31,0xa1);
	regmap_write(lt8912b->my_regmap[0], 0x32,0xa1);
	regmap_write(lt8912b->my_regmap[0], 0x33,0x03);
	regmap_write(lt8912b->my_regmap[0], 0x37,0x00);
	regmap_write(lt8912b->my_regmap[0], 0x38,0x22);
	regmap_write(lt8912b->my_regmap[0], 0x60,0x82);
	regmap_write(lt8912b->my_regmap[0], 0x39,0x45);
	regmap_write(lt8912b->my_regmap[0], 0x3b,0x00);
	regmap_write(lt8912b->my_regmap[0], 0x44,0x31);
	regmap_write(lt8912b->my_regmap[0], 0x55,0x44);
	regmap_write(lt8912b->my_regmap[0], 0x57,0x01);
	regmap_write(lt8912b->my_regmap[0], 0x5a,0x02);
	/* 0x92 */
	regmap_write(lt8912b->my_regmap[1], 0x10,0x20);  // term en  To analog phy for trans lp mode to hs mode
	regmap_write(lt8912b->my_regmap[1], 0x11,0x04);  // settle Set timing for dphy trans state from PRPR to SOT state
	regmap_write(lt8912b->my_regmap[1], 0x12,0x04);  // trail
	regmap_write(lt8912b->my_regmap[1], 0x13,0x00);  // 4 lane  // 01 lane // 02 2 lane //03 3lane
	regmap_write(lt8912b->my_regmap[1], 0x14,0x00);  //debug mux
	regmap_write(lt8912b->my_regmap[1], 0x15,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x1a,0x03);  // hshift 3
	regmap_write(lt8912b->my_regmap[1], 0x1b,0x03);  // vshift 3
	regmap_write(lt8912b->my_regmap[1], 0x18,0x3e);  // hwidth 62
	regmap_write(lt8912b->my_regmap[1], 0x19,0x06);  // vwidth 6
	regmap_write(lt8912b->my_regmap[1], 0x1c,0xd0);  // pix num hactive
	regmap_write(lt8912b->my_regmap[1], 0x1d,0x02);  
	regmap_write(lt8912b->my_regmap[1], 0x1e,0x67); //h v d pol hdmi sel pll sel
	regmap_write(lt8912b->my_regmap[1], 0x2f,0x0c); //fifo_buff_length 12
	regmap_write(lt8912b->my_regmap[1], 0x34,0x5a); // htotal 858
	regmap_write(lt8912b->my_regmap[1], 0x35,0x03); // htotal 858
	regmap_write(lt8912b->my_regmap[1], 0x36,0x0d); // vtotal 525
	regmap_write(lt8912b->my_regmap[1], 0x37,0x02); // vtotal 525
	regmap_write(lt8912b->my_regmap[1], 0x38,0x1e); // vbp 30
	regmap_write(lt8912b->my_regmap[1], 0x39,0x00); // vbp 30
	regmap_write(lt8912b->my_regmap[1], 0x3a,0x09); // vfp 9
	regmap_write(lt8912b->my_regmap[1], 0x3b,0x00); // vfp 9
	regmap_write(lt8912b->my_regmap[1], 0x3c,0x3c); // hbp 60
	regmap_write(lt8912b->my_regmap[1], 0x3d,0x00); // hbp 60
	regmap_write(lt8912b->my_regmap[1], 0x3e,0x10); // hfp 16
	regmap_write(lt8912b->my_regmap[1], 0x3f,0x00); // hfp 16
	regmap_write(lt8912b->my_regmap[1], 0x72,0x12);
	regmap_write(lt8912b->my_regmap[1], 0x73,0x7a); //RGD_PTN_DE_DLY[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x74,0x00); //RGD_PTN_DE_DLY[11:8]  260
	regmap_write(lt8912b->my_regmap[1], 0x75,0x24); //RGD_PTN_DE_TOP[6:0]   150
	regmap_write(lt8912b->my_regmap[1], 0x76,0xd0); //RGD_PTN_DE_CNT[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x77,0xe0); //RGD_PTN_DE_LIN[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x78,0x12); //RGD_PTN_DE_LIN[10:8],RGD_PTN_DE_CNT[11:8]
	regmap_write(lt8912b->my_regmap[1], 0x79,0x5a); //RGD_PTN_H_TOTAL[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x7a,0x0d); //RGD_PTN_V_TOTAL[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x7b,0x23); //RGD_PTN_V_TOTAL[10:8],RGD_PTN_H_TOTAL[11:8]
	regmap_write(lt8912b->my_regmap[1], 0x7c,0x3e); //RGD_PTN_HWIDTH[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x7d,0x06); //RGD_PTN_HWIDTH[9:8],RGD_PTN_VWIDTH[5:0]
	regmap_write(lt8912b->my_regmap[1], 0x70,0x80); 
	regmap_write(lt8912b->my_regmap[1], 0x71,0x51);
	regmap_write(lt8912b->my_regmap[1], 0x4e,0x66); //strm_sw_freq_word[ 7: 0] 
	regmap_write(lt8912b->my_regmap[1], 0x4f,0x66); //strm_sw_freq_word[15: 8]
	regmap_write(lt8912b->my_regmap[1], 0x50,0x26); //strm_sw_freq_word[23:16]
	regmap_write(lt8912b->my_regmap[1], 0x51,0x80); //pattern en

}
#endif
static void lt8912b_read_id(struct lt8912b_dev*lt8912b)
{
	unsigned int chip_id_low;
	unsigned int chip_id_high;
	regmap_read(lt8912b->my_regmap[0], 0x00, &chip_id_high);
	regmap_read(lt8912b->my_regmap[0], 0x01, &chip_id_low);
	lt8912b->chip_id = (chip_id_high << 8)|(chip_id_low);
	printk("chip_id_high = 0x%x chip_id_low=0x%x,chip_id = 0x%x\n",
			chip_id_high ,chip_id_low, lt8912b->chip_id);
}

static void read_resolution_info(struct lt8912b_dev*lt8912b, 
		struct lt8912b_resolution_info *resolution_info)
{

	regmap_read(lt8912b->my_regmap[0], 0x9c, &resolution_info->Hsync_L);
	regmap_read(lt8912b->my_regmap[0], 0x9d, &resolution_info->Hsync_H);
	regmap_read(lt8912b->my_regmap[0], 0x9e, &resolution_info->Vsync_L);
	regmap_read(lt8912b->my_regmap[0], 0x9f, &resolution_info->Vsync_H);

}
#if 0
static void read_pclk_info(struct lt8912b_dev*lt8912b, 
		struct lt8912b_pclk_info *pclk_info)
{

	regmap_read(lt8912b->my_regmap[1], 0x0c, &pclk_info->reg_0x920c);
	regmap_read(lt8912b->my_regmap[1], 0x0d, &pclk_info->reg_0x920d);
	regmap_read(lt8912b->my_regmap[1], 0x0e, &pclk_info->reg_0x920e);
	regmap_read(lt8912b->my_regmap[1], 0x0f, &pclk_info->reg_0x920f);

}
#endif

static void print_resolution_pclk_info(struct lt8912b_dev* lt8912b)
{
	struct lt8912b_resolution_info *resolution_info = &lt8912b->resolution_info;
//	struct lt8912b_pclk_info *pclk_info = &lt8912b->pclk_info;
	
	read_resolution_info(lt8912b, resolution_info);
//	read_pclk_info(lt8912b, pclk_info);
	
	printk(" RES_INFO:\nHsync_L = %x\n Hsync_H = %x\n Vsync_L = %x\n Vsync_H = %x\n",
							resolution_info->Hsync_L, resolution_info->Hsync_H,
							resolution_info->Vsync_L, resolution_info->Vsync_H);
	#if 0
	printk("DDS_INFO:\nreg_0x920c = %x\n reg_0x920d = %x\n reg_0x920e = %x\n reg_0x920f = %x\n",
			pclk_info->reg_0x920c, pclk_info->reg_0x920d, 
			pclk_info->reg_0x920e, pclk_info->reg_0x920f);
	#endif


}
static void  MIPI_Video_Setup(struct lt8912b_dev*lt8912b ,const struct video_timing *video_format)
{
	regmap_write(lt8912b->my_regmap[1], 0x18,(unsigned short)(video_format->hs%256)); // hwidth
	regmap_write(lt8912b->my_regmap[1], 0x19,(unsigned short)(video_format->vs%256)); // vwidth 6
	regmap_write(lt8912b->my_regmap[1], 0x1c,(unsigned short)(video_format->hact%256)); // H_active[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x1d,(unsigned short)(video_format->hact/256)); // H_active[15:8]
	regmap_write(lt8912b->my_regmap[1], 0x2f,0x0c); // fifo_buff_length 12
	regmap_write(lt8912b->my_regmap[1], 0x34,(unsigned short)(video_format->htotal%256)); // H_total[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x35,(unsigned short)(video_format->htotal/256)); // H_total[15:8]
	regmap_write(lt8912b->my_regmap[1], 0x36,(unsigned short)(video_format->vtotal%256)); // V_total[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x37,(unsigned short)(video_format->vtotal/256)); // V_total[15:8]
	regmap_write(lt8912b->my_regmap[1], 0x38,(unsigned short)(video_format->vbp%256)); // VBP[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x39,(unsigned short)(video_format->vbp/256)); // VBP[15:8]
	regmap_write(lt8912b->my_regmap[1], 0x3a,(unsigned short)(video_format->vfp%256)); // VFP[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x3b,(unsigned short)(video_format->vfp/256)); // VFP[15:8]
	regmap_write(lt8912b->my_regmap[1], 0x3c,(unsigned short)(video_format->hbp%256)); // HBP[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x3d,(unsigned short)(video_format->hbp/256)); // HBP[15:8]
	regmap_write(lt8912b->my_regmap[1], 0x3e,(unsigned short)(video_format->hfp%256)); // HFP[7:0]
	regmap_write(lt8912b->my_regmap[1], 0x3f,(unsigned short)(video_format->hfp/256)); // HFP[15:8]
}

static void MIPIRxLogicRes(struct lt8912b_dev*lt8912b)
{
	regmap_write(lt8912b->my_regmap[0], 0x03,0x7f);
	msleep(100);
	regmap_write(lt8912b->my_regmap[0],0x03,0xff);
}

static void MIPI_Input_det(struct lt8912b_dev*lt8912b)
{
	struct lt8912b_resolution_info *last_resolution_info = &lt8912b->resolution_info;
	/*struct lt8912b_pclk_info *last_pclk_info = &lt8912b->pclk_info;*/
	struct lt8912b_resolution_info now_resolution_info;
	
	read_resolution_info(lt8912b, &now_resolution_info);
	
	if((now_resolution_info.Hsync_H) != (last_resolution_info->Hsync_H)||
		(now_resolution_info.Vsync_H) != (last_resolution_info->Vsync_H) ){
		print_resolution_pclk_info(lt8912b);
		if(now_resolution_info.Vsync_H == 0x02 && now_resolution_info.Vsync_L == 0x0d)//0x20D

		{
			MIPI_Video_Setup(lt8912b, &video_640x480_60Hz);
			printk("nvideoformat = VESA_640x480_60 \n");
		}
		else if(now_resolution_info.Vsync_H==0x02 && 
			now_resolution_info.Vsync_L <= 0xef&&
			now_resolution_info.Vsync_L >= 0xec)//0x2EE
		{
			MIPI_Video_Setup(lt8912b, &video_1280x720_60Hz);
			printk("nvideoformat = VESA_1280x720_60\n");
		}
		else if(now_resolution_info.Vsync_H == 0x04 && 
			now_resolution_info.Vsync_L <= 0x67&&
			now_resolution_info.Vsync_L >= 0x63)//0x465
		{
			 MIPI_Video_Setup(lt8912b, &video_1920x1080_60Hz);
			 printk("videoformat = VESA_1920x1080_60\n");
		}
		else
		{
			printk("videoformat = video_none\n");
		}
			last_resolution_info->Hsync_L = now_resolution_info.Hsync_L;
			last_resolution_info->Hsync_L = now_resolution_info.Hsync_H;
			last_resolution_info->Hsync_L = now_resolution_info.Vsync_L;
			last_resolution_info->Hsync_L = now_resolution_info.Vsync_H;
			MIPIRxLogicRes(lt8912b);
	}
	
}
static int read_important_reg_info_threadfn(void *data)
{
	struct lt8912b_dev* lt8912b = data;
	//struct lt8912b_pclk_info *pclk_info = &lt8912b->pclk_info;
	
 	while(!kthread_should_stop()){
	 	MIPI_Input_det(lt8912b);
		
		#if 0
		read_pclk_info(lt8912b, pclk_info);
		printk("DDS_INFO:\n reg_0x920c = %x\n reg_0x920d = %x\n reg_0x920e = %x\n reg_0x920f = %x\n",
			pclk_info->reg_0x920c, pclk_info->reg_0x920d, 
			pclk_info->reg_0x920e, pclk_info->reg_0x920f);
		#endif
		msleep(3000);
	}
	
	return 0;
}

static void reset_lt8912(struct lt8912b_dev *lt8912b)
{

	//printk("%s,%d:should reset lt8912!!\n", __func__, __LINE__);
	gpiod_direction_output(lt8912b->reset_gpiod, 0);
	msleep(100);
	gpiod_direction_output(lt8912b->reset_gpiod, 1);
	msleep(100);
}

static void lt8912b_init(struct lt8912b_dev*lt8912b)
{

	reset_lt8912(lt8912b);
	lt8912b_read_id(lt8912b);
	/* DigitalClockEn, 0x90 */
	regmap_write(lt8912b->my_regmap[0], 0x08,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x09,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x0a,0xff);
	regmap_write(lt8912b->my_regmap[0], 0x0b,0x7c);
	regmap_write(lt8912b->my_regmap[0], 0x0c,0xff);
	/* TxAnalog,0x90 */
	regmap_write(lt8912b->my_regmap[0], 0x31,0xa1);
	regmap_write(lt8912b->my_regmap[0], 0x32,0xa1);
	regmap_write(lt8912b->my_regmap[0], 0x33,0x03);
	regmap_write(lt8912b->my_regmap[0], 0x37,0x00);
	regmap_write(lt8912b->my_regmap[0], 0x38,0x22);
	regmap_write(lt8912b->my_regmap[0], 0x60,0x82);
	/* CbusAnalog,0x90 */
	regmap_write(lt8912b->my_regmap[0], 0x39,0x45);
	regmap_write(lt8912b->my_regmap[0], 0x3b,0x00);
	/*HDMIPllAnalog,0x90*/
	regmap_write(lt8912b->my_regmap[0], 0x44,0x30);
	regmap_write(lt8912b->my_regmap[0], 0x55,0x44);
	regmap_write(lt8912b->my_regmap[0], 0x57,0x01);
	regmap_write(lt8912b->my_regmap[0], 0x5a,0x02);
	/* MipiBasicSet,0x92 */
	regmap_write(lt8912b->my_regmap[1], 0x10,0x01);  // term en,
	regmap_write(lt8912b->my_regmap[1], 0x11,0x04);  // settle
	regmap_write(lt8912b->my_regmap[1], 0x12,0x04);  // trail
	regmap_write(lt8912b->my_regmap[1], 0x13,0x00);  // 4 lane  // 01 lane // 02 2 lane //03 3lane
	regmap_write(lt8912b->my_regmap[1], 0x14,0x00);  //debug mux
	regmap_write(lt8912b->my_regmap[1], 0x1a,0x03);  // hshift 3
	regmap_write(lt8912b->my_regmap[1], 0x1b,0x03);  // vshift 3
	/* DDS1080PConfig,0x92 */
	regmap_write(lt8912b->my_regmap[1], 0x4e,0x6A);//
	regmap_write(lt8912b->my_regmap[1], 0x4f,0x4D);
	regmap_write(lt8912b->my_regmap[1], 0x50,0xF3);
	regmap_write(lt8912b->my_regmap[1], 0x51,0x80);
	regmap_write(lt8912b->my_regmap[1], 0x1f,0x40);
	regmap_write(lt8912b->my_regmap[1], 0x20,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x21,0x18);
	regmap_write(lt8912b->my_regmap[1], 0x22,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x23,0x0E);
	regmap_write(lt8912b->my_regmap[1], 0x24,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x25,0x04);
	regmap_write(lt8912b->my_regmap[1], 0x26,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x27,0x40);
	regmap_write(lt8912b->my_regmap[1], 0x28,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x29,0x18);
	regmap_write(lt8912b->my_regmap[1], 0x2a,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x2b,0x0E);
	regmap_write(lt8912b->my_regmap[1], 0x2c,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x2d,0x04);
	regmap_write(lt8912b->my_regmap[1], 0x2e,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x42,0x64);
	regmap_write(lt8912b->my_regmap[1], 0x43,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x44,0x10);
	regmap_write(lt8912b->my_regmap[1], 0x45,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x46,0x40);
	regmap_write(lt8912b->my_regmap[1], 0x47,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x48,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x49,0x19);
	regmap_write(lt8912b->my_regmap[1], 0x4a,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x4b,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x4c,0xfa);
	regmap_write(lt8912b->my_regmap[1], 0x4d,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x52,0x20);
	regmap_write(lt8912b->my_regmap[1], 0x53,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x54,0x80);
	regmap_write(lt8912b->my_regmap[1], 0x55,0x02);
	regmap_write(lt8912b->my_regmap[1], 0x56,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x57,0x32);
	regmap_write(lt8912b->my_regmap[1], 0x58,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x59,0x00);
	regmap_write(lt8912b->my_regmap[1], 0x5a,0xf4);
	regmap_write(lt8912b->my_regmap[1], 0x5b,0x01);
	regmap_write(lt8912b->my_regmap[1], 0x5c,0x34);
	regmap_write(lt8912b->my_regmap[1], 0x1e,0x4f);
	regmap_write(lt8912b->my_regmap[1], 0x51,0x00);
}

static int lt8912b_driver_probe(struct platform_device * pdev )
{

	struct i2c_adapter *adapter;
	struct lt8912b_dev	*lt8912b = NULL;
	struct device_node * node;
	//lt8912 dev
	struct device	*dev = &pdev->dev;
	GANYE_DEBUG_INFO(dev," ");
	
	lt8912b = devm_kzalloc(dev, sizeof(struct lt8912b_dev), GFP_KERNEL);
	if (!lt8912b)
	{
		printk("lt8912b kzalloc error!!!\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev,lt8912b);
	node = of_parse_phandle(dev->of_node, "i2c-bus", 0);
	if (!node) {
		GANYE_DEBUG_INFO(dev, "No i2c-bus found!!");
		return -ENODEV;
	}
	printk("%s, %d:1\n", __func__, __LINE__);
	adapter = of_find_i2c_adapter_by_node(node);
	of_node_put(node);//Decrement refcount of a node
	
	if (!adapter) {
			dev_err(dev, "No i2c adapter found\n");
			return -EPROBE_DEFER;
	}
	
	printk("%s, %d:2\n", __func__, __LINE__);
	lt8921b_i2c_init(lt8912b, adapter);
	/* reset gpio */
	lt8912b->reset_gpiod = devm_gpiod_get(dev ,"reset", 0);
	if(IS_ERR(lt8912b->reset_gpiod)){
		printk("get reset gpio error!!\n");
	}

	//lt8912b_Hdmi_print_test_pattern(lt8912b);
	lt8912b_init(lt8912b);
	
	/* create kernel thread */
	lt8912b->my_thread = kthread_create(read_important_reg_info_threadfn, lt8912b, "lt8912_thread");
	if(IS_ERR(lt8912b->my_thread)){
		printk("lt8912b->my_thread create error\n");
		return -1;
	}
	wake_up_process(lt8912b->my_thread);
	return 0;

}

static int lt8912b_driver_remove(struct platform_device *pdev)
{

	struct lt8912b_dev *lt8912b = platform_get_drvdata(pdev);
	int tmp_count;
	printk("%s,%d:chip_id=%x\n", __func__, __LINE__, lt8912b->chip_id);
	kthread_stop(lt8912b->my_thread);

	for(tmp_count =0 ; tmp_count <LT8912B_INCLUDE_I2C_DEVICE_NUM; ++tmp_count){
		//regmap_exit(lt8912b->my_regmap[tmp_count]);
		i2c_unregister_device(lt8912b->my_client[tmp_count]);
	}
	
	GANYE_DEBUG_INFO(&pdev->dev, " ");
	return 0;
}

static const struct of_device_id of_lt8912b_match[] = {
	{.compatible = "lontium,lt8912b",},
	{}
};

static struct platform_driver lt8912b_driver = {
	.probe 		= lt8912b_driver_probe,
	.remove 	= lt8912b_driver_remove,
	.driver 	={
		.name = "lt8912b",
		.of_match_table = of_lt8912b_match,
	},
	
	
};

module_platform_driver(lt8912b_driver);
MODULE_AUTHOR("qiganchen <chenqigan@usr.cn>");
MODULE_DESCRIPTION("Lontium LT8912B MIPI-DSI to LVDS and HDMI");
MODULE_LICENSE("GPL v2");
