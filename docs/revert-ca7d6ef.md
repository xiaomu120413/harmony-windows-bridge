# 撤销审计整改提交

Change ID：CHG-20260916-002。状态：Verified（回退内容一致性）。

按用户要求，在独立分支`codex/revert-ca7d6ef`撤销提交`ca7d6ef0229bb91220b701e81f43a97cab4cc67a`，使用git revert保留历史，不修改main。

范围：目标提交的44个文件，包括会话断连/状态协调器、GPU拆分、Native生命周期与xrdp进程修改、签名构建配置、测试及配套设计文档。撤销后代码与目标提交父节点`e5c83ba`一致，仅增加本回退说明与CHANGELOG记录。既有顶部显示工具栏仍保留。

不重写历史，不恢复先前已从Git历史移除的签名材料。本地签名材料继续通过本地排除规则保护，不加入回退提交。验收为反向补丁无冲突、代码树与父节点一致、git diff --check通过；本任务不重新构建或部署设备。

实施结果：git revert --no-commit无冲突；补充本记录及CHANGELOG之前，`git diff --cached --exit-code ca7d6ef^ -- .`返回0，证明完整反向补丁与父节点一致；`git diff --cached --check`通过。未重跑构建、单元测试或真机测试，已生成的设备包也未改动。
