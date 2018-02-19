#include <sound/soc.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <sound/tlv.h>
#include <linux/delay.h>
#include <linux/rts_sysctl.h>

#include "rlx_codec.h"

#ifdef CONFIG_SND_SOC_RLX_DEBUG
#define DBG(args...)	pr_emerg("%s: %s", __func__, args)
#else
#define DBG(args...)
#endif

static const DECLARE_TLV_DB_SCALE(dac_vol_tlv, -6350, 50, 0);
static const DECLARE_TLV_DB_SCALE(adc_vol_tlv, -3350, 50, 0);
static const DECLARE_TLV_DB_SCALE(adc_comp_tlv, 0, 100, 0);
static const DECLARE_TLV_DB_SCALE(dmic_boost_tlv, 0, 1000, 0);
static const DECLARE_TLV_DB_SCALE(alc_ft_boost_tlv, 0, 75, 0);
static const DECLARE_TLV_DB_SCALE(hp_gain_tlv, 0, 150, 0);
static const DECLARE_TLV_DB_SCALE(ao_gain_tlv, 0, 150, 0);

static const char *rlx_codec_afe_gain_enum1[] = {"-10dB", "-7dB", "-4dB",
						"-1dB", "0dB", "2dB",
						"6dB", "-4dB"};
static const SOC_ENUM_DOUBLE_DECL(rlx_ana_adc_gain_enum1,
		RLX_REG_ADDA_ANA_CFG3, RLX_ANA_AFE_VOL_L, RLX_ANA_AFE_VOL_R,
		rlx_codec_afe_gain_enum1);

static const char *rlx_codec_afe_gain_enum2[] = {"-7.6dB", "-3.4dB", "1.6dB",
						"8.7dB", "12dB", "23.5dB",
						"29.5dB", "1.6dB"};
static const SOC_ENUM_DOUBLE_DECL(rlx_ana_adc_gain_enum2,
		RLX_REG_ADDA_ANA_CFG3, RLX_ANA_AFE_VOL_L, RLX_ANA_AFE_VOL_R,
		rlx_codec_afe_gain_enum2);

static int rlx_sync_adc_register(struct snd_soc_codec *codec)
{
	u32 reg_val;

	DBG("rlx sync adc register\n");

	reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG1);
	snd_soc_write(codec, RLX_REG_ADC_CFG1, reg_val | ((u32)0x1 << 31));

	return 0;
}

static int rlx_sync_dac_register(struct snd_soc_codec *codec)
{
	u32 reg_val;

	DBG("rlx sync dac register\n");

	reg_val = snd_soc_read(codec, RLX_REG_DAC_CFG);
	snd_soc_write(codec, RLX_REG_DAC_CFG, reg_val | ((u32)0x1 << 27));

	return 0;
}

static int rlx_adc_put_volsw(struct snd_kcontrol *kcontrol,
		  struct snd_ctl_elem_value *ucontrol)
{
	int err;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	DBG("rlx put volsw\n");

	err = snd_soc_put_volsw(kcontrol, ucontrol);
	if (err < 0)
		return err;

	rlx_sync_adc_register(codec);

	return 0;
}

static int rlx_dac_put_volsw(struct snd_kcontrol *kcontrol,
		      struct snd_ctl_elem_value *ucontrol)
{
	int err;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	DBG("rlx put volsw\n");

	err = snd_soc_put_volsw(kcontrol, ucontrol);
	if (err < 0)
		return err;

	rlx_sync_dac_register(codec);

	return 0;
}

