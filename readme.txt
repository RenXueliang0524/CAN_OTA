BootLoader 写：0x08000000 - 0x0801FFFF，大小 128 KB，用于 BootLoader。

App1 写：0x08020000 - 0x080FFFFF，大小 896 KB，用于应用程序 1。

App2 写：0x08100000 - 0x081DFFFF，大小 896 KB，用于应用程序 2。

OTA Info 写：0x081E0000 - 0x081FFFFF，大小 128 KB，用于保存当前运行分区、升级标志和元数据。
