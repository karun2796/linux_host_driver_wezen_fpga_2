#ifdef CONF_SUPPORT
/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/etherdevice.h>
#include "fmac_api.h"
#include "lnx_fmac_dbgfs_if.h"
#include "lnx_net_stack.h"
#include "lnx_util.h"

#ifndef CONFIG_NRF700X_RADIO_TEST
extern char* rf_params;
#endif /* !CONFIG_NRF700X_RADIO_TEST */
extern unsigned int phy_calib;

#define MAX_CONF_BUF_SIZE ((NRF_WIFI_RF_PARAMS_SIZE * 2) + 50)
#define MAX_ERR_STR_SIZE 80

static __always_inline unsigned char param_get_val(unsigned char *buf,
						   unsigned char *str,
						   unsigned long *val)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		/* To handle the fixed rate 5.5Mbps case */
		if (!strncmp(temp, "5.5", 3)) {
			*val = 5;
			return 1;
		} else if (!kstrtoul(temp, 0, val)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}


static __always_inline unsigned char param_get_sval(unsigned char *buf,
						    unsigned char *str,
						    long *sval)
{
	unsigned char *temp = NULL;

	if (strstr(buf, str)) {
		temp = strstr(buf, "=") + 1;

		/* To handle the fixed rate 5.5Mbps case */
		if (!strncmp(temp, "5.5", 3)) {
			*sval = 55;
			return 1;
		} else if (!kstrtol(temp, 0, sval)) {
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}


static __always_inline unsigned char param_get_match(unsigned char *buf,
						     unsigned char *str)
{

	if (strstr(buf, str))
		return 1;
	else
		return 0;
}


static __always_inline char *rate_to_string(int rate,
					    unsigned char tput_mode)
{
	if (rate == -1)
		return "Disabled";
	else if (tput_mode == RPU_TPUT_MODE_VHT)
		return "VHT";
	else if (tput_mode == RPU_TPUT_MODE_HT)
		return "HT";
	else if (tput_mode == RPU_TPUT_MODE_HE_SU)
		return "HE_SU";
	else if (tput_mode == RPU_TPUT_MODE_HE_ER_SU)
		return "HE_ER_SU";
	else
		return "Legacy";
}

static __always_inline bool check_valid_data_rate(unsigned char tput_mode,
						  unsigned int nss,
						  int dr)
{
	bool is_mcs = dr & 0x80;
	bool ret = false;

	if (dr == -1)
		return true;

	if (is_mcs) {
		dr = dr & 0x7F;

		if (tput_mode & RPU_TPUT_MODE_HT) {
			if (nss == 2) {
				if ((dr >= 8) && (dr <= 15))
					ret = true;
				else
					pr_err("Invalid MIMO HT MCS: %d\n",
					       dr);
			}
			if (nss == 1) {
				if ((dr >= 0) && (dr <= 7))
					ret = true;
				else
					pr_err("Invalid SISO HT MCS: %d\n",
					       dr);
			}

		} else if (tput_mode == RPU_TPUT_MODE_VHT) {
			if ((dr >= 0 && dr <= 9))
				ret = true;
			else
				pr_err("Invalid VHT MCS value: %d\n", dr);
		} else if (tput_mode == RPU_TPUT_MODE_HE_SU) {
			if ((dr >= 0 && dr <= 7))
				ret = true;
			else
				pr_err("Invalid HE_SU MCS value: %d\n", dr);
		} else if (tput_mode == RPU_TPUT_MODE_HE_ER_SU) {
			if ((dr >= 0 && dr <= 2))
				ret = true;
			else
				pr_err("Invalid HE_ER_SU MCS value: %d\n", dr);
		} else
			pr_err("%s: Invalid throughput mode: %d\n", __func__, dr);
	} else {
		if (tput_mode != RPU_TPUT_MODE_LEGACY) {
			ret = false;
			pr_err("Invalid rate_flags for legacy: %d\n", dr);
		} else {
			if ((dr == 1) ||
			    (dr == 2) ||
			    (dr == 55) ||
			    (dr == 11) ||
			    (dr == 6) ||
			    (dr == 9) ||
			    (dr == 12) ||
			    (dr == 18) ||
			    (dr == 24) ||
			    (dr == 36) ||
			    (dr == 48) ||
			    (dr == 54) ||
			    (dr == -1)) {
				ret = true;
			} else
				pr_err("Invalid Legacy Rate value: %d\n", dr);
		}
	}

	return ret;
}


static __always_inline bool check_valid_channel(unsigned char chan_num)
{
	if (((chan_num >= 1) && (chan_num <= 14)) ||
	    (chan_num == 36) ||
	    (chan_num == 38) ||
	    (chan_num == 40) ||
	    (chan_num == 44) ||
	    (chan_num == 46) ||
	    (chan_num == 48) ||
	    (chan_num == 52) ||
	    (chan_num == 54) ||
	    (chan_num == 56) ||
	    (chan_num == 60) ||
	    (chan_num == 62) ||
	    (chan_num == 64) ||
	    (chan_num == 100) ||
	    (chan_num == 102) ||
	    (chan_num == 104) ||
	    (chan_num == 108) ||
	    (chan_num == 110) ||
	    (chan_num == 112) ||
	    (chan_num == 116) ||
	    (chan_num == 118) ||
	    (chan_num == 120) ||
	    (chan_num == 124) ||
	    (chan_num == 126) ||
	    (chan_num == 128) ||
	    (chan_num == 132) ||
	    (chan_num == 134) ||
	    (chan_num == 136) ||
	    (chan_num == 140) ||
	    (chan_num == 142) ||
	    (chan_num == 144) ||
	    (chan_num == 149) ||
	    (chan_num == 151) ||
	    (chan_num == 153) ||
	    (chan_num == 157) ||
	    (chan_num == 159) ||
	    (chan_num == 161) ||
	    (chan_num == 165))
	    return true;

	return false;
}


static __always_inline int check_channel_settings(unsigned char tput_mode,
						  struct chan_params *chan)
{
	int ret_val = 0;
	char err_str[MAX_ERR_STR_SIZE];

	switch (tput_mode) {
		case RPU_TPUT_MODE_LEGACY:
			switch (chan->bw) {
				case RPU_CH_BW_20:
#ifndef SOC_CALDER
					if (chan->sec_20_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "Legacy, sec_20_offset has been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_40_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "Legacy, sec_40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}
#endif /* !SOC_CALDER */
					break;
#ifndef SOC_CALDER
				case RPU_CH_BW_40:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Legacy, bw = 40 has been set.\n");
					ret_val = -1;
					goto err;
#ifdef notyet
				case RPU_CH_BW_80:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Legacy, bw = 80 has been set.\n");
					ret_val = -1;
#endif
					break;

#endif /* !SOC_CALDER */
				default:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Legacy, bw has not been set.\n");
					ret_val = -1;
			}
			break;
		case RPU_TPUT_MODE_HT:
			switch (chan->bw) {
				case RPU_CH_BW_20:
#ifndef SOC_CALDER
					if (chan->sec_20_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HT, sec_20_offset has been set.\n");
						ret_val = -1;
						goto err;
					}
					if (chan->sec_40_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HT, sec_40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}
#endif /* !SOC_CALDER */
					break;
#ifndef SOC_CALDER
				case RPU_CH_BW_40:
					if (chan->sec_40_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HT, sec_40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_20_offset == 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HT, bw(40), sec_20_offset has not been set.\n");
						ret_val = -1;
						goto err;
					}

					if ((chan->primary_num >= 1) && (chan->primary_num <= 14)) {
						if ((chan->primary_num >= 1) && (chan->primary_num <= 4)) {
							if (chan->sec_20_offset != 1) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "2.4G, sec_20_offset is not availble.\n");
								ret_val = -1;
								goto err;
							}
						} else if ((chan->primary_num >= 8) && (chan->primary_num <= 14)) {
							if (chan->sec_20_offset != -1) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "2.4G, sec_20_offset is not available.\n");
								ret_val = -1;
								goto err;
							}
						}
					} else {
						if (chan->sec_20_offset == 1) {
							if ((chan->primary_num == 40) || (chan->primary_num == 48) ||
							    (chan->primary_num == 56) || (chan->primary_num == 64) ||
							    (chan->primary_num == 104) || (chan->primary_num == 112) ||
							    (chan->primary_num == 120) || (chan->primary_num == 128) ||
							    (chan->primary_num == 136) || (chan->primary_num == 144) ||
							    (chan->primary_num == 153) || (chan->primary_num == 161)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HT, sec_20_offset not available.\n");
								ret_val = -1;
								goto err;
							}
						} else if (chan->sec_20_offset == -1) {
							if ((chan->primary_num == 36) || (chan->primary_num == 44) ||
							    (chan->primary_num == 52) || (chan->primary_num == 60) ||
							    (chan->primary_num == 100) || (chan->primary_num == 108) ||
							    (chan->primary_num == 116) || (chan->primary_num == 124) ||
							    (chan->primary_num == 132) || (chan->primary_num == 140) ||
							    (chan->primary_num == 149) || (chan->primary_num == 157)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HT, sec_20_offset not available.\n");
								ret_val = -1;
								goto err;
							}
						}
					}
					break;
#ifdef notyet
				case RPU_CH_BW_80:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "HT, chnl = %d, bw = %d has been set.\n",
						 chan->primary_num,
						 chan->bw);
					ret_val = -1;
					break;