static const struct snd_kcontrol_new rlx_soc_codec_controls[] = {
	SOC_DOUBLE_EXT_TLV("Master Playback Volume", RLX_REG_DAC_CFG,
			   RLX_DA_GAIN_L, RLX_DA_GAIN_R, 127, 0,
			   snd_soc_get_volsw, rlx_dac_put_volsw, dac_vol_tlv),
	SOC_DOUBLE("Master Playback Switch", RLX_REG_DAC_CFG,
		   RLX_DA_MUTE_L, RLX_DA_MUTE_R, 1, 1),
	SOC_SINGLE_TLV("ADC Compensate Capture Volume", RLX_REG_ADC_CFG1,
		       RLX_AD_COMP_GAIN, 3, 0, adc_comp_tlv),
	SOC_DOUBLE_EXT_TLV("Rear Dmic Capture Volume", RLX_REG_ADC_CFG1,
			   RLX_DMIC_BOOST_GAIN_L,
			   RLX_DMIC_BOOST_GAIN_R,
			   3, 0, snd_soc_get_volsw,
			   rlx_adc_put_volsw, dmic_boost_tlv),
	SOC_DOUBLE("Dmic Capture Switch", RLX_REG_ADC_CFG2,
		   RLX_DMIC_MIX_MUTE_L, RLX_DMIC_MIX_MUTE_R, 1, 1),
	SOC_DOUBLE("Amic Capture Switch", RLX_REG_ADC_CFG2,
		   RLX_AD_MIX_MUTE_L, RLX_AD_MIX_MUTE_R, 1, 1),
	SOC_SINGLE_EXT_TLV("Front Amic Capture Volume", RLX_REG_AGC_CFG5,
			   RLX_ALC_FT_BOOST, 39, 0,
			   snd_soc_get_volsw, rlx_adc_put_volsw,
			   alc_ft_boost_tlv),
	SOC_SINGLE_TLV("Headphone Playback Volume", RLX_REG_ADDA_ANA_CFG1,
			RLX_ANA_HP_GAIN_ADJ, 7, 0, hp_gain_tlv),
	SOC_SINGLE_TLV("Line Playback Volume", RLX_REG_ADDA_ANA_CFG1,
			RLX_ANA_AO_GAIN_ADJ, 7, 0, ao_gain_tlv),
};

static const struct snd_kcontrol_new rlx_ana_vol_3901a_controls[] = {
	SOC_DOUBLE_EXT_TLV("Master Capture Volume", RLX_REG_ADC_CFG1,
			RLX_AD_GAIN_L, RLX_AD_GAIN_R, 127, 0,
			snd_soc_get_volsw, rlx_adc_put_volsw, adc_vol_tlv),
	SOC_DOUBLE_EXT("Master Capture Switch", RLX_REG_ADC_CFG1,
		       RLX_AD_MUTE_L, RLX_AD_MUTE_R, 1, 1,
		       snd_soc_get_volsw, rlx_adc_put_volsw),
	SOC_ENUM("Rear Amic Capture Volume", rlx_ana_adc_gain_enum1),
};

static const struct snd_kcontrol_new rlx_ana_vol_3901c_controls[] = {
	SOC_DOUBLE_EXT_TLV("Master Capture Volume", RLX_REG_ADC_CFG1,
			RLX_AD_GAIN_L, RLX_AD_GAIN_R, 87, 0,
			snd_soc_get_volsw, rlx_adc_put_volsw, adc_vol_tlv),
	SOC_DOUBLE_EXT("Master Capture Switch", RLX_REG_ADC_CFG1,
		       RLX_AD_MUTE_L, RLX_AD_MUTE_R, 1, 1,
		       snd_soc_get_volsw, rlx_adc_put_volsw),
	SOC_ENUM("Rear Amic Capture Volume", rlx_ana_adc_gain_enum2),
};

static int rlx_codec_rwable_register(struct snd_soc_codec *codec,
				     unsigned int reg)
{
	DBG("rlx codec rwable register\n");

	switch (reg) {
	case RLX_REG_DAC_CFG:
	case RLX_REG_DAC_PDM_DFG:
	case RLX_REG_ADC_CFG1:
	case RLX_REG_ADC_CFG2:
	case RLX_REG_AGC_CFG1:
	case RLX_REG_AGC_CFG2:
	case RLX_REG_AGC_CFG3:
	case RLX_REG_AGC_CFG4:
	case RLX_REG_AGC_CFG5:
	case RLX_REG_AGC_CFG6:
	case RLX_REG_TCON_CFG:
	case RLX_REG_ADDA_ANA_CFG1:
	case RLX_REG_ADDA_ANA_CFG2:
	case RLX_REG_ADDA_ANA_CFG3:
	case RLX_REG_ADDA_ANA_CFG4:
	case RLX_REG_ADDA_ANA_CFG5:
	case RLX_REG_ADDA_OCP_CTL:
		return 1;
	default:
		return 0;
	}
}

static unsigned int rlx_soc_codec_read(struct snd_soc_codec *codec,
				       unsigned int reg)
{
	struct rlx_codec_data *codec_data = snd_soc_codec_get_drvdata(codec);

	DBG("rlx soc codec read\n");

	if (rlx_codec_rwable_register(codec, reg) == 0) {
		pr_err("invalid register address 0x%x\n", reg);
		return 0;
	}

	return readl(codec_data->addr + reg);
}

static int rlx_soc_codec_write(struct snd_soc_codec *codec, unsigned int reg,
			       unsigned int value)
{
	struct rlx_codec_data *codec_data = snd_soc_codec_get_drvdata(codec);

	DBG("rlx soc codec write\n");

	if (rlx_codec_rwable_register(codec, reg) == 0) {
		pr_err("invalid register address 0x%x\n", reg);
		return 0;
	}

	writel(value, codec_data->addr + reg);

	return 0;
}

