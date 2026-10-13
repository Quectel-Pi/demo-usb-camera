# USB Camera

[English](README.md)


USB Camera 鏄竴涓熀浜?Qt Quick/QML 鍜?C++ 鐨?Linux 妗岄潰鐩告満搴旂敤锛屾敮鎸佸疄鏃堕瑙堛€佹媿鐓с€佸綍鍍忋€佸鎽勫儚澶撮€夋嫨銆佽澶囬噸鎵拰閿欒鎻愮ず銆?

## 鍔熻兘

- 瀹炴椂鎽勫儚澶撮瑙?
- 鎷嶇収淇濆瓨涓?JPG
- 褰曞儚淇濆瓨涓?MP4
- 澶氭憚鍍忓ご涓嬫媺閫夋嫨
- 閲嶆柊鎵弿鎽勫儚澶?
- 缂哄皯鏉冮檺銆佺己灏戞彃浠躲€佺紪鐮佸櫒涓嶅彲鐢ㄧ瓑閿欒鎻愮ず
- 鑷姩閫夋嫨鏈€浣抽噰闆嗘ā寮?
- 鍙厤缃?H.264 褰曞儚缂栫爜鍣?

## 鎶€鏈爤

- 鍓嶇锛歈t Quick / QML / Qt Quick Controls 2
- 鍚庣锛欳++11 / Qt / GStreamer 1.0 / Linux V4L2
- 鏋勫缓锛歲make / make / pkg-config

璇︾粏鏋舵瀯璇疯 [ARCHITECTURE.md](ARCHITECTURE.md)銆?

## 纭欢瑕佹眰

- Linux 璁惧
- 涓€涓垨澶氫釜 V4L2 鍙瘑鍒殑 USB UVC 鎽勫儚澶?
- 鎽勫儚澶磋嚦灏戞敮鎸?`MJPG` 鎴?`YUYV`
- 褰曞儚闇€瑕佺郴缁熶腑鑷冲皯鏈変竴涓彲鐢?H.264 缂栫爜鍣ㄦ彃浠?

鎺ㄨ崘鎽勫儚澶存敮鎸?MJPEG 鍜?30 FPS 鎴栨洿楂樺抚鐜囥€?

## 渚濊禆

闇€瑕佸畨瑁咃細