#endif
#endif /* !SOC_CALDER */
				default:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "HT, chnl = %d, proper bw has not been set.\n",
						 chan->primary_num);
					ret_val = -1;
			}
			break;
		case RPU_TPUT_MODE_VHT:
			if ((chan->primary_num >= 1) && (chan->primary_num <= 14)) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "VHT, in 2.4 G has been set.\n");
				ret_val = -1;
				goto err;
			}

			switch (chan->bw) {
				case RPU_CH_BW_20:
#ifndef SOC_CALDER
					if ((chan->sec_20_offset != 0) ||
					    (chan->sec_40_offset != 0)) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "VHT, bw=20, sec_20/40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}
#endif /* !SOC_CALDER */
					break;
#ifndef SOC_CALDER
				case RPU_CH_BW_40:
					if (chan->sec_40_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "VHT, bw=40, sec_40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_20_offset == 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "VHT, bw=40, sec_20_offset has not been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_20_offset == 1) {
						if ((chan->primary_num == 40) || (chan->primary_num == 48) ||
						    (chan->primary_num == 56) || (chan->primary_num == 64) ||
						    (chan->primary_num == 104) || (chan->primary_num == 112) ||
						    (chan->primary_num == 120) || (chan->primary_num == 128) ||
						    (chan->primary_num == 136) || (chan->primary_num == 144) ||
						    (chan->primary_num == 153) || (chan->primary_num == 161)) {
							snprintf(err_str,
								 MAX_ERR_STR_SIZE,
								 "VHT, (20+) is not availabl for chnl\n");
							ret_val = -1;
							goto err;
						}
					} else if (chan->sec_20_offset == -1) {
						if ((chan->primary_num == 36) || (chan->primary_num == 44) ||
						    (chan->primary_num == 52) || (chan->primary_num == 60) ||
						    (chan->primary_num == 100) || (chan->primary_num == 108) ||
						    (chan->primary_num == 116) || (chan->primary_num == 124) ||
						    (chan->primary_num == 132) || (chan->primary_num == 140) ||
						    (chan->primary_num == 149) || (chan->primary_num == 157)) {
							snprintf(err_str,
								 MAX_ERR_STR_SIZE,
								 "VHT, (20-) is not available for chnl.\n");
							ret_val = -1;
							goto err;
						}
					}

					break;
#ifdef notyet
				case RPU_CH_BW_80:
					if (chan->sec_40_offset == 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "VHT, sec_40_offset has not been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_20_offset == 1) {
						if (chan->sec_40_offset == 1) {
							if ((chan->primary_num == 44) || (chan->primary_num == 60) ||
							    (chan->primary_num == 108) || (chan->primary_num == 124) ||
							    (chan->primary_num == 140) || (chan->primary_num == 157)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "VHT, (20+) & (40+) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						} else if (chan->sec_40_offset == -1) {
							if ((chan->primary_num == 36) || (chan->primary_num == 52) ||
							    (chan->primary_num == 100) || (chan->primary_num == 116) ||
							    (chan->primary_num == 132) || (chan->primary_num == 149)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "VHT, (20+) & (40-) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						}
					} else if (chan->sec_20_offset == -1) {
						if (chan->sec_40_offset == 1) {
							if ((chan->primary_num == 48) || (chan->primary_num == 64) ||
							    (chan->primary_num == 112) || (chan->primary_num == 128) ||
							    (chan->primary_num == 144) || (chan->primary_num == 161)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "VHT, (20-) & (40+) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						} else if (chan->sec_40_offset == -1) {
							if ((chan->primary_num == 40) || (chan->primary_num == 56) ||
							    (chan->primary_num == 104) || (chan->primary_num == 120) ||
							    (chan->primary_num == 136) || (chan->primary_num == 153)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "VHT, (20-) & (40-) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						}
					}
					break;
#endif
#endif /* !SOC_CALDER */
				default:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "VHT, chnl = %d, bw=%d has not been set\n",
						 chan->primary_num,
						 chan->bw);
					ret_val = -1;
			}
			break;
		case RPU_TPUT_MODE_HE_SU:
		case RPU_TPUT_MODE_HE_ER_SU:
			switch (chan->bw) {
				case RPU_CH_BW_20:
#ifndef SOC_CALDER
					if (chan->sec_20_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HE, sec_20_offset has been set.\n");
						ret_val = -1;
						goto err;
					}
					if (chan->sec_40_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HE, sec_40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}
#endif /* !SOC_CALDER */
					break;
#ifndef SOC_CALDER
				case RPU_CH_BW_40:
					if (chan->sec_40_offset != 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HE, sec_40_offset has been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_20_offset == 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HE, bw(40), sec_20_offset has not been set.\n");
						ret_val = -1;
						goto err;
					}

					if ((chan->primary_num >= 1) && (chan->primary_num <= 14)) {
						if ((chan->primary_num >= 1) && (chan->primary_num <= 4)) {
							if (chan->sec_20_offset != 1) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "2.4G, sec_20_offset is not availble.\n");
								ret_val = -1;
								goto err;
							}
						} else if ((chan->primary_num >= 8) && (chan->primary_num <= 14)) {
							if (chan->sec_20_offset != -1) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "2.4G, sec_20_offset is not available.\n");
								ret_val = -1;
								goto err;
							}
						}
					} else {
						if (chan->sec_20_offset == 1) {
							if ((chan->primary_num == 40) || (chan->primary_num == 48) ||
							    (chan->primary_num == 56) || (chan->primary_num == 64) ||
							    (chan->primary_num == 104) || (chan->primary_num == 112) ||
							    (chan->primary_num == 120) || (chan->primary_num == 128) ||
							    (chan->primary_num == 136) || (chan->primary_num == 144) ||
							    (chan->primary_num == 153) || (chan->primary_num == 161)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HE, sec_20_offset not available.\n");
								ret_val = -1;
								goto err;
							}
						} else if (chan->sec_20_offset == -1) {
							if ((chan->primary_num == 36) || (chan->primary_num == 44) ||
							    (chan->primary_num == 52) || (chan->primary_num == 60) ||
							    (chan->primary_num == 100) || (chan->primary_num == 108) ||
							    (chan->primary_num == 116) || (chan->primary_num == 124) ||
							    (chan->primary_num == 132) || (chan->primary_num == 140) ||
							    (chan->primary_num == 149) || (chan->primary_num == 157)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HE, sec_20_offset not available.\n");
								ret_val = -1;
								goto err;
							}
						}
					}
					break;
#ifdef notyet
				case RPU_CH_BW_80:
					if (chan->sec_40_offset == 0) {
						snprintf(err_str,
							 MAX_ERR_STR_SIZE,
							 "HE, sec_40_offset has not been set.\n");
						ret_val = -1;
						goto err;
					}

					if (chan->sec_20_offset == 1) {
						if (chan->sec_40_offset == 1) {
							if ((chan->primary_num == 44) || (chan->primary_num == 60) ||
							    (chan->primary_num == 108) || (chan->primary_num == 124) ||
							    (chan->primary_num == 140) || (chan->primary_num == 157)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HE, (20+) & (40+) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						} else if (chan->sec_40_offset == -1) {
							if ((chan->primary_num == 36) || (chan->primary_num == 52) ||
							    (chan->primary_num == 100) || (chan->primary_num == 116) ||
							    (chan->primary_num == 132) || (chan->primary_num == 149)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HE, (20+) & (40-) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						}
					} else if (chan->sec_20_offset == -1) {
						if (chan->sec_40_offset == 1) {
							if ((chan->primary_num == 48) || (chan->primary_num == 64) ||
							    (chan->primary_num == 112) || (chan->primary_num == 128) ||
							    (chan->primary_num == 144) || (chan->primary_num == 161)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HE, (20-) & (40+) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						} else if (chan->sec_40_offset == -1) {
							if ((chan->primary_num == 40) || (chan->primary_num == 56) ||
							    (chan->primary_num == 104) || (chan->primary_num == 120) ||
							    (chan->primary_num == 136) || (chan->primary_num == 153)) {
								snprintf(err_str,
									 MAX_ERR_STR_SIZE,
									 "HE, (20-) & (40-) not available for chnl.\n");
								ret_val = -1;
								goto err;
							}
						}
					}
					break;
#endif
				default:
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "HE, chnl = %d, bw=%d has not been set\n",
						 chan->primary_num,
						 chan->bw);
					ret_val = -1;
#endif /* !SOC_CALDER */
			}
			break;
			break;
		default:
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid tput_mode = (%d) has been set\n",
				 tput_mode);
			ret_val = -1;
	}
err:

	if (ret_val) {
		pr_err("Error condition: %s\n", err_str);
		return -1;
	}

	return 0;
}