static int rlx_codec_adc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG1);
		snd_soc_write(codec, RLX_REG_ADC_CFG1,
				reg_val & ~((u32)0x1 << RLX_AD_RST_N));

		reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG1);
		snd_soc_write(codec, RLX_REG_ADC_CFG1,
				reg_val | ((u32)0x1 << RLX_AD_RST_N));
		break;
	case SND_SOC_DAPM_POST_PMU:
		rlx_sync_adc_register(codec);
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_dac_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_DAC_CFG);
		snd_soc_write(codec, RLX_REG_DAC_CFG,
				reg_val &
				~((u32)0x1 << RLX_DA_RST_N) &
				~((u32)0x1 << RLX_DAMOD_RST_N));

		reg_val = snd_soc_read(codec, RLX_REG_DAC_CFG);
		snd_soc_write(codec, RLX_REG_DAC_CFG,
				reg_val |
				((u32)0x1 << RLX_DA_RST_N) |
				((u32)0x1 << RLX_DAMOD_RST_N));
		break;
	case SND_SOC_DAPM_POST_PMU:
		rlx_sync_dac_register(codec);
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_pdm_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_DAC_PDM_DFG);
		snd_soc_write(codec, RLX_REG_DAC_PDM_DFG,
				reg_val & ~((u32)0x1 << RLX_SDM_RST_N));

		reg_val = snd_soc_read(codec, RLX_REG_DAC_PDM_DFG);
		snd_soc_write(codec, RLX_REG_DAC_PDM_DFG,
				reg_val | ((u32)0x1 << RLX_SDM_RST_N));
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_tcon_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* set tcon rst_n */
		reg_val = snd_soc_read(codec, RLX_REG_TCON_CFG);
		snd_soc_write(codec, RLX_REG_TCON_CFG,
				reg_val &
				~((u32)0x1 << RLX_AUDIO_IP_TCON_RST_N));

		/* release tcon rst_n */
		reg_val = snd_soc_read(codec, RLX_REG_TCON_CFG);
		snd_soc_write(codec, RLX_REG_TCON_CFG,
				reg_val |
				((u32)0x1 << RLX_AUDIO_IP_TCON_RST_N));
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_power_vcm(struct snd_soc_codec *codec, int on)
{
	static int rlx_codec_power_count = 0;
	u32 reg_val;

	if (on) {
		if (rlx_codec_power_count <= 0) {
			reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
			snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
					reg_val |
					((u32)0x1 << RLX_ANA_POW_MBIAS) |
					((u32)0x1 << RLX_ANA_POW_VCM));
			msleep(200);
			reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
			snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
					reg_val |
					((u32)0x1 << RLX_ANA_VCM_READY));
			msleep(400);
		}
		rlx_codec_power_count++;
	} else {
		rlx_codec_power_count--;
		if (rlx_codec_power_count <= 0) {
			reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
			snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
					reg_val &
					~((u32)0x1 << RLX_ANA_POW_MBIAS) &
					~((u32)0x1 << RLX_ANA_POW_VCM));
			msleep(200);
			reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
			snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
					reg_val |
					((u32)0x1 << RLX_ANA_VCM_READY));
			msleep(400);
			reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
			snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
					reg_val &
					~((u32)0x1 << RLX_ANA_VCM_READY));
		}
	}

	return 0;
}

static int rlx_codec_power_depop(struct snd_soc_codec *codec, int on)
{
	u32 reg_val;

	if (on) {
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val |
				((u32)0x1 << RLX_ANA_POW_DEPOP) |
				((u32)0x1 << RLX_ANA_POW_DEPOP_CK) |
				((u32)0x1 << RLX_ANA_POW_DEPOP_CORE) |
				((u32)0x1 << RLX_ANA_POW_DEPOP_OP));
	} else {
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val &
				~((u32)0x1 << RLX_ANA_POW_DEPOP) &
				~((u32)0x1 << RLX_ANA_POW_DEPOP_CK) &
				~((u32)0x1 << RLX_ANA_POW_DEPOP_CORE) &
				~((u32)0x1 << RLX_ANA_POW_DEPOP_OP));
	}

	return 0;
}

