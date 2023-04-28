/*
 * Copyright (C) 2015 Marvell Technology Group Ltd.
 *
 * Jisheng Zhang <jszhang@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

struct phy_berlin4_usb3_priv {
	void __iomem		*base;
};

#define PHY_STATUS_POLLING_INTERVAL  (100)  // us
#define PHY_STATUS_POLLING_RETRY     (1000)

#define CID_REG0     (0<<2)

#define POWER_REG0   (1<<2)
    #define PU_PLL_MASK  (1 << 14)
    #define PU_PLL_OFFSET (14)
    #define PU_RX_MASK  (1 << 13)
    #define PU_RX_OFFSET (13)
    #define PU_TX_MASK  (1 << 12)
    #define PU_TX_OFFSET (12)
    #define PHY_MODE_MASK (0x7 << 5)
    #define PHY_MODE_OFFSET (5)
    #define REF_FREF_SEL_MASK (0x1F)
    #define REF_FREF_SEL_OFFSET (0)

#define CAL_REG0  (0x2<<2)
    #define USE_MAX_PLL_RATE_MASK  (1 << 12)
    #define USE_MAX_PLL_RATE_OFFSET  (12)

#define DFE_REG1 (0x8<<2)
    #define DFE_UPDATE_EN_MASK (0x3F << 6)
    #define DFE_UPDATE_EN_OFFSET (6)

#define G1_SETTING_0 (0xD<<2)
    #define G1_TX_SLEW_CTRL_EN_MASK (1 << 15)
    #define G1_TX_SLEW_CTRL_EN_OFFSET (15)

#define G2_SETTING_0 (0xF<<2)
    #define G2_TX_SLEW_CTRL_EN_MASK (1 << 15)
    #define G2_TX_SLEW_CTRL_EN_OFFSET (15)

#define G2_SETTING_RX (0x10<<2)

#define G3_SETTING_0 (0x11<<2)
    #define G3_TX_SLEW_CTRL_EN_MASK (1 << 15)
    #define G3_TX_SLEW_CTRL_EN_OFFSET (15)

#define G4_SETTING_0 (0x13<<2)
    #define G4_TX_SLEW_CTRL_EN_MASK (1 << 15)
    #define G4_TX_SLEW_CTRL_EN_OFFSET (15)


#define LOOPBACK_REG0  (0x23<<2)
    #define SEL_BITS_MASK  (0x3 << 10)
    #define SEL_BITS_OFFSET (10)
    #define PLL_READY_RX_MASK (1 << 5)
    #define PLL_READY_RX_OFFSET (5)
    #define PLL_READY_TX_MASK (1 << 4)
    #define PLL_READY_TX_OFFSET (4)


#define INTERFACE_REG1 (0x25<<2)
    #define PHY_GEN_MAX_MASK (0x3 << 10)
    #define PHY_GEN_MAX_OFFSET (10)

#define ISOLATE_REG0 (0x26<<2)
    #define PHY_GEN_TX_MASK   (0xF << 4)
    #define PHY_GEN_TX_OFFSET (4)
    #define PHY_GEN_RX_MASK   (0xF)
    #define PHY_GEN_RX_OFFSET (0)

#define G2_TX_SSC_AMP (0x3E<<2)

#define IMPEDANCE_REG (0X41<<2)

#define PCIE_REG0 (0x48<<2)
    #define IDLE_SYNC_EN_MASK (1 << 12)
    #define IDLE_SYNC_EN_OFFSET  (12)


#define MISC_REG0    (0x4F<<2)
    #define REFCLK_SEL_MASK (0x1 << 10)
    #define REFCLK_SEL_OFFSET (10)
    #define CLK500M_EN_MASK (1 << 7)
    #define CLK500M_EN_OFFSET (7)
    #define TXDCLK_2X_SEL_MASK (1 << 6)
    #define TXDCLK_2X_SEL_OFFSET (6)

#define INTERFACE_REG2 (0x51<<2)

#define CFG_TX_AMP   (0x5E<<2)

#define CFG_SSC_CTRL (0x5F<<2)

#define LANE_STATUS1 (0x183<<2)
    #define PM_TXDCLK_PCLK_EN_MASK (1)
    #define PM_TXDCLK_PCLK_EN_OFFSET (0)

#define LANE_STATUS2 (0x184<<2)
    #define MAC_PHY_RATE_MASK (1 << 11)
    #define MAC_PHY_RATE_OFFSET (11)

#define GLOB_CLK_CTRL  (0x1C1<<2)
    #define MODE_CORE_CLK_FREQ_SEL_MASK (1 << 9)
    #define MODE_CORE_CLK_FREQ_SEL_OFFSET  (9)
    #define MODE_REFDIV_MASK (3 << 4)
    #define MODE_REFDIV_OFFSET  (4)
    #define MODE_PIPE_WIDTH_32_MASK (1 << 3)
    #define MODE_PIPE_WIDTH_32_OFFSET (3)
    #define MODE_FIXED_PCLK_MASK (1 << 2)
    #define MODE_FIXED_PCLK_OFFSET (2)
    #define PIPE_SFT_RESET_MASK (1)
    #define PIPE_SFT_RESET_OFFSET (0)

#define GLOB_CLK_SRC_LO  (0x1C3<<2)
    #define PLL_READY_DLY_MASK (0x7 << 5)
    #define PLL_READY_DLY_OFFSET (5)

#define GLOB_PM_CFG0  (0x1D0<<2)
    #define CFG_PM_RXDLOZ_WAIT_MASK (0xFF)
    #define CFG_PM_RXDLOZ_WAIT_OFFSET (0)

#define LANE_CONFIG4 (0x188<<2)
#define CFG_RX_REG   (0X189<<2)
#define GLOB_DB_CFG  (0x1C8<<2)

static int phy_berlin4_usb3_power_on(struct phy *phy)
{
	int retry;
	u32 data;
	struct phy_berlin4_usb3_priv *priv = phy_get_drvdata(phy);

	/* GLOB_CLK_CTRL : 0x1C1
	b5:b4="10"--> Divide 4
	b3=1 --> 32 bit pipe
	b0=1--> Soft Reset */
	writel(0x29, priv->base + GLOB_CLK_CTRL);

	/* CFG_RX_REG 0x189
	Set cfg_sq_det_sel (R189h [1]) = 1
	Set cfg_rx_eq_ctrl (R189h [2]) = 1
	b3=1 -> CFG_RXEIDET_DG_EN :1'b1: enable the RxElecIdle de-glitch logic
	b0=0 -> CFG_RX_INIT_SEL :1: reset rx_init control state machine when detect LFPS signaling
	b4=0 -> CFG_RXEI_DG_WEIGHT :1'b1: 1-tap de-glitching */
	writel(0x6, priv->base + CFG_RX_REG);

	/* g2_setting TX 0xF
	b15=1 : G2_TX_SLEW_CTRL_EN
	b14-b12=2: G2_TX_SLEW_RATE_SEL (0 fasttest, 7 slowest)
	b11=1: G2_TX_EMPH_EN
	b10-b7=0110=6 : G2_TX_POST_EMPHASIS_VALUE
	b6=1:Transmitter amplitude adjust
	b5-b1=10110=0x16: TX_Amplitude
	b0=0: SSC_EN=0 */
	writel(0xAD6C, priv->base + G2_SETTING_0);

	/* g2_setting RX 0x10
	sendln 'WD F7A25040 BD2'; Per recommendation PHY Eugene
	b10=0: DFE_ENABLE
	b9-b8=11: G2_RX_SELMUFF[1:0] -> 1/2048 (smallest bandwidth) */
	writel(0xBD2, priv->base + G2_SETTING_RX);

	/* Impedance
	sendln 'WD F7A25104 A708'; Per recommendation PHY Eugene */
	writel(0xA708, priv->base + IMPEDANCE_REG);

	/* pcie_reg0
	b12: IDLE_SYNC_EN
	sendln 'WD F7A25120 1060'; Per spec recommendation. Enable IDLE_SYNC */
	writel(0x1060, priv->base + PCIE_REG0);

	/* GLOB_CLK_SRC_LO :0x1C3
	b7:5:"000" --> 0 per recommendation from PHY team
	b0=1 --> PCLK generated from PLL */
	writel(0x01, priv->base + GLOB_CLK_SRC_LO);
	/*Wait 1ms BandGap*/
	udelay(1000);

	/* LANE_CONFIG4: 0x188
	b12:8="0010" --> 25Mhz
	b4: 1 --> CFG_DFE_PAT_SEL per recommendation from Eugene
	b2:0="011" --> PIN_DEF_EN=1, PIN_DFE_PAT_DIS=1 PIN_DFE_UPDATE_DIS=0 */
	writel(0x213, priv->base + LANE_CONFIG4);

	/* GLOB_PM_CFG0: 0x1D0
	b11:8="001" --> CFG_PM_RXDEN_WAIT[3:0]
	b7:0="0x07" --> CFG_PM_RXDLOZ_WAIT[7:0] */
	writel(0x107, priv->base + GLOB_PM_CFG0);

	/* power_reg0 : 0x01
	b[14]=1 --> PU_PLL=1
	b[13]=1 --> PU_RX=1
	b[12]=1 --> PU_TX=1
	b[10]=1 --> PU_DFE=1
	b[7:5]="101" --> USB3 mode
	b[4:0]="0x02" --> REFCLK = 25Mhz */
	writel(0xFCA2, priv->base + POWER_REG0);

	/* cal_reg0:0x02
	b[7:2]="010000"=0x10 --> SPEED[5:0] */
	writel(0x040, priv->base + CAL_REG0);

	/* GLOB_DB_CFG : 0x1C8
	b[2:1]="01" --> CFG_GEN1_TXELECIDLE_DLY[1:0] 2'b01: tx_idle_hiz is delayed by 1 clock cycle
	b0=0 --> CFG_IGNORE_PHY_RDY, 1:rxvalid logic ignored phy_rdy */
	writel(0x2, priv->base + GLOB_DB_CFG);

	/* interface_reg1 : 0x25
	b[11:10]="01" --> 5GBps USB3 */
	writel(0x400, priv->base + INTERFACE_REG1);

	/* isolate_reg0 : 0x26
	b11=1: AUTO_RX_INIT_DIS,1: Disable automatic rx_init trigger after power up,
	partial slumber/slumber and speed change
	b8=1: TX_DRV_IDLE : 1: Driver at common mode voltage
	b[7:4]="0x1": PHY_GEN_TX[3:0], 5GBbps USB3
	b[3:0]="0x1": PHY_GEN_RX[3:0], 5GBbps USB3 */
	writel(0x911, priv->base + ISOLATE_REG0);

	/* Set SSC_EN_FROM_PIN to 0.
	R5F[14]  / SSC_EN_FROM_PIN   : 1 => 0
	default = 0x45CF */
	writel(0x5CF, priv->base + CFG_SSC_CTRL);

	/* Set Amplitude, emphasis and SSC
	RF[15]    G2_TX_SLEW_CTRL_EN  = 1
	RF[14:12] G2_TX_SLEW_RATE_SEL = 2h => 0h
	RF[11]    G2_TX_EMPH_EN = 1
	RF[10:7]  G2_TX_EMPH    = Ah
	RF[6]     G2_TX_AMP_ADJ = 1
	RF[5:1]   G2_TX_AMP     = 16h => 1Fh
	RF[0]     G2_TX_SSC_EN  = 0 => 1
	default = AD6Ch */
	writel(0xAD6D, priv->base + G2_SETTING_0);

	/* Adjust SSC amplitude to meet spec
	R3E[15:9]   G2_TX_SSC_AMP  = 18h  => 27h
	default = 3100h (just 3096ppm, not enough) */
	writel(0x4F00, priv->base + G2_TX_SSC_AMP);

	/* Ref clock 25Mhz, interface_reg2: 0x51
	b8=1 --> REF1M_GEN_DIV_FORCE,Reference 1 MHz Generation Divider Force.
	b[7:0]=0x10 -->Reference 1 MHz Generation Divider. */
	writel(0x080, priv->base + INTERFACE_REG2);

	/* FFE : No documentation.
	sendln 'WD F7A25448 44' ; Per Eugene recommendation */
	writel(0x44, priv->base + 0x448);

	/* Reserved; No documentation
	sendln 'WD F7A255EC 0F0' ; Per Eugene recommendation */
	writel(0x0F0, priv->base + 0x5EC);

	/* misc_reg0:0x4F
	b8=0 --> Output 100Mhz clock PIN
	b7=1 --> Enable 500Mhz clock
	b6=1 --> TXDCLK_2X_SEL,1: PIN_TXDCLK_2X sends out 500MHz clock
	b[3:0]=0xd --> ICP = 360uA */
	writel(0xA0CD, priv->base + MISC_REG0);

	/* WaiKit tune up value for compliance 04/20/2015*/
	data = readl(priv->base + CFG_TX_AMP);
	data &= 0xFFFF81FF;
	data |=0x7000; // force tx_amp
	writel(data, priv->base + CFG_TX_AMP);

	/* Deassert Soft reset */
	writel(0x28, priv->base + GLOB_CLK_CTRL);

	retry = PHY_STATUS_POLLING_RETRY;
	do {
		data = readl(priv->base + LANE_STATUS1);
		if (data & PM_TXDCLK_PCLK_EN_MASK)
			break;
		else {
			udelay(PHY_STATUS_POLLING_INTERVAL);
			retry--;
		}
	} while(retry);

	retry = PHY_STATUS_POLLING_RETRY;
	do {
		data = readl(priv->base + LANE_STATUS2);
		if (!(data & 0x400))
			break;
		else {
			udelay(PHY_STATUS_POLLING_INTERVAL);
			retry--;
		}
	} while(retry);

	/* Now the PIPE-PHY power up sequence is done.
	PIPE-PHY is ready to run. Now Software can start link layer configuration.*/
	if (!retry)
		return -1;

	return 0;
}