static int nrf_wifi_lnx_wlan_fmac_conf_disp(struct seq_file *m, void *v)
{
	int i = 0;
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)m->private;
	struct rpu_conf_params *conf_params = NULL;

	conf_params = &rpu_ctx_lnx->conf_params;

	seq_puts(m, "************* Configured Parameters ***********\n");

#ifdef CONFIG_NRF700X_RADIO_TEST
	if (conf_params->op_mode == RPU_OP_MODE_RADIO_TEST)
		seq_puts(m,
			 "op_mode = Production\n");
	else if (conf_params->op_mode == RPU_OP_MODE_FCM)
		seq_puts(m,
			 "op_mode = FCM\n");
	else
		seq_puts(m,
			 "op_mode = Invalid\n");
#endif /* CONFIG_NRF700X_RADIO_TEST */

#ifndef SOC_CALDER
	seq_printf(m,
		   "nss = %d\n",
		   conf_params->nss);

	seq_printf(m,
		   "antenna_sel = %d\n",
		   conf_params->antenna_sel);
#endif /* !SOC_CALDER */

#ifdef CONFIG_NRF700X_RADIO_TEST
	if (conf_params->op_mode == RPU_OP_MODE_RADIO_TEST) {
#endif /* CONFIG_NRF700X_RADIO_TEST */
		seq_puts(m,
			 "rf_params =");

		for (i = 0; i < sizeof(conf_params->rf_params); i++)
			seq_printf(m,
				   " %02X",
				   conf_params->rf_params[i]);

		seq_puts(m, "\n");
#ifdef CONFIG_NRF700X_RADIO_TEST
	}
#endif /* CONFIG_NRF700X_RADIO_TEST */

	seq_printf(m,
		   "tx_pkt_chnl_bw = %d\n",
		   conf_params->tx_pkt_chnl_bw);

	seq_printf(m,
		   "tx_pkt_tput_mode = %d\n",
		   conf_params->tx_pkt_tput_mode);

	seq_printf(m,
		   "tx_pkt_sgi = %d\n",
		   conf_params->tx_pkt_sgi);

#ifndef SOC_CALDER
	seq_printf(m,
		   "tx_pkt_nss = %d\n",
		   conf_params->tx_pkt_nss);

	seq_printf(m,
		   "tx_pkt_stbc = %d\n",
		   conf_params->tx_pkt_stbc);

	seq_printf(m,
		   "tx_pkt_fec_coding = %d\n",
		   conf_params->tx_pkt_fec_coding);
#endif /* !SOC_CALDER */

	seq_printf(m,
		   "tx_pkt_preamble = %d\n",
		   conf_params->tx_pkt_preamble);

	seq_printf(m,
		   "tx_pkt_mcs = %d\n",
		   conf_params->tx_pkt_mcs);

	if (conf_params->tx_pkt_rate == 5)
		seq_puts(m,
			 "tx_pkt_rate = 5.5\n");
	else
		seq_printf(m,
			   "tx_pkt_rate = %d\n",
			   conf_params->tx_pkt_rate);

	seq_printf(m,
		   "phy_threshold = %d\n",
		   conf_params->phy_threshold);

	seq_printf(m,
		   "phy_calib_rxdc = %d\n",
		   (conf_params->phy_calib &
		    NRF_WIFI_PHY_CALIB_FLAG_RXDC) ? 1:0);

	seq_printf(m,
		   "phy_calib_txdc = %d\n",
		   (conf_params->phy_calib &
		    NRF_WIFI_PHY_CALIB_FLAG_TXDC) ? 1:0);

	seq_printf(m,
		   "phy_calib_txpow = %d\n",
		   (conf_params->phy_calib &
		    NRF_WIFI_PHY_CALIB_FLAG_TXPOW) ? 1:0);

	seq_printf(m,
		   "phy_calib_rxiq = %d\n",
		   (conf_params->phy_calib &
		    NRF_WIFI_PHY_CALIB_FLAG_RXIQ) ? 1:0);

	seq_printf(m,
		   "phy_calib_txiq = %d\n",
		   (conf_params->phy_calib &
		    NRF_WIFI_PHY_CALIB_FLAG_TXIQ) ? 1:0);

	seq_printf(m,
		   "phy_calib_dpd = %d\n",
		   (conf_params->phy_calib &
		    NRF_WIFI_PHY_CALIB_FLAG_DPD) ? 1:0);