static int rlx_codec_hpout_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val &
				~((u32)0x1 << RLX_ANA_MUTE_HPOUT_L) &
				~((u32)0x1 << RLX_ANA_MUTE_HPOUT_R));
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG1);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG1,
				reg_val &
				~((u32)0x1 << RLX_ANA_HP_NORM));
		rlx_codec_power_depop(codec, 1);
		rlx_codec_power_vcm(codec, 1);
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG1);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG1,
				reg_val |
				((u32)0x1 << RLX_ANA_HP_NORM));
		rlx_codec_power_depop(codec, 0);
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val &
				~((u32)0x1 << RLX_ANA_VCM_READY));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG1);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG1,
				reg_val &
				~((u32)0x1 << RLX_ANA_HP_NORM));
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val |
				((u32)0x1 << RLX_ANA_MUTE_HPOUT_L) |
				((u32)0x1 << RLX_ANA_MUTE_HPOUT_R));
		break;
	case SND_SOC_DAPM_POST_PMD:
		rlx_codec_power_depop(codec, 1);
		rlx_codec_power_vcm(codec, 0);
		rlx_codec_power_depop(codec, 0);
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_aout_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val &
				~((u32)0x1 << RLX_ANA_MUTE_AOUT_L) &
				~((u32)0x1 << RLX_ANA_MUTE_AOUT_R));
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG1);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG1,
				reg_val &
				~((u32)0x1 << RLX_ANA_AOUT_NORM));
		rlx_codec_power_depop(codec, 1);
		rlx_codec_power_vcm(codec, 1);
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG1);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG1,
				reg_val |
				((u32)0x1 << RLX_ANA_AOUT_NORM));
		rlx_codec_power_depop(codec, 0);
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val &
				~((u32)0x1 << RLX_ANA_VCM_READY));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG1);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG1,
				reg_val &
				~((u32)0x1 << RLX_ANA_AOUT_NORM));
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val |
				((u32)0x1 << RLX_ANA_MUTE_AOUT_L) |
				((u32)0x1 << RLX_ANA_MUTE_AOUT_R));
		break;
	case SND_SOC_DAPM_POST_PMD:
		rlx_codec_power_depop(codec, 1);
		rlx_codec_power_vcm(codec, 0);
		rlx_codec_power_depop(codec, 0);
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_ana_adc_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		rlx_codec_power_vcm(codec, 1);
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_ANA_CFG2);
		snd_soc_write(codec, RLX_REG_ADDA_ANA_CFG2,
				reg_val &
				~((u32)0x1 << RLX_ANA_VCM_READY));
		reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG2);
		snd_soc_write(codec, RLX_REG_ADC_CFG2,
				reg_val &
				~((u32)0x1 << RLX_AD_MIX_MUTE_L) &
				~((u32)0x1 << RLX_AD_MIX_MUTE_R));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG2);
		snd_soc_write(codec, RLX_REG_ADC_CFG2,
				reg_val |
				((u32)0x1 << RLX_AD_MIX_MUTE_L) |
				((u32)0x1 << RLX_AD_MIX_MUTE_R));
		break;
	case SND_SOC_DAPM_POST_PMD:
		rlx_codec_power_vcm(codec, 0);
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_dmic_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG2);
		snd_soc_write(codec, RLX_REG_ADC_CFG2,
				reg_val &
				~((u32)0x1 << RLX_DMIC_MIX_MUTE_L) &
				~((u32)0x1 << RLX_DMIC_MIX_MUTE_R));
		break;
	case SND_SOC_DAPM_PRE_PMD:
		reg_val = snd_soc_read(codec, RLX_REG_ADC_CFG2);
		snd_soc_write(codec, RLX_REG_ADC_CFG2,
				reg_val |
				((u32)0x1 << RLX_DMIC_MIX_MUTE_L) |
				((u32)0x1 << RLX_DMIC_MIX_MUTE_R));
		break;
	default:
		pr_err("invalid event\n");
		break;
	}

	return 0;
}

static int rlx_codec_amic_intern_power(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
#ifdef CONFIG_SND_SOC_RLX_AMIC_IN_LDO
	u32 reg_val;
	struct snd_soc_codec *codec = w->codec;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_OCP_CTL);
		snd_soc_write(codec, RLX_REG_ADDA_OCP_CTL,
				reg_val | ((u32)0x3 << RLX_ANA_POW_LDO));
		break;
	case SND_SOC_DAPM_POST_PMD:
		reg_val = snd_soc_read(codec, RLX_REG_ADDA_OCP_CTL);
		snd_soc_write(codec, RLX_REG_ADDA_OCP_CTL,
				reg_val & ~((u32)0x3 << RLX_ANA_POW_LDO));
		break;
	default:
		pr_err("invalid event\n");
		break;
	}