- Qt 5.15 鎴栧吋瀹圭増鏈?
- Qt GUI / Qt QML / Qt Quick / Qt Quick Controls 2
- GStreamer 1.0 寮€鍙戝寘
- GStreamer app/video 寮€鍙戝寘
- GStreamer 鎻掍欢锛歚v4l2src`銆乣appsink`銆乣appsrc`銆乣jpegdec`銆乣videoconvert`銆乣h264parse`銆乣mp4mux`
- 鑷冲皯涓€涓?H.264 缂栫爜鍣細`mpph264enc`銆乣x264enc`銆乣openh264enc` 鎴?`avenc_h264`
- C++ 缂栬瘧鍣ㄣ€乹make銆乵ake銆乸kg-config

Ubuntu/Debian 涓婄殑鍖呭悕浼氶殢鍙戣鐗堜笉鍚岃€屽彉鍖栵紝閫氬父闇€瑕?Qt 寮€鍙戝寘銆丟Streamer base/good/bad/ugly/libav 鎻掍欢闆嗗悎浠ュ強瀵瑰簲寮€鍙戝寘銆?

## 鏋勫缓

```bash
qmake camapp.pro
make
```

## 杩愯

```bash
./camapp
```

濡傛灉鎽勫儚澶存棤鏉冮檺锛屽厛纭褰撳墠鐢ㄦ埛鏄惁鍦?`video` 鐢ㄦ埛缁勪腑锛?

```bash
groups
```

鍙互灏嗗綋鍓嶇敤鎴峰姞鍏?`video` 鐢ㄦ埛缁勶紝閲嶆柊鐧诲綍鍚庣敓鏁堬細

```bash
sudo usermod -aG video "$USER"
```

## 褰曞儚缂栫爜鍣ㄩ厤缃?

搴旂敤榛樿鑷姩閫夋嫨绗竴涓彲鐢ㄧ紪鐮佸櫒锛?

```text
mpph264enc -> x264enc -> openh264enc -> avenc_h264
```

涔熷彲浠ラ€氳繃鐜鍙橀噺鎸囧畾锛?

```bash
CAMAPP_VIDEO_ENCODER=x264enc ./camapp
```

鏀寔鐨勫€硷細

```text
mpph264enc
x264enc
openh264enc
avenc_h264
```

濡傛灉鎸囧畾缂栫爜鍣ㄤ笉鍙敤锛屽簲鐢ㄤ細鎻愮ず瀹夎瀵瑰簲 GStreamer 鎻掍欢鎴栧彇娑?`CAMAPP_VIDEO_ENCODER`銆?

## 妫€鏌ョ幆澧?

妫€鏌ユ憚鍍忓ご璁惧锛?

```bash
ls /dev/video*
```

妫€鏌?GStreamer 鎻掍欢锛?

```bash
gst-inspect-1.0 v4l2src
gst-inspect-1.0 jpegdec
gst-inspect-1.0 x264enc
gst-inspect-1.0 openh264enc
gst-inspect-1.0 avenc_h264
gst-inspect-1.0 mpph264enc
```

鍙鑷冲皯涓€涓?H.264 缂栫爜鍣ㄥ瓨鍦紝褰曞儚鍔熻兘鍗冲彲灏濊瘯鍚姩銆?

## 杈撳嚭鏂囦欢

鐓х墖榛樿淇濆瓨鍒帮細

```text
~/Pictures/Camera/
```

瑙嗛榛樿淇濆瓨鍒帮細

```text
~/Videos/Camera/
```

鏂囦欢鍚嶆牸寮忥細

```text
IMG_yyyyMMdd_HHmmss_zzz.jpg
VID_yyyyMMdd_HHmmss.mp4
```

## 甯歌闂

### 娌℃湁鎽勫儚澶寸敾闈?

纭鎽勫儚澶村凡杩炴帴锛屽苟涓旂郴缁熻兘鐪嬪埌 `/dev/video*`銆傚鏋滃簲鐢ㄦ彁绀烘潈闄愰敊璇紝璇峰皢褰撳墠鐢ㄦ埛鍔犲叆 `video` 鐢ㄦ埛缁勬垨璋冩暣璁惧鏉冮檺銆?

### 搴旂敤鎻愮ず缂哄皯 GStreamer 鎻掍欢

鏍规嵁閿欒淇℃伅瀹夎瀵瑰簲鎻掍欢銆傞瑙堥€氬父闇€瑕?`v4l2src`銆乣jpegdec`銆乣videoconvert`銆乣appsink`锛屽綍鍍忚繕闇€瑕?`appsrc`銆丠.264 缂栫爜鍣ㄣ€乣h264parse` 鍜?`mp4mux`銆?

### 褰曞儚鍚姩澶辫触

鍏堟鏌ユ槸鍚﹀瓨鍦ㄥ彲鐢ㄧ紪鐮佸櫒锛?

```bash
gst-inspect-1.0 x264enc
gst-inspect-1.0 openh264enc
gst-inspect-1.0 avenc_h264
gst-inspect-1.0 mpph264enc
```

鐒跺悗鍙互鏄惧紡鎸囧畾涓€涓凡瀹夎缂栫爜鍣細

```bash
CAMAPP_VIDEO_ENCODER=x264enc ./camapp
```

### 澶氫釜鎽勫儚澶存椂濡備綍鍒囨崲

搴旂敤搴曢儴鎻愪緵鎽勫儚澶翠笅鎷夋銆傚綍鍍忚繃绋嬩腑浼氱鐢ㄥ垏鎹紝鍋滄褰曞儚鍚庡彲閲嶆柊閫夋嫨鎽勫儚澶淬€?

## 寮€婧愬墠娓呯悊

璇蜂笉瑕佹彁浜ゆ瀯寤轰骇鐗╋紝渚嬪锛?

- `camapp`
- `Makefile`
- `.qmake.stash`
- `*.o`
- `moc_*`
- `qrc_*.cpp`

杩欎簺宸插湪 `.gitignore` 涓拷鐣ャ€?