#ifdef notyet
#ifdef DEBUG_MODE_SUPPORT
	seq_puts(m, "beacon_head =");
	for (i = 0; i < rpu_ctx_lnx->info.beacon_data.head_len; i++)
		seq_printf(m, " %02X",rpu_ctx_lnx->info.beacon_data.head[i]);
	seq_puts(m, "\n");

	seq_puts(m, "beacon_tail =");
	for (i = 0; i < rpu_ctx_lnx->info.beacon_data.tail_len; i++)
		seq_printf(m, " %02X",rpu_ctx_lnx->info.beacon_data.tail[i]);
	seq_puts(m, "\n");

	seq_puts(m, "probe_resp =");
	for (i = 0; i < rpu_ctx_lnx->info.beacon_data.probe_resp_len; i++)
		seq_printf(m, " %02X",rpu_ctx_lnx->info.beacon_data.probe_resp[i]);
	seq_puts(m, "\n");

	seq_printf(m, "update_template = 0\n");

	seq_printf(m,
		   "tx_rx_pol = %d\n",
		   rpu_ctx_lnx->btcoex.pta_params.tx_rx_pol);
	seq_printf(m,
		   "lead_time = %d\n",
		   rpu_ctx_lnx->btcoex.pta_params.lead_time);
	seq_printf(m,
		   "pti_samp_time = %d\n",
		   rpu_ctx_lnx->btcoex.pta_params.pti_samp_time);
	seq_printf(m,
		   "tx_rx_samp_time = %d\n",
		   rpu_ctx_lnx->btcoex.pta_params.tx_rx_samp_time);
	seq_printf(m,
		   "dec_time = %d\n",
		   rpu_ctx_lnx->btcoex.pta_params.dec_time);
	seq_printf(m,
		   "bt_ctrl = %d\n",
		   rpu_ctx_lnx->btcoex.bt_ctrl);
	seq_printf(m,
		   "bt_mode = %d\n",
		   rpu_ctx_lnx->btcoex.bt_mode);
	seq_printf(m,
		   "coex_cmd_ctrl = %d\n",
		   rpu_ctx_lnx->btcoex.coex_cmd_ctrl);
	seq_printf(m,
		   "update_btcoex_params = 0\n");

	seq_printf(m,
		   "stats_type = %d\n",
		   conf_params->stats_type);

	seq_printf(m,
		   "max_agg_limit = %d\n",
		   conf_params->max_agg_limit);

	seq_printf(m,
		   "max_agg = %d\n",
		   conf_params->max_agg);

	seq_printf(m,
		   "mimo_ps = %d\n",
		   conf_params->mimo_ps);
#ifdef BG_SCAN_SUPPORT
	seq_printf(m,
		   "bg_scan = %d\n",
		   conf_params->bg_scan);
#endif /* BG_SCAN_SUPPORT */

	seq_printf(m,
		   "rate_protection_type = %d\n",
		   conf_params->rate_protection_type);
	seq_printf(m,
		   "ch_scan_mode = %s\n",
		   conf_params->ch_scan_mode? "AUTO" : "CHANNEL_MAPPING");

	seq_printf(m,
		   "ch_probe_cnt = %d\n",
		   conf_params->ch_probe_cnt);

	seq_printf(m,
		   "passive_scan_dur = %d\n",
		   conf_params->passive_scan_dur);

	seq_printf(m,
		   "active_scan_dur = %d\n",
		   conf_params->active_scan_dur);

#endif /* DEBUG_MODE_SUPPORT */
#endif /* notyet */
#ifdef CONFIG_NRF700X_RADIO_TEST
	seq_printf(m,
		   "chnl_primary = %d\n",
		   conf_params->chan.primary_num);

	seq_printf(m,
		   "chnl_bw = %d\n",
		   conf_params->chan.bw);

#ifndef SOC_CALDER
	seq_printf(m,
		   "chnl_sec_20_offset = %d\n",
		   conf_params->chan.sec_20_offset);

	seq_printf(m,
		   "chnl_sec_40_offset = %d\n",
		   conf_params->chan.sec_40_offset);
#endif /* !SOC_CALDER */

	seq_printf(m,
		   "tx_mode = %d\n",
		   conf_params->tx_mode);

	seq_printf(m,
		   "tx_pkt_num = %d\n",
		   conf_params->tx_pkt_num);

	seq_printf(m,
		   "tx_pkt_len = %d\n",
		   conf_params->tx_pkt_len);

	seq_printf(m,
		   "tx_power = %d\n",
		   conf_params->tx_power);

	seq_printf(m,
		   "tx = %d\n",
		   conf_params->tx);

	if (conf_params->op_mode == RPU_OP_MODE_RADIO_TEST)
		seq_printf(m,
			   "rx = %d\n",
			   conf_params->rx);
#ifndef SOC_CALDER
	if (conf_params->op_mode == RPU_OP_MODE_FCM)
		seq_printf(m,
			   "aux_adc_input_chain_id = %d\n",
			   conf_params->aux_adc_input_chain_id);
#endif /* !SOC_CALDER */
#endif /* CONFIG_NRF700X_RADIO_TEST */
	seq_printf(m,
		   "he_ltf = %d\n",
		   conf_params->he_ltf);

	seq_printf(m,
		   "he_gi = %d\n",
		   conf_params->he_gi);

	seq_printf(m,
		   "he_ltf_gi = %s\n",
		   conf_params->set_he_ltf_gi ? "Enabled" : "Disabled");

	seq_printf(m,
		   "power_save = %s\n",
		   conf_params->power_save? "ON" : "OFF");

	seq_printf(m,
		   "rts_threshold = %d\n",
		   conf_params->rts_threshold);
	seq_printf(m,
		"uapsd_queue = %d\n",
		conf_params->uapsd_queue);
	return 0;
}


void nrf_wifi_lnx_wlan_fmac_conf_init(struct rpu_conf_params *conf_params)
{
	memset(conf_params, 0, sizeof(*conf_params));

	/* Initialize values which are other than 0 */
#ifdef CONFIG_NRF700X_RADIO_TEST
	conf_params->op_mode = RPU_OP_MODE_RADIO_TEST;
#endif /* CONFIG_NRF700X_RADIO_TEST */
#ifndef SOC_CALDER
	conf_params->nss = min(MAX_TX_STREAMS, MAX_RX_STREAMS);
#endif /* !SOC_CALDER */
	conf_params->antenna_sel = 1;


	memset(conf_params->rf_params, 0xFF,
	       sizeof(conf_params->rf_params));
	hex_str_to_val(conf_params->rf_params,
		       sizeof(conf_params->rf_params),
		       NRF_WIFI_DEF_RF_PARAMS);

#ifndef CONFIG_NRF700X_RADIO_TEST
	if (rf_params) {
		memset(conf_params->rf_params, 0xFF,
		       sizeof(conf_params->rf_params));
		hex_str_to_val(conf_params->rf_params,
			       sizeof(conf_params->rf_params),
			       rf_params);
	}
#endif /* !CONFIG_NRF700X_RADIO_TEST */

	conf_params->tx_pkt_nss = 1;
	conf_params->tx_pkt_mcs = -1;
	conf_params->tx_pkt_rate = -1;

	conf_params->phy_calib = phy_calib;

#ifdef notyet
#ifdef DEBUG_MODE_SUPPORT
	conf_params->stats_type = RPU_STATS_TYPE_ALL;
	conf_params->max_agg_limit = MAX_TX_AGG_SIZE;
	conf_params->mimo_ps = 1;

#ifdef BG_SCAN_SUPPORT
	conf_params->bg_scan_channel_list[i] = 1;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_channel_list[i] = 6;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_channel_list[i] = 11;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_channel_list[i] = 36;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_channel_list[i] = 40;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_channel_list[i] = 44;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_channel_list[i] = 48;
	conf_params->bg_scan_channel_flags[i++] = ACTIVE;
	conf_params->bg_scan_num_channels = 7;
	/* Background scan: Once every 5 seconds */
	conf_params->bg_scan_intvl = 5000 * 1000;
#endif /* BG_SCAN_SUPPORT */

	conf_params->rate_protection_type = 1;
	conf_params->phy_threshold = PHY_THRESHOLD_NORMAL;
#define	HOST_CHANNEL_MAPPING_SCAN_MODE 0
	conf_params->ch_scan_mode =  HOST_CHANNEL_MAPPING_SCAN_MODE;
	conf_params->ch_probe_cnt = CHNL_PROBE_CNT;
	conf_params->active_scan_dur = ACTIVE_SCAN_DURATION;
	conf_params->passive_scan_dur = PASSIVE_SCAN_DURATION;
#endif /* DEBUG_MODE_SUPPORT */
#endif /* notyet */

#ifdef CONFIG_NRF700X_RADIO_TEST
	conf_params->chan.primary_num = 1;
	conf_params->tx_mode = 1;
	conf_params->tx_pkt_num = -1;
	conf_params->tx_pkt_len = 1400;
	conf_params->phy_threshold = PHY_THRESHOLD_PROD_MODE;
	conf_params->aux_adc_input_chain_id = 1;
#endif /* CONFIG_NRF700X_RADIO_TEST */
	conf_params->he_ltf = -1;
	conf_params->he_gi = -1;
	conf_params->set_he_ltf_gi = 0;
	conf_params->power_save = 0;
	conf_params->rts_threshold = 0;
}