#endif
	return 0;
}

static const char *rlx_codec_out_l_src[] = {"DAC_L", "DAC_R",
						"AIN_L", "AIN_R"};
static const char *rlx_codec_out_r_src[] = {"DAC_R", "DAC_L",
						"AIN_R", "AIN_L"};

static const SOC_ENUM_SINGLE_DECL(rlx_codec_aout_l_enum, RLX_REG_ADDA_ANA_CFG2,
				RLX_ANA_MUX_AOUT_L, rlx_codec_out_l_src);

static const struct snd_kcontrol_new rlx_codec_aout_l_mux =
	SOC_DAPM_ENUM("AOUT left channel source", rlx_codec_aout_l_enum);

static const SOC_ENUM_SINGLE_DECL(rlx_codec_aout_r_enum, RLX_REG_ADDA_ANA_CFG2,
				RLX_ANA_MUX_AOUT_R, rlx_codec_out_r_src);

static const struct snd_kcontrol_new rlx_codec_aout_r_mux =
	SOC_DAPM_ENUM("AOUT right channel source", rlx_codec_aout_r_enum);

static const SOC_ENUM_SINGLE_DECL(rlx_codec_hpout_l_enum, RLX_REG_ADDA_ANA_CFG3,
				RLX_ANA_MUX_HPOUT_L, rlx_codec_out_l_src);

static const struct snd_kcontrol_new rlx_codec_hpout_l_mux =
	SOC_DAPM_ENUM("HPOUT left channel source", rlx_codec_hpout_l_enum);

static const SOC_ENUM_SINGLE_DECL(rlx_codec_hpout_r_enum, RLX_REG_ADDA_ANA_CFG3,
				RLX_ANA_MUX_HPOUT_R, rlx_codec_out_r_src);

static const struct snd_kcontrol_new rlx_codec_hpout_r_mux =
	SOC_DAPM_ENUM("HPOUT right channel source", rlx_codec_hpout_r_enum);

