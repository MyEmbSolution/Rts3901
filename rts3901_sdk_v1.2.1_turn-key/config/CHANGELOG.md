# IPCam SDK Change Log

All notable changes to this project will be documented in this file.

Changes should be grouped to describe their impact on the project.
The group labels are listed as follows:

- **Added**: for new features.
- **Changed**: for changes in existing functionality.
- **Deprecated**: for once-stable features removed in upcoming releases.
- **Removed**: for deprecated features removed in this release.
- **Fixed**: for any bug fixes.
- **Security**: to invite users to upgrade in case of vulnerabilities.

## 1.2.1 - 2017-01-16

### Added
- SDK: Update AAC encoder to suport sample rate 8000 and 16000
- SDK: add API to set NS level
- turn-key: Add options IR-Mode, CVBR and GOP to webpage

### Changed
- SDK: Don't match magic ID when update partition
- SDK: Keep mcu clock always on
- SDK(H264): Change the way of kbl calculation
- SDK(H264): Change i-p frame qpmin from 10 to 20 for rs cbr
- SDK(H264): Change qpmin from 20 to 10 for cvbr
- turn-key: Change ffm muxer to shm muxer

### Fixed
- SDK(OSD): Fix memory leak
- SDK(H264): Decrease heart-beat effect and false color
- SDK(H264): Add range restriction of p frame min qp and diff sum
- turn-key: Set max delay when use rtpenc muxer
- turn-key: Set MD and Mask by relative coordinates
- turn-key: update max_bitrate and min_bitrate in FFMpeg rtstream device

## 1.2 - 2016-11-23

### Added
- SDK: pass flash partition info through kernel command line
- turn-key: export new API to access configuration
- turn-key: add lark to process audio's content
- turn-key: support IR-Mode

## 1.1.2 - 2016-11-23

### Fixed

- kernel: mark warning trace for USB Wifi driver
- kernel: select amic for default
- u-boot: enable partition in bsp
- u-boot: refer partition.ini in bsp in default
- rtscore: change rc params when gop was changed
- turn-key: set resolution type in octopus_plugin

### Added

- u-boot: add DDR3 MCM option




## 1.1.1 - 2016-11-10

### Fixed
- kernel: Fix pinctrl request issue
- kernel: Fix uboot config issue
- turn-key: Fix the issue of VLC plugin refresh
- turn-key: Fix the issue of snapshot fail
- turn-key: Fix the issue of reset ISP

## 1.1 - 2016-08-10

### Fixed
- kernel: Add barrier to ensure the data consistency in SD command buffer
- kernel: call xb2flush to make sure force reset does work
- kernel: call xb2flush to make sure clock setting does work
- kernel(usbphy): negative-edge to optimize timing
- turn-key: correct the way of reading ldc table
- turn-key: correct typos to avoid deadlock in libsysconf
- turn-key: load driver before mount JFFS2

### Added
- kernel: move vendor driver out of kernel source tree
- kernel: get MAC address in user mode
- kernel: support Wifi monitor mode
- kernel: support control of ethernet LED
- turn-key: use rtstream instead of ffmpeg alsa
- turn-key: use timestamp to process aec
- turn-key: write rtp mode to rtpenc muxer's metadata

### Changed
- turn-key: refactor rts_opt_cam with rtstream
- turn-key: optimize rtpenc muxer
- audio: change the analog amic boost for rts3901C

## 1.0.2 - 2016-08-08


### Fixed
- rtscore: free object when disable OSD
- rtscore: calc sum of mbmv only in cvbr mode
- rtscore: add more rtstream sample
- turn-key: reset OSD when bitrate changed

### Added
- kernel: export SoC HW ID
- kernel: adjust CPU frequency dynamically

### Changed

## 1.0.1 - 2016-06-14

### Fixed
- MP4: adjust sps position to avoid unneeded zeroes
- MP4: donot write sps info to mdat
- turning-server: RealCam TCP sync sometimes fail
- record: monitor free memory when recording
- peacock: fix AV sync issue caused by AEC
- librtsio: fix get gpio value may get wrong value
- kernel: fix bug of can not reboot in kernel when using flash 'w25q128fv'
- kernel: refine the calculation of F clock
- kernel: change i2c sda hold time to 8
- h1encoder: update SPS/PPS to correct FPS
- kernel: correct the register definition of watchdog


