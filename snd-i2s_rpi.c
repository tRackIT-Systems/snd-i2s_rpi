/*
 * =====================================================================================
 *
 *       Filename:  snd-i2s_rpi
 *
 *    Description:  
 *
 *        Version:  0.0.4
 *        Created:  2024-01-09
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Jonas Höchst (hoechst@trackit.systems), originally written by Paul Creaser
 *   Organization:  tRackIT Systems (http://trackit.systems)
 *
 * =====================================================================================
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/platform_device.h>
#include <sound/simple_card.h>
#include <linux/delay.h>
#include "snd-i2s_rpi.h"

/*
 * modified for linux 4.1.5
 * inspired by https://github.com/msperl/spi-config
 * with thanks for https://github.com/notro/rpi-source/wiki
 * as well as Florian Meier for the rpi i2s and dma drivers
 *
 * to use a differant (simple-card compatible) codec
 * change the codec name string in two places and the
 * codec_dai name string. (see codec's source file)
 *
 *
 * N.B. playback vs capture is determined by the codec choice
 * */

void device_release_callback(struct device *dev) { /*  do nothing */ };

static short rpi_platform_generation = 1;
module_param(rpi_platform_generation, short, 0);
MODULE_PARM_DESC(rpi_platform_generation, "Raspberry Pi Generation: 0 = RPiZero; 1 = RPi2, RPi3; 2 = RPi4");

static struct asoc_simple_card_info default_snd_rpi_simple_card_info = {
	.card = "snd_rpi_i2s_card", // -> snd_soc_card.name
	.name = "simple-card_codec_link", // -> snd_soc_dai_link.name
	.codec = "snd-soc-dummy", // "dmic-codec", // -> snd_soc_dai_link.codec_name
	.platform = "not-set.i2s",
	.daifmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
	.cpu_dai = {
		.name = "not-set.i2s", // -> snd_soc_dai_link.cpu_dai_name
		.sysclk = 0 
	},
	.codec_dai = {
		.name = "snd-soc-dummy-dai", //"dmic-codec", // -> snd_soc_dai_link.codec_dai_name
		.sysclk = 0 
	},
};

static struct platform_device default_snd_rpi_simple_card_device = {
	.name = "asoc-simple-card", //module alias
	.id = 0,
	.num_resources = 0,
	.dev = {
		.release = &device_release_callback,
		.platform_data = &default_snd_rpi_simple_card_info, // *HACK ALERT*
	},
};

static char *pri_platform = "3f203000.i2s";
static char *alt_platform = "20203000.i2s";
static char *pi4_platform = "7e203000.i2s";

static struct asoc_simple_card_info card_info;
static struct platform_device card_device;

int i2s_rpi_init(void)
{
	const char *dmaengine = "bcm2708-dmaengine"; //module name
	static char *card_platform;
	int ret;

	printk(KERN_INFO "snd-i2s_rpi: Version %s\n", SND_I2S_RPI_VERSION);

	card_platform = pri_platform;
	if (rpi_platform_generation == 0) {
		card_platform = alt_platform;
	} else if (rpi_platform_generation == 2) {
		card_platform = pi4_platform;
	}

	printk(KERN_INFO "snd-i2s_rpi: Setting platform to %s\n", card_platform);

	ret = request_module(dmaengine);
	// pr_alert("request module load '%s': %d\n",dmaengine, ret);

	card_info = default_snd_rpi_simple_card_info;
	card_info.platform = card_platform;
	card_info.cpu_dai.name = card_platform;

	card_device = default_snd_rpi_simple_card_device;
	card_device.dev.platform_data = &card_info;

	ret = platform_device_register(&card_device);

	//pr_alert("register platform device '%s': %d\n",snd_rpi_simple_card_device.name, ret);

	return 0;
}

void i2s_rpi_exit(void)
{
	// you'll have to sudo modprobe -r the card & codec drivers manually (first?)
	platform_device_unregister(&card_device);
	//pr_alert("i2s mic module unloaded\n");
}


module_init(i2s_rpi_init);
module_exit(i2s_rpi_exit);
MODULE_DESCRIPTION("ASoC simple-card I2S Microphone");
MODULE_AUTHOR("Jonas Höchst <hoechst@trackit.systems>");
MODULE_LICENSE("GPL v2");