static const struct snd_soc_dapm_widget rlx_soc_codec_dapm_widgets[] = {
	/* Capture widgets */
	SND_SOC_DAPM_AIF_OUT("RLXD Capture", "RLX-CODEC Digital Capture",
			0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("RLXA Capture", "RLX-CODEC Analog Capture",
			0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_ADC("ADC Filter", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_SUPPLY("ADC Filter CLK", RLX_REG_TCON_CFG, RLX_AD_CLK_EN,
			0, rlx_codec_adc_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("DMIC CLK", RLX_REG_TCON_CFG, RLX_DMIC_CLK_EN,
			0, rlx_codec_dmic_event,
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU),

	/* analog widgets */
	SND_SOC_DAPM_PGA("Analog Inputbf", RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_INPUTBF, 0, NULL, 0),
	SND_SOC_DAPM_ADC_E("Analog ADC Filter", NULL, RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_ADC, 0, rlx_codec_ana_adc_event,
			SND_SOC_DAPM_PRE_PMD | SND_SOC_DAPM_POST_PMU |
			SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_SUPPLY("ADC ANA CLK", RLX_REG_TCON_CFG, RLX_AD_ANA_CLK_EN,
			0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("AMIC INTERN POWER", SND_SOC_NOPM, 0, 0,
			rlx_codec_amic_intern_power,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD),

	/* Playback widgets */
	SND_SOC_DAPM_AIF_IN("RLXD Playback", "RLX-CODEC Digital Playback",
			0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("RLXA Playback", "RLX-CODEC Analog Playback",
			0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC Filter", NULL, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_SUPPLY("DAC Filter CLK", RLX_REG_TCON_CFG, RLX_DA_CLK_EN,
			0, rlx_codec_dac_event,
			SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMU),
	SND_SOC_DAPM_SUPPLY("PDM CLK", RLX_REG_TCON_CFG, RLX_PDM_CLK_EN,
			0, rlx_codec_pdm_event, SND_SOC_DAPM_PRE_PMU),
	SND_SOC_DAPM_SUPPLY("TCON CLK", RLX_REG_TCON_CFG, RLX_AUDIO_IP_TCON_EN,
			0, rlx_codec_tcon_event,
			SND_SOC_DAPM_PRE_PMU),

	/* analog widgets */
	SND_SOC_DAPM_DAC("Analog DAC Filter", NULL, RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_DAC, 0),
	SND_SOC_DAPM_PGA("Analog DF2SE", RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_DF2SE, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY("Dacvref Power", RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_DACVREF, 0, NULL, 0),
	SND_SOC_DAPM_MUX("AOUT MUXL", SND_SOC_NOPM, 0, 0,
			&rlx_codec_aout_l_mux),
	SND_SOC_DAPM_MUX("AOUT MUXR", SND_SOC_NOPM, 0, 0,
			&rlx_codec_aout_r_mux),
	SND_SOC_DAPM_MUX("HPOUT MUXL", SND_SOC_NOPM, 0, 0,
			&rlx_codec_hpout_l_mux),
	SND_SOC_DAPM_MUX("HPOUT MUXR", SND_SOC_NOPM, 0, 0,
			&rlx_codec_hpout_r_mux),
	SND_SOC_DAPM_PGA_S("AOUT Amp", 1, RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_AOUT, 0, rlx_codec_aout_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD |
			SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_PGA_S("HPOUT Amp", 1, RLX_REG_ADDA_ANA_CFG2,
			RLX_ANA_POW_HPOUT, 0, rlx_codec_hpout_event,
			SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD |
			SND_SOC_DAPM_POST_PMD),

	/* Output Lines */
	SND_SOC_DAPM_OUTPUT("PDM"),
	SND_SOC_DAPM_INPUT("DMIC"),

	SND_SOC_DAPM_OUTPUT("LOUT"),
	SND_SOC_DAPM_OUTPUT("HP"),

	SND_SOC_DAPM_INPUT("LIN"),
	SND_SOC_DAPM_INPUT("AMIC"),
};

static const struct snd_soc_dapm_route rlx_soc_codec_dapm_routes[] = {
	/* PDM */
	{"DAC Filter", NULL, "RLXD Playback"},
	{"DAC Filter", NULL, "RLXA Playback"},
	{"DAC Filter", NULL, "DAC Filter CLK"},
	{"PDM", NULL, "DAC Filter"},
	{"PDM", NULL, "PDM CLK"},
	{"PDM CLK", NULL, "TCON CLK"},
	{"DAC Filter CLK", NULL, "TCON CLK"},

	/* DMIC */
	{"RLXD Capture", NULL, "ADC Filter"},
	{"RLXA Capture", NULL, "ADC Filter"},
	{"ADC Filter", NULL, "ADC Filter CLK"},
	{"ADC Filter", NULL, "DMIC"},
	{"DMIC", NULL, "DMIC CLK"},
	{"DMIC CLK", NULL, "TCON CLK"},
	{"ADC Filter CLK", NULL, "TCON CLK"},

	/* LOUT & HP */
	{"Analog DAC Filter", NULL, "DAC Filter"},
	{"Analog DAC Filter", NULL, "Dacvref Power"},
	{"Analog DF2SE", NULL, "Analog DAC Filter"},

	{"AOUT MUXL", "DAC_L", "Analog DF2SE"},
	{"AOUT MUXL", "DAC_R", "Analog DF2SE"},
	{"AOUT MUXL", "AIN_L", "Analog Inputbf"},
	{"AOUT MUXL", "AIN_R", "Analog Inputbf"},

	{"AOUT MUXR", "DAC_R", "Analog DF2SE"},
	{"AOUT MUXR", "DAC_L", "Analog DF2SE"},
	{"AOUT MUXL", "AIN_R", "Analog Inputbf"},
	{"AOUT MUXL", "AIN_L", "Analog Inputbf"},

	{"HPOUT MUXL", "DAC_L", "Analog DF2SE"},
	{"HPOUT MUXL", "DAC_R", "Analog DF2SE"},
	{"HPOUT MUXL", "AIN_L", "Analog Inputbf"},
	{"HPOUT MUXL", "AIN_R", "Analog Inputbf"},

	{"HPOUT MUXR", "DAC_R", "Analog DF2SE"},
	{"HPOUT MUXR", "DAC_L", "Analog DF2SE"},
	{"HPOUT MUXL", "AIN_R", "Analog Inputbf"},
	{"HPOUT MUXL", "AIN_L", "Analog Inputbf"},

	{"AOUT Amp", NULL, "AOUT MUXL"},
	{"AOUT Amp", NULL, "AOUT MUXR"},

	{"LOUT", NULL, "AOUT Amp"},

	{"HPOUT Amp", NULL, "HPOUT MUXL"},
	{"HPOUT Amp", NULL, "HPOUT MUXR"},

	{"HP", NULL, "HPOUT Amp"},

	/* AMIC & LIN */
	{"ADC ANA CLK", NULL, "TCON CLK"},
	{"ADC Filter", NULL, "Analog ADC Filter"},
	{"Analog ADC Filter", NULL, "ADC ANA CLK"},
	{"Analog ADC Filter", NULL, "Analog Inputbf"},

	{"Analog Inputbf", NULL, "LIN"},
	{"Analog Inputbf", NULL, "AMIC"},
	{"AMIC", NULL, "AMIC INTERN POWER"},
};

static int rlx_codec_dai_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	DBG("rlx codec dai prepare\n");

	if (dai->id == RLX_CODEC_DIGITAL) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			snd_soc_dapm_enable_pin(dapm, "PDM");
		else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			snd_soc_dapm_enable_pin(dapm, "DMIC");
	} else if (dai->id == RLX_CODEC_ANALOG) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_dapm_enable_pin(dapm, "HP");
			snd_soc_dapm_enable_pin(dapm, "LOUT");
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			snd_soc_dapm_enable_pin(dapm, "AMIC");
			snd_soc_dapm_enable_pin(dapm, "LIN");
		}
	}

	snd_soc_dapm_sync(dapm);

	return 0;
}

static void rlx_codec_dai_shutdown(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	DBG("rlx codec dai shutdown\n");

	if (dai->id == RLX_CODEC_DIGITAL) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			snd_soc_dapm_disable_pin(dapm, "PDM");
		else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			snd_soc_dapm_disable_pin(dapm, "DMIC");
	} else if (dai->id == RLX_CODEC_ANALOG) {
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			snd_soc_dapm_disable_pin(dapm, "HP");
			snd_soc_dapm_disable_pin(dapm, "LOUT");
		} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			snd_soc_dapm_disable_pin(dapm, "AMIC");
			snd_soc_dapm_disable_pin(dapm, "LIN");
		}
	}

	snd_soc_dapm_sync(dapm);
}

static int rlx_codec_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct rlx_codec_data *codec_data = snd_soc_codec_get_drvdata(codec);
	u32 reg_val, val;

	DBG("rlx codec dai hw params\n");

	reg_val = readl(codec_data->addr + RLX_REG_TCON_CFG);
	switch (params_rate(params)) {
	case 8000:
		val = 0x07;
		break;
	case 11025:
		val = 0x0E;
		break;
	case 12000:
		val = 0x06;
		break;
	case 16000:
		val = 0x05;
		break;
	case 22050:
		val = 0x0C;
		break;
	case 24000:
		val = 0x04;
		break;
	case 32000:
		val = 0x03;
		break;
	case 44100:
		val = 0x08;
		break;
	case 48000:
		val = 0x00;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg_val = reg_val & ~((u32) 0xF << RLX_DAC_SAMPLE_RATE);
		reg_val = reg_val | (val << RLX_DAC_SAMPLE_RATE);
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		reg_val = reg_val & ~((u32) 0xF << RLX_ADC_SAMPLE_RATE);
		reg_val = reg_val | (val << RLX_ADC_SAMPLE_RATE);
	} else {
		return -EINVAL;
	}

	writel(reg_val, codec_data->addr + RLX_REG_TCON_CFG);

	return 0;
}

static struct snd_soc_dai_ops rlx_codec_dai_ops = {
	.startup = rlx_codec_dai_startup,
	.hw_params = rlx_codec_dai_hw_params,
	.shutdown = rlx_codec_dai_shutdown,
};

static struct snd_soc_dai_driver rlx_codec_dai[] = {
	{
		.name = "rlx-codec-analog",
		.id = RLX_CODEC_ANALOG,
		.playback = {
			.stream_name = "RLX-CODEC Analog Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_CODEC_RATES,
			.formats = RLX_CODEC_FORMATS,
		},
		.capture = {
			.stream_name = "RLX-CODEC Analog Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_CODEC_RATES,
			.formats = RLX_CODEC_FORMATS,
		},
		.ops = &rlx_codec_dai_ops,
	},
	{
		.name = "rlx-codec-digital",
		.id = RLX_CODEC_DIGITAL,
		.playback = {
			.stream_name = "RLX-CODEC Digital Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_CODEC_RATES,
			.formats = RLX_CODEC_FORMATS,
		},
		.capture = {
			.stream_name = "RLX-CODEC Digital Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = RLX_CODEC_RATES,
			.formats = RLX_CODEC_FORMATS,
		},
		.ops = &rlx_codec_dai_ops,
	},
};

static int rlx_soc_codec_probe(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_add_codec_controls(codec, rlx_soc_codec_controls,
                        ARRAY_SIZE(rlx_soc_codec_controls));

	if (RTS_SOC_HW_ID.hw_ver == HW_ID_VER_RTS3901 &&
			RTS_SOC_HW_ID.hw_rev == HW_ID_REV_C)
		snd_soc_add_codec_controls(codec, rlx_ana_vol_3901c_controls,
                        ARRAY_SIZE(rlx_ana_vol_3901c_controls));
	else
		snd_soc_add_codec_controls(codec, rlx_ana_vol_3901a_controls,
                        ARRAY_SIZE(rlx_ana_vol_3901a_controls));

	snd_soc_dapm_disable_pin(dapm, "PDM");
	snd_soc_dapm_disable_pin(dapm, "DMIC");
	snd_soc_dapm_disable_pin(dapm, "HP");
	snd_soc_dapm_disable_pin(dapm, "LOUT");
	snd_soc_dapm_disable_pin(dapm, "AMIC");
	snd_soc_dapm_disable_pin(dapm, "LIN");

	snd_soc_dapm_sync(dapm);

#ifdef CONFIG_SND_SOC_RLX_INTERN_CODEC_AMIC
	snd_soc_update_bits(codec, RLX_REG_ADDA_ANA_CFG3,
			(0x7 << RLX_ANA_AFE_VOL_L) |
			(0x7 << RLX_ANA_AFE_VOL_R),
			(0x6 << RLX_ANA_AFE_VOL_L) |
			(0x6 << RLX_ANA_AFE_VOL_R));
#endif

	return 0;
}

static int rlx_soc_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver rlx_soc_codec_driver = {
	.probe = rlx_soc_codec_probe,
	.remove = rlx_soc_codec_remove,
	.read = rlx_soc_codec_read,
	.write = rlx_soc_codec_write,
	.dapm_widgets = rlx_soc_codec_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(rlx_soc_codec_dapm_widgets),
	.dapm_routes = rlx_soc_codec_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(rlx_soc_codec_dapm_routes),
};

static int rlx_codec_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;
	struct rlx_codec_data *codec_data;

	DBG("rlx codec probe\n");

	codec_data =
	    devm_kzalloc(&pdev->dev, sizeof(struct rlx_codec_data),
			 GFP_KERNEL);
	if (codec_data == NULL) {
		pr_err("Unable to alloc codec data\n");
		return -ENOMEM;
	}
	codec_data->pdev = pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("Unable to get codec address\n");
		ret = -ENXIO;
		goto mem_err;
	}

	if (!request_mem_region(res->start, resource_size(res),
				"rlx-codec")) {
		pr_err("Unable to request mem region\n");
		ret = -EBUSY;
		goto mem_err;
	}

	codec_data->base = res->start;
	codec_data->size = res->end - res->start + 1;

	codec_data->addr = ioremap(codec_data->base, codec_data->size);
	if (codec_data->addr == NULL) {
		pr_err("failed to ioregmap\n");
		ret = -ENXIO;
		goto err;
	}

	ret = snd_soc_register_codec(&pdev->dev, &rlx_soc_codec_driver,
				     rlx_codec_dai,
				     ARRAY_SIZE(rlx_codec_dai));
	if (ret) {
		pr_err("register codec failed\n");
		goto io_err;
	}

	dev_set_drvdata(&pdev->dev, codec_data);

	return 0;

