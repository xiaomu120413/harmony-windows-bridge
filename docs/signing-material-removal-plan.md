# 签名材料出库方案（M1）

## 问题
`tools/app/material/{ac,ce,fd}` 是 hvigor 加密签名密码（`storePassword`/`keyPassword`）的解密密钥材料，随仓库 git 跟踪提交（`git ls-files` 命中）。任何人克隆仓库后运行 `harmony/scripts/windows/decrypt-hvigor-password.js` 即可还原明文 keystore 密码。这与 README「不要提交签名材料」自相矛盾——加密的解密密钥本身已提交，加密形同虚设。

## 方案（需用户决策执行，本文档不执行任何 git 操作）

### 步骤 1：移除 git 跟踪（保留本地）
```bash
git rm --cached -r tools/app/material
```
- `--cached` 只移除 git 跟踪，本地文件保留（构建仍可用）。
- 提交后新克隆者无 material，需团队分发。

### 步骤 2：加入 .gitignore
在 `.gitignore` 加：
```
tools/app/material/
```

### 步骤 3：团队私有分发
- material 目录打包加密（如 7z/zip + 密码），密码通过面交/私有渠道（非仓库）分发给有权限的构建者。
- 或放团队私有 CI secret 存储，构建时注入。

### 步骤 4：CI/本地构建注入解密密钥
`signing-passwords.ps1` 已支持 env：`HAP_STORE_PASSWORD`/`HAP_KEY_PASSWORD`（:10-15）。CI 用 secret 注入这两个 env，本地构建者用环境变量或参数传入。这样 material 即使不在仓库，CI 也能解密 build-profile 的加密密码（或直接用 env 明文密码绕过 build-profile 加密）。

### 步骤 5（可选，破坏性）：git 历史重写
`git rm --cached` 只移除未来跟踪，**历史提交里仍有 material**（`git log -p` 可见）。彻底移除需重写历史：
- `git filter-repo --path tools/app/material --invert-paths`（推荐，需安装 git-filter-repo）
- 或 BFG `bfg --delete-folders material`
- **破坏性**：重写历史改变所有提交 SHA，团队需 `git reset --hard origin/main` + 强制拉取，开源仓库需公告。
- 仅当 material 曾公开泄露时才必须做；私有仓库可仅步骤 1-4 防止未来。

## 影响分析
- **新克隆者**：无 material，首次构建需团队分发 material 或用 env 密码。
- **CI**：用 secret 注入 `HAP_STORE_PASSWORD`/`HAP_KEY_PASSWORD`，不依赖 material。
- **现有本地**：material 保留本地，构建不受影响。
- **历史**：步骤 1-4 不改历史（material 仍在历史提交，但未来不跟踪）；步骤 5 重写历史彻底移除。

## 决策点（待用户）
1. 是否执行步骤 1-4（移除跟踪 + .gitignore + 团队分发 + CI env）？
2. 是否执行步骤 5（git 历史重写，破坏性）？仅当 material 曾公开泄露时建议。
3. material 分发渠道（私有 CI secret / 加密包面交 / 其他）？

## 状态
- **步骤 1-2 已执行**（2026-09-15）：`git rm --cached -r tools/app/material` + `.gitignore` 加入 `tools/app/material/`。
- **步骤 5 已执行**（2026-09-16）：`git filter-repo --path tools/app/material --invert-paths --force` 重写 484 提交移除 material 历史 + `git push --force origin main`（`e3a85d7→e5c83ba`）。material 从公开 GitHub 仓库历史移除。force push 前备份 material 到仓库外 + stash 未提交改动；force push 后从 stash index commit（9edf3024）恢复全部 untracked 新文件（22 个）+ 从备份恢复本地 material。
- **步骤 3-4 待用户执行**：material 团队私有分发 + CI env 注入 `HAP_STORE_PASSWORD`/`HAP_KEY_PASSWORD`（signing-passwords.ps1 已支持）。
- **GitHub fork/缓存待清除**：fork 仓库 + GitHub 对象缓存可能仍含旧 material，需联系 GitHub Support（support.github.com）请求清除敏感数据缓存 + 删除含旧历史的 fork。
- **backup-m1-filterrepo 分支**：保留重写前历史（含 material），建议确认无需恢复后 `git branch -D backup-m1-filterrepo` 删除 + `git reflog expire --expire=now --all && git gc --prune=now` 清本地残留对象。
- **本地 material**：从备份 `C:\Users\mu\Desktop\code\demo-material-backup` 恢复到 `tools/app/material/`（`.gitignore` 已忽略，不入 git）。
- 代码层 `signing-passwords.ps1` 已支持 env 注入（L4 修复）。
