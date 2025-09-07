# Git 使用

## 1. Git 拉取project代码

- 在gitlab页面点击左上角登录的用户
- 点击 Edit profile ，选择 SSH-Keys添加要clone代码机器的 SSH 公钥
- 本机生成 SSH 公钥的方法：

```
$ ssh-keygen # -C 邮箱？忘了
$ $ cat ~/.ssh/id_rsa.pub // 将输出的内容复制到刚刚网页的SSH-Keys中
```

- 然后就可以在对应的机器通过clone命令拉取代码，链接来自project的右边code的ssh链接
  （默认情况下会拉取远程仓库的主分支）

```
$ git clone url # 来自project 的 code url，默认情况下会拉取远程仓库的主分支
$ git clone -b <branch_name> url # 拉取指定分支的代码，-b or --branche
```

- 拉取远程仓库的最新修改并合并到本地

```
$ git pull # 拉取远程仓库的最新修改并合并到本地仓库
$ git pull --rebase # 将远程分支的更改重新应用到你的分支上，而不是创建一个新的合并提交。
$ git fetch # 仅拉取远程仓库的最新修改，但不影响本地修改，可以查看最新的修改和分支，可以通过git merge进行合并
```

## 2. Git 分支增删改查

- 查看拉取的分支或当前所处的分支（当前分支前面有个*号）

```
$ git branch
```

- 查看所有分支（包括远程分支）

```
$ git branch -a
$ git branch -r # 查看远程分支
```

- 切换分支

```
$ git checkout <branch_name>
```

- 创建分支（新分支是基于当前所处的 commit 或者 HEAD 指向的版本）

```
$ git branch <branch_name> # 创建分支，但不切换分支
$ git checkout -b <branch_name> # 创建分支并切换
```

- 重命名分支（将本地分支 `<old_branch_name>` 重命名为 `<new_branch_name>`）

```
$ git branch -m <old_branch_name> <new_branch_name>
```

- 删除分支

```
$ git branch -d <branch_name>
```

- 推送分支到远程仓库

```
$ git push origin <branch_name> # 将创建或修改的分支推送到远程仓库
$ git push origin -d <branch-name> # 将删除的分支推送到远程仓库
```

## 3. Git 提交本地修改

- 查看本地仓库的修改情况和暂存情况

```
$ git status # or git st
```

- 保存本地修改到暂存区，以便后续提交

```
$ git add . 
$ git add <file_name>
```

- 撤销保存到本地暂存区的修改，但保留工作目录的修改

```
$ git reset HEAD <file> # 撤销 git add <file> 的效果，HEAD指代暂存区
$ git reset HEAD . # 撤销 git add . 的效果
```

- 将暂存区的修改生成快照并提交到本地仓库

```
$ git commit -m "commit info"
```

- 撤销提交到本地仓库的修改快照

```
$ git reset HEAD~1 # 撤销commit并保留本地修改，1表示倒数哪个版本
$ git reset --hard HEAD~1 # 撤销 commit 并且丢弃所有修改的内容
```

- 将本地仓库的提交推送到远程仓库

```
$ git pull # 先拉取远程仓库的最新版本合并到本地仓库！因为不能跨版本提交！
$ git push # 将本地仓库的提交推送到远程仓库
```

- 撤销推送到远程的提交？

将本地仓库回退，然后提交就版本内容？？？？？？

## 4. Git保存临时改动

- 所有未提交的修改（包括已暂存和未暂存的）保存到 Git 的堆栈中，并且重置工作目录和暂存区，使其和最后一次提交的状态保持一致

```
$ git stash
```

- 查看保存的临时修改（列出所有保存在堆栈中的临时修改，每一条记录都有一个唯一的标识符（stash ID））

```
$ git stash list
```

- 恢复保存的临时修改，可以使用以下命令

```
$ git stash apply #恢复最新的 stash
$ git stash apply stash@{<stash-id>} # 恢复特定的 stash
```

- 删除临时的修改

```
$ git stash drop stash@{<stash-id>} #删除指定的临时修改
$ git stash drop #删除最新的 stash
```

## Git rebase 调整提交记录

```
$ git checkout master
$ git pull
$ git checkout dev
$ git rebase master
$ # 解决冲突
$ git push --force-with-lease #远程分支没有新的提交时才会强制推送
```

## Git 常用命令别名

```
git config --global alias.st "status"
git config --global alias.br "branch"
git config --global alias.lg "log --color --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset' --abbrev-commit"
```