io_err:
	iounmap(codec_data->addr);
err:
	release_mem_region(codec_data->base, resource_size(res));
mem_err:
	devm_kfree(&pdev->dev, codec_data);
	codec_data = NULL;

	return ret;
}

static int rlx_codec_remove(struct platform_device *pdev)
{
	struct rlx_codec_data *codec_data;
	struct resource *res;

	codec_data = dev_get_drvdata(&pdev->dev);
	iounmap(codec_data->addr);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res)
		release_mem_region(codec_data->base, resource_size(res));
	snd_soc_unregister_codec(&pdev->dev);
	devm_kfree(&pdev->dev, codec_data);
	dev_set_drvdata(&pdev->dev, NULL);
	codec_data = NULL;

	return 0;
}

static struct platform_driver rlx_codec_driver = {
	.driver = {
		.name = "rlx-codec",
		.owner = THIS_MODULE,
	},
	.probe = rlx_codec_probe,
	.remove = rlx_codec_remove,
};

static int __init rlx_codec_init(void)
{
	DBG("rlx codec init\n");

	return platform_driver_register(&rlx_codec_driver);
}
module_init(rlx_codec_init);

static void __exit rlx_codec_exit(void)
{
	platform_driver_unregister(&rlx_codec_driver);
}
module_exit(rlx_codec_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wind_Han <wind_han@realsil.com.cn>");
MODULE_DESCRIPTION("Realtek RLX ALSA soc codec driver");
