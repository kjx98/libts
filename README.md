libTS3 基础库
=================

[![Build Status](https://travis-ci.org/kjx98/libts.svg?branch=master)](
https://travis-ci.org/kjx98/libts)

## 一、基本说明
* libTS3 是 Jesse Kuang 独力开发的「C++ 跨平台交易基础设施」程序库
* 使用 C++11 (C++17 in near future)
	* Windows: 
	* Linux:
		* GCC 7.2 or newer
		* cmake version 3.10 or newer
* 跨平台Linux优先，但不支持老旧的OS
* 仅考虑 64 位平台
* 字符串仅支持 UTF8，source file 也使用 UTF8 编码。GBK应转码
	* MSVC: 已在 include/ts3/types.h 里面设定: #pragma execution_character_set("UTF-8")
	* GCC: 默认值已是 UTF-8, 或可强制使用: -fexec-charset=UTF-8 参数
在众多 open source 的情况下，为何还要写？
* 追求速度
	* std 、 boost 非常好，但速度不是他们追求的。
	* 当 **速度** 与 **通用性** 需要取舍时，libts3 选择 **速度**。
	* 当 **速度** 与 **内存RAM用量** 需要取舍时，libts3 选择 **速度**。
		* libts3 会权衡 大量的RAM 造成 CPU cache missing，速度不一定会变快。
	* 速度问题分成2类: Low latency、High throughtput，当2者有冲突时，尽量选择 **Low latency**。
* 降低第3方的依赖。
* 更符合自己的需求。
	* 许多第3方的 library，速度很快、功能强大，但也远超过我的需求。
	* 我个人倾向于：设计到刚好满足自己的需求就好。

### 准备工作
主要开发工具及版本
* Linux: CentOS 7 w/ devtoolset-8 and epel, or fedora 30 w/ gcc9
	* cmake version 3.13
	* gcc 8
	* build via cmake
* Windows: VS 2015	**(TODO)**
	* ./build/vs2015/ts3.sln
* 开启 compiler 全部的警告信息：警告信息的重要性，相信不用再提醒了。
	* 警告信息 -- 零容忍。

## 二、STL 库
  julian 日期， message 消息包抽象，serialization 实现POD简单类型的序列化与反序列化，timestamp 低延迟微秒/纳秒级计时与时间戳，priceType抽象针对价格及金额的定点

### julian
  儒略日数（Julian Day Number，JDN）的计算是从格林威治标准时间的中午开始，包含一个整天的时间，起点的时间（0日）回溯至儒略历的公元前4713年1月1日中午12点（在格里历是公元前4714年11月24日），这个日期是三种多年周期的共同起点，且是历史上最接近现代的一个起点。
  1970年1月1日， julian 日为2440588。采用julian日， 一个16位整数即可表示约180年，而且计算星期几只需要取7的余数即可（即0表示周一，6表示周日）

### serialization
  简单序炼化与反序列化，采用Little Endian（即小端编码，Intel/ARM64的CPU整数字节编码），字符串string以及字节串Bytes采用Pascal编码（长度0...255）以一字节表示长度后续bytes
  模板成员函数decode/encode支持intergral类型（即单字节/2/4/8字节数量值）

### timestamp
  timeval类 简单包封timespec结构并采用int32_t 表示秒以及纳秒
  sysclock类 用于重演/模拟环境的系统时钟
  timestamp 抽象毫秒/微秒/纳秒级别时间戳
  DateTime 抽象 毫秒/微秒/纳秒级别 的日期时间点

### priceType
  定点数与浮点double的相互转换

### types
  增加int48_t 	定义
### Math
formula: $ f(x) = \int_{-\infty}^\infty \hat f(\xi)e^{2 \pi \xi x}d\xi $
