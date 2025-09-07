# 修改 linux 的 core 生成路径

## 临时修改

```
echo "core.%e-%p-%t" >> /proc/sys/kernel/core_pattern
cat /proc/sys/kernel/core_pattern
```

## 永久修改

```
sudo sysctl -w kernel.core_pattern="core.%e-%p-%t"
cat /proc/sys/kernel/core_pattern
```