static ssize_t nrf_wifi_lnx_wlan_fmac_conf_write(struct file *file,
					      const char __user *in_buf,
					      size_t count,
					      loff_t *ppos)
{
	char *conf_buf = NULL;
	unsigned long val = 0;
	long sval = 0;
#ifdef DEBUG_MODE_SUPPORT
	int len = 0;
#endif
	char err_str[MAX_ERR_STR_SIZE];
	ssize_t ret_val = count;
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = NULL;
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
#ifdef SOC_WEZEN
	long data_rate = -1;
	long rate_flag = -1;
#endif /* SOC_WEZEN */

	rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)file->f_inode->i_private;

	if (count >= MAX_CONF_BUF_SIZE) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Size of input buffer cannot be more than %d chars\n",
			 MAX_CONF_BUF_SIZE);

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf = kzalloc(MAX_CONF_BUF_SIZE, GFP_KERNEL);

	if (!conf_buf) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Not enough memory available\n");

		ret_val = -EFAULT;
		goto error;
	}

	if (copy_from_user(conf_buf,
			   in_buf,
			   count)) {
		snprintf(err_str, MAX_ERR_STR_SIZE,
			 "Copy from input buffer failed\n");

		ret_val = -EFAULT;
		goto error;
	}

	conf_buf[count-1] = '\0';

#ifdef CONFIG_NRF700X_RADIO_TEST
	if ((rpu_ctx_lnx->conf_params.op_mode != RPU_OP_MODE_RADIO_TEST) &&
	    (rpu_ctx_lnx->conf_params.op_mode != RPU_OP_MODE_FCM)) {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Invalid OP mode %d\n",
			 rpu_ctx_lnx->conf_params.op_mode);
		ret_val = -EFAULT;
		goto error;
	}
