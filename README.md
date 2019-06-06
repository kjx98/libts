libTS3 基础程序库
=================

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
	* MSVC: 已在 libts/include/ts3/types.h 里面设定: #pragma execution_character_set("UTF-8")
	* GCC: 默认值已是 UTF-8, 或可强制使用: -fexec-charset=UTF-8 参数
在众多 open source 的情况下，为何还要写？
* 追求速度
	* std 、 boost 非常好，但速度不是他们追求的。
	* 当 **速度** 与 **通用性** 需要取舍时，libts3 选择 **速度**。
	* 当 **速度** 与 **内存RAM用量** 需要取舍时，libts3 选择 **速度**。
		* libts3 会考虑到 大量的RAM 造成 CPU cache missing，速度不一定会变快。
	* 速度问题分成2类: Low latency、High throughtput，当2者有冲突时，尽量选择 **Low latency**。
* 降低第3方的依赖。
* 更符合自己的需求。
	* 许多第3方的 library，速度很快、功能强大，但也远超过我的需求。
	* 我个人倾向于：设计到刚好满足自己的需求就好。

### 准备工作
主要开发工具及版本
* Windows: VS 2015
	* ./build/vs2015/ts3.sln
* Linux: CentOS 7 w/ devtoolset-8 and epel, or fedora 30 w/ gcc9
	* cmake version 3.13
	* gcc 8
	* build via cmake
* 开启 compiler 全部的警告信息：警告信息的重要性，相信不用再提醒了。
	* 警告信息 -- 零容忍。

