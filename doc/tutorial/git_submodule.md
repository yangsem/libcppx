# git submodule 使用说明

## 使用目的

为 GitHub 仓库添加子模块（外部依赖），易于管理和了解依赖情况

## 添加子模块

```
git submodule add <repo-url> [<path>]
```

指定分支,打开 .gitmodules 文件，增加 branch 字段，如：
```
[submodule "3rd"]
	path = 3rd/googletest
	url = https://github.com/google/googletest.git
	branch = v1.12.x # Use branch v1.12.x for C++11 support
```

## 移除子模块

```
git submodule deinit [<path>]
git rm [<path>]
rm -rf .git/modules/xxx
```

## 使用子模块

```
git submodule init
git submodule update

# --recursive：递归地更新所有子模块（包括子模块的子模块）。
# --remote：从子模块的远程仓库拉取最新的更改。
git submodule update --recursive --remote
```

## 查看子模块状态

```
git submodule status
```