#endif /* CONFIG_NRF700X_RADIO_TEST */

	if (param_get_sval(conf_buf, "phy_threshold=", &sval)) {
		if ((sval < -113) || (sval > 20)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 sval);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		rpu_ctx_lnx->conf_params.phy_threshold = sval;
	} else if (param_get_val(conf_buf, "phy_calib_rxdc=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val)
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib |
							      NRF_WIFI_PHY_CALIB_FLAG_RXDC);
		else
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib &
							      ~(NRF_WIFI_PHY_CALIB_FLAG_RXDC));
	} else if (param_get_val(conf_buf, "phy_calib_txdc=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val)
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib |
							      NRF_WIFI_PHY_CALIB_FLAG_TXDC);
		else
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib &
							      ~(NRF_WIFI_PHY_CALIB_FLAG_TXDC));
	} else if (param_get_val(conf_buf, "phy_calib_txpow=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val)
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib |
							      NRF_WIFI_PHY_CALIB_FLAG_TXPOW);
		else
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib &
							      ~(NRF_WIFI_PHY_CALIB_FLAG_TXPOW));
	} else if (param_get_val(conf_buf, "phy_calib_rxiq=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val)
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib |
							      NRF_WIFI_PHY_CALIB_FLAG_RXIQ);
		else
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib &
							      ~(NRF_WIFI_PHY_CALIB_FLAG_RXIQ));
	} else if (param_get_val(conf_buf, "phy_calib_txiq=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val)
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib |
							      NRF_WIFI_PHY_CALIB_FLAG_TXIQ);
		else
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib &
							      ~(NRF_WIFI_PHY_CALIB_FLAG_TXIQ));
	} else if (param_get_val(conf_buf, "phy_calib_dpd=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %ld\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val)
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib |
							      NRF_WIFI_PHY_CALIB_FLAG_DPD);
		else
			rpu_ctx_lnx->conf_params.phy_calib = (rpu_ctx_lnx->conf_params.phy_calib &
							      ~(NRF_WIFI_PHY_CALIB_FLAG_DPD));
#ifndef SOC_CALDER
	} else if (param_get_val(conf_buf, "nss=", &val)) {
		if ((val != 1) && (val != 2)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.nss == val)
			goto out;

		rpu_ctx_lnx->conf_params.nss = val;
	} else if (param_get_val(conf_buf, "antenna_sel=", &val)) {
		if ((val != 1) && (val != 2)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.antenna_sel == val)
			goto out;

		rpu_ctx_lnx->conf_params.antenna_sel = val;
#endif /* !SOC_CALDER */
	} else if (param_get_val(conf_buf, "he_ltf=", &val)) {
		if ((val < 0) || (val > 2)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.he_ltf == val)
			goto out;
		rpu_ctx_lnx->conf_params.he_ltf = val;
	} else if (param_get_val(conf_buf, "he_gi=", &val)) {
		if ((val < 0) || (val > 2)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.he_gi == val)
			goto out;
		rpu_ctx_lnx->conf_params.he_gi = val;
#ifndef CONFIG_NRF700X_RADIO_TEST
	} else if (param_get_val(conf_buf, "set_he_ltf_gi=", &val)) {
		if ((val != 0) && (val != 1)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.set_he_ltf_gi == val)
			goto out;


		status = nrf_wifi_fmac_conf_ltf_gi(rpu_ctx_lnx->rpu_ctx,
						     rpu_ctx_lnx->conf_params.he_ltf,
						     rpu_ctx_lnx->conf_params.he_gi,
						     val);
		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming LTF GI failed\n");
			goto error;
		}
		rpu_ctx_lnx->conf_params.set_he_ltf_gi = val;
	} else if (param_get_val(conf_buf, "power_save=", &val)) {
		if ((val != 0) && (val != 1)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.power_save == val)
			goto out;

		status = nrf_wifi_fmac_set_power_save(rpu_ctx_lnx->rpu_ctx,
							0,
							val);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming power save failed\n");
			goto error;
		}
		rpu_ctx_lnx->conf_params.power_save = val; 
	} else if (param_get_val(conf_buf, "rts_threshold=", &val)) {
		struct nrf_wifi_umac_set_wiphy_info *wiphy_info = NULL;

		if (val <= 0) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.rts_threshold == val)
			goto out;

		wiphy_info = kzalloc(sizeof(*wiphy_info), GFP_KERNEL);

		if (!wiphy_info) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Unable to allocate memory\n");
			goto error;
		}	
		wiphy_info->rts_threshold = val;	

		status = nrf_wifi_fmac_set_wiphy_params(rpu_ctx_lnx->rpu_ctx,
							  0,
							  wiphy_info);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming rts_threshold failed\n");
			goto error;
		}
		if (wiphy_info)
			kfree(wiphy_info);

		rpu_ctx_lnx->conf_params.rts_threshold = val; 
	} else if (param_get_val(conf_buf, "uapsd_queue=", &val)) {
		if ((val < 0) || (val > 15)) {
                        snprintf(err_str,
                                 MAX_ERR_STR_SIZE,
                                 "Invalid value %lu\n",
                                 val);
                        ret_val = -EINVAL;
                        goto error;
                }

                if (rpu_ctx_lnx->conf_params.uapsd_queue == val)
                        goto out;

                status = nrf_wifi_fmac_set_uapsd_queue(rpu_ctx_lnx->rpu_ctx,
                                                        0,
                                                        val);

                if (status != NRF_WIFI_STATUS_SUCCESS) {
                        snprintf(err_str,
                                 MAX_ERR_STR_SIZE,
                                 "Programming uapsd queue failed\n");
                        goto error;
                }
                rpu_ctx_lnx->conf_params.uapsd_queue = val;
#endif
	} else if (param_get_match(conf_buf, "rf_params=")) {
#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode != RPU_OP_MODE_RADIO_TEST) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "rf_params setting only allowed in radio test mode\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx ||
		    rpu_ctx_lnx->conf_params.rx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX/RX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		memset(rpu_ctx_lnx->conf_params.rf_params, 0xFF,
		       sizeof(rpu_ctx_lnx->conf_params.rf_params));
		hex_str_to_val(rpu_ctx_lnx->conf_params.rf_params,
			       sizeof(rpu_ctx_lnx->conf_params.rf_params),
			       strstr(conf_buf, "=") + 1);
#ifdef DEBUG_MODE_SUPPORT
	} else if (param_get_match(conf_buf, "beacon_head=")) {
		memset(rpu_ctx_lnx->info.beacon_data.head, 0,
		       sizeof(rpu_ctx_lnx->info.beacon_data.head));
		len = hex_str_to_val(rpu_ctx_lnx->info.beacon_data.head,
				     sizeof(rpu_ctx_lnx->info.beacon_data.head),
				     strstr(conf_buf, "=") + 1);
		if (len > 0)
			rpu_ctx_lnx->info.beacon_data.head_len = len;
		else 
			rpu_ctx_lnx->info.beacon_data.head_len = 0;

		pr_err("%s, DUMPING BEACON DATA HEAD, head length = %d\n",
		       __func__,
		       rpu_ctx_lnx->info.beacon_data.head_len);

		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16,
			       1, rpu_ctx_lnx->info.beacon_data.head,
			       rpu_ctx_lnx->info.beacon_data.head_len, 1);

	} else if (param_get_match(conf_buf, "beacon_tail=")) {
		memset(rpu_ctx_lnx->info.beacon_data.tail, 0,
		       sizeof(rpu_ctx_lnx->info.beacon_data.tail));
		len = hex_str_to_val(rpu_ctx_lnx->info.beacon_data.tail,
				     sizeof(rpu_ctx_lnx->info.beacon_data.tail),
				     strstr(conf_buf, "=") + 1);
		if (len > 0)
			rpu_ctx_lnx->info.beacon_data.tail_len = len;
		else
			rpu_ctx_lnx->info.beacon_data.tail_len = 0;

		pr_err("%s, DUMPING BEACON DATA TAIL, tail length = %d\n",
		       __func__,
		       rpu_ctx_lnx->info.beacon_data.tail_len);

		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16,
			       1, rpu_ctx_lnx->info.beacon_data.tail,
			       rpu_ctx_lnx->info.beacon_data.tail_len, 1);

	} else if (param_get_match(conf_buf, "probe_resp=")) {
		memset(rpu_ctx_lnx->info.beacon_data.probe_resp, 0,
		       sizeof(rpu_ctx_lnx->info.beacon_data.probe_resp));
		len = hex_str_to_val(rpu_ctx_lnx->info.beacon_data.probe_resp,
				     sizeof(rpu_ctx_lnx->info.beacon_data.probe_resp),
				     strstr(conf_buf, "=") + 1);
		if (len>0)
			rpu_ctx_lnx->info.beacon_data.probe_resp_len = len;
		else
			rpu_ctx_lnx->info.beacon_data.probe_resp_len = 0;

		pr_err("%s, DUMPING PROBE_RESP, probe_resp_len = %d\n",
		       __func__,
		       rpu_ctx_lnx->info.beacon_data.probe_resp_len);

		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16,
			       1, rpu_ctx_lnx->info.beacon_data.probe_resp,
			       rpu_ctx_lnx->info.beacon_data.probe_resp_len, 1);

	} else if (param_get_val(conf_buf, "update_template=", &val)) {
		if (val != 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		pr_err("%s, DUMPING BEACON DATA. \
		       head_len = %d, \
		       tail_len = %d, \
		       probe_resp_len = %d\n",
		       __func__,
		       rpu_ctx_lnx->info.beacon_data.head_len,
		       rpu_ctx_lnx->info.beacon_data.tail_len,
		       rpu_ctx_lnx->info.beacon_data.probe_resp_len);

		pr_err("%s, DUMPING BEACON DATA TAIL\n", __func__);
		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16,
			       1, rpu_ctx_lnx->info.beacon_data.head,
			       rpu_ctx_lnx->info.beacon_data.head_len, 1);

		pr_err("%s, DUMPING BEACON DATA TAIL\n", __func__);
		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16,
			       1, rpu_ctx_lnx->info.beacon_data.tail,
			       rpu_ctx_lnx->info.beacon_data.tail_len, 1);

		pr_err("%s, DUMPING PROBE_RESP \n", __func__);
		print_hex_dump(KERN_DEBUG, "", DUMP_PREFIX_NONE, 16,
			       1, rpu_ctx_lnx->info.beacon_data.probe_resp,
			       rpu_ctx_lnx->info.beacon_data.probe_resp_len, 1);

		status =  nrf_wifi_fmac_chg_bcn(rpu_ctx_lnx->rpu_ctx,
						  1,
						  &rpu_ctx_lnx->info);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "nrf_wifi_fmac_chg_bcn failed\n");
			goto error;
		}

	} else if (param_get_val(conf_buf, "tx_rx_pol=", &val)) {
		rpu_ctx_lnx->btcoex.pta_params.tx_rx_pol = val;
	} else if (param_get_val(conf_buf, "lead_time=", &val)) {
		rpu_ctx_lnx->btcoex.pta_params.lead_time = val;
	} else if (param_get_val(conf_buf, "pti_samp_time=", &val)) {
		rpu_ctx_lnx->btcoex.pta_params.pti_samp_time = val;
	} else if (param_get_val(conf_buf, "tx_rx_samp_time=", &val)) {
		rpu_ctx_lnx->btcoex.pta_params.tx_rx_samp_time = val;
	} else if (param_get_val(conf_buf, "dec_time=", &val)) {
		rpu_ctx_lnx->btcoex.pta_params.dec_time = val;
	} else if (param_get_val(conf_buf, "bt_ctrl=", &val)) {
		rpu_ctx_lnx->btcoex.bt_ctrl = val;
	} else if (param_get_val(conf_buf, "bt_mode=", &val)) {
		rpu_ctx_lnx->btcoex.bt_mode = val;
	} else if (param_get_val(conf_buf, "coex_cmd_ctrl=", &val)) {
		rpu_ctx_lnx->btcoex.coex_cmd_ctrl = val;
	} else if (param_get_val(conf_buf, "update_btcoex_params=", &val)) {
		if (val != 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}
		status = nrf_wifi_fmac_conf_btcoex(rpu_ctx_lnx->rpu_ctx,
						     &rpu_ctx_lnx->btcoex);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "nrf_wifi_fmac_conf_btcoex failed\n");
			goto error;
		}