### Added
- kernel(isp): adjust power sequence follow isp v1.03
- kernel(isp): support smaller resolution
- kernel(isp): support hclk and 216M source
- MP4: support g711-alaw
- SDK: add minimal default configs
- tuning_server: refine upgrade mcu fw flow for RealCam
- librtsisp: update to latest version
- librtscamkit: update to latest version
- rtstream: update to latest version
- rtstream: add awb/ae API
- rtstream: laod IQ table from flash
- rtstream: add ROI API
- rtstream: add OSD API to set bitmap
- rtstream: add rect mask API
- webpages: refactor recording scheudle
- webpages: support MD event
- webpages: sort file list and show in full path
- SDK: adjust repo layout


### Changed


## 1.0 - 2016-06-02

### Fixed
- webpages: restroe default valud for rotate when reset ISP


### Added
- SDK: update toolchain to 4.8.5
- rtstream: add API about audio control
- rtisp: update API about get/set MD/Mask coordinate
- 3rdlibrary: update shared library for 4.8.5

### Changed
- H264: update bitrate control algorithm


## 0.10 - 2016-05-24

### Fixed
- kernel(H264): adjust force reset flow of H264 as spec
- kernel(H264): synchronize the open and close operation of H264 encoder


### Added
- kernel: export MCU timeout to sysfs
- peacock: add interface to change H264 rc alg
- peacock: export API to get/set H264 profile and level

### Changed
- kernel(flash): disable auto write of SPI flash
- kernel(sd): disable OCP

## 0.9.1 - 2016-04-06

### Fixed
- peacock: set CODEC_FLAG_GLOBAL_HEADER flag
- peacock: re-recording when SPS changed
- config:  mv lighttpd user file to /etc/conf
- librtsisp: wrong gpio argument check

## 0.9 - 2016-03-10
### Fixed
- sysconf: Fix that config files may have a chance to get damaged
- kernel:mtd: Fix that kernel partition may have a chance to get damaged

### Added
- webpages: Add rotate mode
- webpages: Support plugin selection
- webpages: Add event-triggered recording
- API: Add librtsio

### Changed
- webpages: Optimize network setting

## 0.8.0 - 2015-12-24
### Fixed
- kernel:isp: Fix the bug that enabling MCU fail in some situation
- networkmanager: Fix that ipcam cannot join hidden wifi network
- octopus_cam: Fix that peacock.json would be corrupted in some situation
- webpages: Fix that update image fail in Web UI

### Added
- webpages: Support LDC
- webpages: Add Factory Reset


## 0.7.1 - 2015-12-09
### Fixed
- audio: Fix low volume level of AMIC

### Added
- webpages: Add reset button in ISP setting page

### Changed
- kernel:h264: Use USB pll in Rev. A


## 0.7.0 - 2015-11-27
### Added
- webpages: Integrate new webpage UI
- webpages: Support DDNS
- webpages: Support PPPoE
- webpages: Support SoftAP
- webpages: Support 3DNR
- webpages: Support dehaze
- kernel:wifi: Support rtl8821au and rtl8188ftv
- kernel:wifi: Enhance signal strength of rtl8188eus
- kernel:misc: Support SRAM device
- Support SquashFS
- peacock: Support RTSP over HTTP
- peacock: Support RTP over TCP
- peacock: Support Mjpeg
- octopus: Support OSD stroke
- octopus: Support dehaze and temporal denoise

### Changed
- kernel: Refactor camera driver


## 0.6.0 - 2015-10-16
### Fixed
- nm_cfg: Fix wps_pin save bug
- gaia: Fix OSD time drifting

### Added
- nm_cfg: Support WEP hexadecimal format key
- nm_cfg: Support auto-generating WPS pin code
- nm_cfg: Support PPPoE
- nm_cfg: Support the function of turning on/off wifi module
- kernel:pmu: long press the power key to power off instead of reset
- libsysconf: Add new API to save and store configurations
- gaia: Initialize ISP
- webpages: Show firmware version
- webpages: Support WPS
- webpages: Add switch to turn on/off wifi module
- webpages: Refactor webpages with bootstrap framework
- octopus_plugin: Support grid mode of private mask and motion detect
- octopus_plugin: Add API to get/set H264 encoder attribute
- octopus_plugin: Add API to create wifi station WPS pin code
- octopus_plugin: Add API for PPPoE function
- octopus_plugin: Add API to turn on/off wifi module
- octopus: Map alsa device to domain cam
- onvif: Support SetSynchronizationPoint


## 0.5.1 - 2015-08-31
### Fixed
- nm_cfg: Fix that writing wrong ipaddr to hw config partition in some situation

### Added
- nm_cfg: Save netmask and gateway to hw config partition

## 0.5.0 - 2015-08-18
### Added
- SDK version number will be kept in config/VERSION file
