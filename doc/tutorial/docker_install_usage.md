# docker 的安装和基本使用

## docker 安装

​1. ​安装依赖包​​：需要安装 yum-utils（它提供 yum-config-manager工具）、device-mapper-persistent-data和 lvm2

```
sudo yum install -y yum-utils device-mapper-persistent-data lvm2
```


2. 添加 Docker 的官方仓库

对于国内用户，为加速下载，可以考虑使用国内镜像源，例如阿里云

```
sudo yum-config-manager --add-repo http://mirrors.aliyun.com/docker-ce/linux/centos/docker-ce.repo
```


3. 安装 Docker 引擎​

使用 yum 安装 Docker 及其相关组件.此命令会安装最新稳定版本的 Docker Community Edition (CE)。

```
sudo yum install -y docker-ce docker-ce-cli containerd.io
```

4. 启动Docker服务

```
sudo systemctl start docker
```

5. 设置开机自启动

```
sudo systemctl enable docker
```

6. 验证释放安装正确

```
sudo docker run hello-world
```

7. 管理用户权限

默认情况下，执行 Docker 命令需要 sudo权限。为了让非 root 用户也能运行 Docker 命令，可以将用户添加到 docker用户组

```
sudo usermod -aG docker $USER
```

8. 配置镜像加速器​

```
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json <<-'EOF'
{
  "registry-mirrors": ["https://<你的镜像加速器地址>"]
}
EOF
```

9. 重新加载配置重启服务

```
sudo systemctl daemon-reload
sudo systemctl restart docker
```

10 . 验证 hello-world 失败

```
sudo docker run hello-world
Unable to find image 'hello-world:latest' locally

docker: Error response from daemon: Get "https://registry-1.docker.io/v2/": context deadline exceeded (Client.Timeout exceeded while awaiting headers)

Run 'docker run --help' for more information
```

