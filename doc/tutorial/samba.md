# samba 挂在服务器磁盘到window

## 服务器安装 samba

```
$ sudo apt-get install samba
```

## 配置 smaba 服务

打开配置文件
```
$ vim /etc/samba/smb.conf
```

追加内容(不需要这个，直接跳过)：
```
[YourShareName]   # 共享名称，Windows 上会看到这个名称
comment = Your Description  # 共享描述
path = /path/to/your/directory  # 要共享的 Linux 目录的**绝对路径**
browseable = yes    # 是否可浏览
writable = yes      # 是否可写
read only = no      # 是否只读
valid users = your_samba_user  # 允许访问的 Samba 用户
```

添加 smaba 用户（必须是linux存在的用户）
```
sudo smbpasswd -a your_username
```

## 启动 samba 服务

检查服务是否启动
```
$ sudo systemctl status smb

○ smb.service - Samba SMB Daemon
     Loaded: loaded (/usr/lib/systemd/system/smb.service; disabled; preset: disabled)
     Active: inactive (dead)
       Docs: man:smbd(8)
             man:samba(7)
             man:smb.conf(5)
```

启动 samba 服务
```
sudo systemctl start smb nmb
```

## 防火墙检查

防火墙放行的服务
```
sudo firewall-cmd --list-services
```

防火墙放行的端口
```
sudo firewall-cmd --list-ports
```

添加 samba 服务到防火墙（永久生效）
```
sudo firewall-cmd --add-service=samba --permanent
```

重新加载防火墙规则​
```
sudo firewall-cmd --reload
```

验证是否放行
```
sudo firewall-cmd --list-services | grep samba
```

## 设置开机自启动
```
sudo systemctl enable smb nmb
```

## window 访问

打开此电脑 -> 映射网络驱动器 -> \\ip\user

提升报错，待解决
```
您没有权限访问\\192.168.1.X（局域网） 请与网络管理员联系请求访问权限
``