static const struct phy_ops phy_berlin4_usb3_ops = {
	.power_on	= phy_berlin4_usb3_power_on,
	.owner		= THIS_MODULE,
};

static const struct of_device_id phy_berlin4_usb3_of_match[] = {
	{
		.compatible = "marvell,berlin4ct-usb3-phy",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, phy_berlin4_usb3_of_match);

static int phy_berlin4_usb3_probe(struct platform_device *pdev)
{
	struct phy_berlin4_usb3_priv *priv;
	struct resource *res;
	struct phy *phy;
	struct phy_provider *phy_provider;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	phy = devm_phy_create(&pdev->dev, NULL, &phy_berlin4_usb3_ops);
	if (IS_ERR(phy)) {
		dev_err(&pdev->dev, "failed to create PHY\n");
		return PTR_ERR(phy);
	}

	phy_set_drvdata(phy, priv);

	phy_provider =
		devm_of_phy_provider_register(&pdev->dev, of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(phy_provider);
}

static struct platform_driver phy_berlin4_usb3_driver = {
	.probe	= phy_berlin4_usb3_probe,
	.driver	= {
		.name		= "phy-berlin4-usb3",
		.of_match_table	= phy_berlin4_usb3_of_match,
	},
};
module_platform_driver(phy_berlin4_usb3_driver);

MODULE_AUTHOR("Jisheng Zhang <jszhang@marvell.com>");
MODULE_DESCRIPTION("Marvell Berlin4 PHY driver for USB3.0");
MODULE_LICENSE("GPL v2");