#endif
	} else if (param_get_val(conf_buf, "tx_pkt_chnl_bw=", &val)) {
		if (val >= RPU_CH_BW_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_chnl_bw == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_chnl_bw = val;
	} else if (param_get_val(conf_buf, "tx_pkt_tput_mode=", &val)) {
		if (val >= RPU_TPUT_MODE_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_tput_mode == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_tput_mode = val;
	} else if (param_get_val(conf_buf, "tx_pkt_sgi=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_sgi == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_sgi = val;
#ifndef SOC_CALDER
	} else if (param_get_val(conf_buf, "tx_pkt_nss=", &val)) {
		if ((val != 1) && (val != 2)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (val > rpu_ctx_lnx->conf_params.nss) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid: (tx_pkt_nss > nss)\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_pkt_nss == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_nss = val;
#endif /* !SOC_CALDER */
	} else if (param_get_val(conf_buf, "tx_pkt_preamble=", &val)) {
#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_tput_mode == 0) {
			if ((val != RPU_PKT_PREAMBLE_SHORT) &&
			    (val != RPU_PKT_PREAMBLE_LONG)) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Invalid value %lu\n",
					 val);
				ret_val = -EINVAL;
				goto error;
			}
		} else {
#ifdef notyet
			if ((val != RPU_PKT_PREAMBLE_MIXED) &&
			    (val != RPU_PKT_PREAMBLE_GF)) {
#else
				if (val != RPU_PKT_PREAMBLE_MIXED) {
#endif /* notyet */
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Invalid value %lu\n",
						 val);
					ret_val = -EINVAL;
					goto error;
				}
			}

			if (rpu_ctx_lnx->conf_params.tx_pkt_preamble == val)
				goto out;

			rpu_ctx_lnx->conf_params.tx_pkt_preamble = val;
#ifndef SOC_CALDER
	} else if (param_get_val(conf_buf, "tx_pkt_stbc=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_stbc == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_stbc = val;
	} else if (param_get_val(conf_buf, "tx_pkt_fec_coding=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_fec_coding == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_fec_coding = val;
#endif /* !SOC_CALDER */
	} else if (param_get_sval(conf_buf, "tx_pkt_mcs=", &sval)) {
#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_rate != -1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "tx_pkt_rate is set\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (!(check_valid_data_rate(rpu_ctx_lnx->conf_params.tx_pkt_tput_mode,
					    rpu_ctx_lnx->conf_params.tx_pkt_nss,
					    sval | 0x80))) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 sval);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_pkt_mcs == sval)
			goto out;

#ifdef SOC_WEZEN
#ifndef CONFIG_NRF700X_RADIO_TEST
		rate_flag = rpu_ctx_lnx->conf_params.tx_pkt_tput_mode;
		data_rate = sval;

		if (rpu_ctx_lnx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_HE_TB)
			data_rate = -1;

		status = nrf_wifi_fmac_set_tx_rate(rpu_ctx_lnx->rpu_ctx,
						   rate_flag,
						   data_rate);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming TX Rate failed\n");
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /* SOC_WEZEN */
		rpu_ctx_lnx->conf_params.tx_pkt_mcs = sval;
	} else if (param_get_sval(conf_buf, "tx_pkt_rate=", &sval)) {
#ifdef CONFIG_NRF700X_RADIO_TEST
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */

		if (rpu_ctx_lnx->conf_params.tx_pkt_mcs != -1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "tx_pkt_mcs is set\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (!(check_valid_data_rate(rpu_ctx_lnx->conf_params.tx_pkt_tput_mode,
					    rpu_ctx_lnx->conf_params.tx_pkt_nss,
					    sval))) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 sval);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_pkt_rate == sval)
			goto out;
#ifdef SOC_WEZEN
#ifndef CONFIG_NRF700X_RADIO_TEST
		rate_flag = rpu_ctx_lnx->conf_params.tx_pkt_tput_mode;
		data_rate = sval;

		if (rpu_ctx_lnx->conf_params.tx_pkt_tput_mode == RPU_TPUT_MODE_HE_TB)
			data_rate = -1;

		status = nrf_wifi_fmac_set_tx_rate(rpu_ctx_lnx->rpu_ctx,
						   rate_flag,
						   data_rate);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming TX Rate failed\n");
			goto error;
		}
#endif /* CONFIG_NRF700X_RADIO_TEST */
#endif /*SOC_WEZEN */
		rpu_ctx_lnx->conf_params.tx_pkt_rate = sval;
#ifdef notyet
#ifdef DEBUG_MODE_SUPPORT
	} else if (param_get_val(conf_buf, "stats_type=", &val)) {
		if (val >= RPU_STATS_TYPE_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", val);
			ret_val = -EINVAL;
			goto error;
		}

		rpu_ctx_lnx->conf_params.stats_type = val;
	} else if (param_get_val(conf_buf, "max_agg_limit=", &val)) {
		if (val < 1 ||
		    val > MAX_TX_AGG_SIZE) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.max_agg_limit == val)
			goto out;

		rpu_ctx_lnx->conf_params.max_agg_limit = val;
	} else if (param_get_val(conf_buf, "max_agg=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.max_agg == val)
			goto out;

		rpu_ctx_lnx->conf_params.max_agg = val;
	} else if (param_get_val(conf_buf, "mimo_ps=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.mimo_ps == val)
			goto out;

		rpu_ctx_lnx->conf_params.mimo_ps = val;
#ifdef BG_SCAN_SUPPORT
	} else if (param_get_val(conf_buf, "bg_scan=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.bg_scan == val)
			goto out;

		rpu_ctx_lnx->conf_params.bg_scan = val;
#endif /* BG_SCAN_SUPPORT */
	} else if (param_get_val(conf_buf, "rate_protection_type=", &val)) {
		if (val > 2) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.rate_protection_type == val)
			goto out;

		rpu_ctx_lnx->conf_params.rate_protection_type = val;
	} else if (param_get_val(conf_buf, "ch_scan_mode=", &val)) {
		if (val == 0 || val == 1)
			rpu_ctx_lnx->conf_params.ch_scan_mode = val;
		else {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid parameter value:valid(0/1)\n");
			ret_val = -EFAULT;
			goto error;
		}
	} else if (param_get_val(conf_buf, "ch_probe_cnt=", &val)) {
		if (val >= 1 || val <= 4)
			rpu_ctx_lnx->conf_params.ch_probe_cnt = val;
		else {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid parameter value:Range(1 to 4)\n");
			ret_val = -EFAULT;
			goto error;
		}
	} else if (param_get_val(conf_buf, "active_scan_dur=", &val)) {
		if (val >= ACTIVE_SCAN_DURATION && val < 65535)
			rpu_ctx_lnx->conf_params.active_scan_dur = val;
		else {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid parameter value:valid(80 to 65534)\n");
			ret_val = -EFAULT;
			goto error;
		}
	} else if (param_get_val(conf_buf, "passive_scan_dur=", &val)) {
		if (val >= PASSIVE_SCAN_DURATION && val < 65535)
			rpu_ctx_lnx->conf_params.passive_scan_dur = val;
		else {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid parameter value:Range(130 to 65534)\n");
			ret_val = -EFAULT;
			goto error;
		}		
#endif /* DEBUG_MODE_SUPPORT */
#endif /* notyet */
#ifdef CONFIG_NRF700X_RADIO_TEST
	} else if (param_get_val(conf_buf, "op_mode=", &val)) {
		if ((val != RPU_OP_MODE_RADIO_TEST) &&
		    (val != RPU_OP_MODE_FCM)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == val)
			goto out;

		rpu_ctx_lnx->conf_params.op_mode = val;
	} else if (param_get_val(conf_buf, "chnl_primary=", &val)) {
		if (!(check_valid_channel(val))) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 sval);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.chan.primary_num == val)
			goto out;

		rpu_ctx_lnx->conf_params.chan.primary_num = val;
	} else if (param_get_val(conf_buf, "chnl_bw=", &val)) {
		if (val >= RPU_CH_BW_MAX) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.chan.bw == val)
			goto out;

		rpu_ctx_lnx->conf_params.chan.bw = val;
#ifndef SOC_CALDER
	} else if (param_get_sval(conf_buf, "chnl_sec_20_offset=", &sval)) {
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.chan.bw == 0) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Channel bandwidth is 20 MHz\n");
			ret_val = -EFAULT;
			goto error;
		}

		if ((rpu_ctx_lnx->conf_params.chan.bw == 1) ||
		    (rpu_ctx_lnx->conf_params.chan.bw == 2)) {
			if ((sval != -1) && (sval != 1) && (sval != 0)) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Invalid value %lu\n",
					 sval);
				ret_val = -EINVAL;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.chan.sec_20_offset == sval)
			goto out;

		rpu_ctx_lnx->conf_params.chan.sec_20_offset = sval;
	} else if (param_get_sval(conf_buf, "chnl_sec_40_offset=", &sval)) {
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if ((rpu_ctx_lnx->conf_params.chan.bw == 0) ||
		    (rpu_ctx_lnx->conf_params.chan.bw == 1)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Channel bandwidth is 20/40 MHz\n");
			ret_val = -EFAULT;
			goto error;
		}

		if ((rpu_ctx_lnx->conf_params.chan.bw == 2)) {
			if ((sval != -1) && (sval != 1) && (sval != 0)) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Invalid value %lu\n",
					 sval);
				ret_val = -EINVAL;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.chan.sec_40_offset == sval)
			goto out;

		rpu_ctx_lnx->conf_params.chan.sec_40_offset = sval;
#endif /* !SOC_CALDER */
	} else if (param_get_val(conf_buf, "tx_mode=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n", sval);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_mode == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_mode = val;
	} else if (param_get_sval(conf_buf, "tx_pkt_num=", &sval)) {
		if ((sval < -1) || (sval == 0)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 sval);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_pkt_num == sval)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_num = sval;
	} else if (param_get_val(conf_buf, "tx_pkt_len=", &val)) {
		if (val == 0) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_pkt_len == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_pkt_len = val;
	} else if (param_get_val(conf_buf, "tx_power=", &val)) {
		if (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) {
			if (rpu_ctx_lnx->conf_params.rx) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "Disable RX\n");
				ret_val = -EFAULT;
				goto error;
			}
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx_power == val)
			goto out;

		rpu_ctx_lnx->conf_params.tx_power = val;
	} else if (param_get_val(conf_buf, "tx=", &val)) {
		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if ((val == 1) &&
		    (rpu_ctx_lnx->conf_params.op_mode == RPU_OP_MODE_RADIO_TEST) &&
		    (rpu_ctx_lnx->conf_params.rx)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable RX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx == val)
			goto out;


		if (val == 1) {
#ifndef SOC_CALDER
			if (rpu_ctx_lnx->conf_params.chan.sec_20_offset != 0) {
				if ((rpu_ctx_lnx->conf_params.chan.bw <= 0)) {
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Invalid channel parameter settings\n");
					ret_val = -EINVAL;
					goto error;
				}
			}

			if (rpu_ctx_lnx->conf_params.chan.sec_40_offset != 0) {
				if (rpu_ctx_lnx->conf_params.chan.bw != 2) {
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Invalid channel parameter settings\n");
					ret_val = -EINVAL;
					goto error;
				}
			}
#endif /* !SOC_CALDER */

			if (rpu_ctx_lnx->conf_params.chan.bw == 0) {
#ifndef SOC_CALDER
				if (rpu_ctx_lnx->conf_params.chan.sec_20_offset != 0) {
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Invalid channel parameter settings\n");
					ret_val = -EINVAL;
					goto error;
				}

				if (rpu_ctx_lnx->conf_params.chan.sec_40_offset != 0) {
					snprintf(err_str,
						 MAX_ERR_STR_SIZE,
						 "Invalid channel parameter settings\n");
					ret_val = -EINVAL;
					goto error;
				}
#endif /* !SOC_CALDER */
			}

			if (check_channel_settings(rpu_ctx_lnx->conf_params.tx_pkt_tput_mode,
						   &rpu_ctx_lnx->conf_params.chan) != 0) {
				ret_val = -EINVAL;
				goto error;
			}

			if ((rpu_ctx_lnx->conf_params.tx_pkt_rate != -1) &&
			    (rpu_ctx_lnx->conf_params.tx_pkt_mcs != -1)) {
				snprintf(err_str,
					 MAX_ERR_STR_SIZE,
					 "'tx_pkt_rate' & 'tx_pkt_mcs' cannot be set simultaneously\n");
				ret_val = -EINVAL;
				goto error;
			}
		}


		rpu_ctx_lnx->conf_params.tx = val;

		status = nrf_wifi_fmac_radio_test_prog_tx(rpu_ctx_lnx->rpu_ctx,
							  &rpu_ctx_lnx->conf_params);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming TX failed\n");
			goto error;
		}
	} else if (param_get_val(conf_buf, "rx=", &val)) {
		if (rpu_ctx_lnx->conf_params.op_mode != RPU_OP_MODE_RADIO_TEST) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "rx setting only allowed in radio test mode\n");
			ret_val = -EINVAL;
			goto error;
		}

		if (val > 1) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if ((val == 1) &&
		    rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.rx == val)
			goto out;

		rpu_ctx_lnx->conf_params.rx = val;

		status = nrf_wifi_fmac_radio_test_prog_rx(rpu_ctx_lnx->rpu_ctx,
							  &rpu_ctx_lnx->conf_params);

		if (status != NRF_WIFI_STATUS_SUCCESS) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Programming RX failed\n");
			goto error;
		}
#ifndef SOC_CALDER
	} else if (param_get_val(conf_buf, "aux_adc_input_chain_id=", &val)) {
		if (rpu_ctx_lnx->conf_params.op_mode != RPU_OP_MODE_FCM) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "aux_adc_input_chain_id setting only allowed in FCM mode\n");
			ret_val = -EINVAL;
			goto error;
		}

		if ((val == 0) || (val > MAX_TX_STREAMS)) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Invalid value %lu\n",
				 val);
			ret_val = -EINVAL;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.tx) {
			snprintf(err_str,
				 MAX_ERR_STR_SIZE,
				 "Disable TX\n");
			ret_val = -EFAULT;
			goto error;
		}

		if (rpu_ctx_lnx->conf_params.aux_adc_input_chain_id == val)
			goto out;

		rpu_ctx_lnx->conf_params.aux_adc_input_chain_id = val;
#endif /* !SOC_CALDER */
#endif /* CONFIG_NRF700X_RADIO_TEST */
	} else {
		snprintf(err_str,
			 MAX_ERR_STR_SIZE,
			 "Invalid parameter name: %s\n",
			 conf_buf);
		ret_val = -EFAULT;
		goto error;
	}

	goto out;

error:
	pr_err("Error condition: %s\n", err_str);
out:
	kfree(conf_buf);

	return ret_val;
}


static int nrf_wifi_lnx_wlan_fmac_conf_open(struct inode *inode, struct file *file)
{
	struct nrf_wifi_ctx_lnx *rpu_ctx_lnx = (struct nrf_wifi_ctx_lnx *)inode->i_private;

	return single_open(file,
			   nrf_wifi_lnx_wlan_fmac_conf_disp,
			   rpu_ctx_lnx);
}


static const struct file_operations fops_wlan_conf = {
	.open = nrf_wifi_lnx_wlan_fmac_conf_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = nrf_wifi_lnx_wlan_fmac_conf_write,
	.release = single_release
};


int nrf_wifi_lnx_wlan_fmac_dbgfs_conf_init(struct dentry *root,
					struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	int ret = 0;

	if ((!root) || (!rpu_ctx_lnx)) {
		pr_err("%s: Invalid parameters\n", __func__);
		ret = -EINVAL;
		goto fail;
	}

	rpu_ctx_lnx->dbgfs_wlan_conf_root = debugfs_create_file("conf",
								0644,
								root,
								rpu_ctx_lnx,
								&fops_wlan_conf);

	if (!rpu_ctx_lnx->dbgfs_wlan_conf_root) {
		pr_err("%s: Failed to create debugfs entry\n", __func__);
		ret = -ENOMEM;
		goto fail;
	}

	goto out;

fail:
	nrf_wifi_lnx_wlan_fmac_dbgfs_conf_deinit(rpu_ctx_lnx);
out:
	return ret;
}


void nrf_wifi_lnx_wlan_fmac_dbgfs_conf_deinit(struct nrf_wifi_ctx_lnx *rpu_ctx_lnx)
{
	if (rpu_ctx_lnx->dbgfs_wlan_conf_root)
		debugfs_remove(rpu_ctx_lnx->dbgfs_wlan_conf_root);

	rpu_ctx_lnx->dbgfs_wlan_conf_root = NULL;
}
#endif /* CONF_SUPPORT */
