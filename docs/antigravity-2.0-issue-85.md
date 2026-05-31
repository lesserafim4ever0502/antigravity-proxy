# Antigravity 2.0 失效问题汇总（Issue #85）

## 背景

GitHub Issue #85 反馈：升级到 Antigravity 2.0.0 后，原有代理配置失效。

Issue 链接：https://github.com/yuaotian/antigravity-proxy/issues/85

## 已确认的问题点

### 1. language server 进程改名

Issue 评论中确认，Antigravity 2.0 的语言服务器进程从旧命名：

- `language_server_windows_x64.exe`
- `language_server_windows.exe`

变为：

- `language_server.exe`

因此默认 `target_processes` 必须覆盖新版进程名，否则 `child_injection_mode=filtered` 时会跳过关键进程。

### 2. 必须保持子进程注入开启

有用户反馈将 `child_injection` 误设为 `false` 后可以登录，但 Agent 执行失败；恢复为 `true` 后可用。

推荐保留：

```json
"child_injection": true,
"child_injection_mode": "filtered"
```

### 3. Antigravity IDE 进程名带空格

Issue 中多份可用配置包含：

```json
"Antigravity IDE.exe"
```

旧代码按第一个空格截断 `CreateProcess` 命令行，可能把 `Antigravity IDE.exe` 截成 `Antigravity`，导致配置项无法命中。当前修复已改为优先识别引号包裹路径和 `.exe` 后缀。

### 4. 可能需要在多个安装目录放置补丁

有用户反馈 Antigravity 2.0 可能存在两个相关目录都需要放置 `version.dll` 与 `config.json` 的情况。当前 DLL 的行为是读取自身所在目录的 `config.json`，因此部署时应确保实际启动的 Antigravity/IDE/Agent 目录内都有对应文件。

## 当前推荐配置片段

```json
"child_injection": true,
"child_injection_mode": "filtered",
"child_injection_exclude": [],
"target_processes": [
  "language_server.exe",
  "language_server_windows",
  "Antigravity.exe",
  "Antigravity IDE.exe",
  "node.exe"
]
```

说明：

- `language_server.exe` 覆盖 Antigravity 2.0 新语言服务器。
- `language_server_windows` 保留旧版兼容，可匹配 `language_server_windows_x64.exe`。
- `Antigravity IDE.exe` 覆盖 2.0 评论中出现的新 IDE 进程名。
- `node.exe` 覆盖 language server 派生的关键子进程。

## 仍需继续排查的情况

如果日志已显示 DLL 加载成功、目标进程注入成功、SOCKS5 隧道建立成功，但 Agent 仍报：

```text
Agent execution terminated due to error.
FAILED_PRECONDITION (code 400): User location is not supported for the API use.
```

优先排查代理出口 IP、ASN、机房/托管网络特征，参考：

- `docs/agent-ip-diagnosis.md`

如果使用 sing-box、Clash、2rayN 等代理软件，Issue 中也有人反馈 TUN/sniff/rule 行为会影响结果。此类问题不一定是 DLL 注入失效。

## 验证建议

1. 查看 `proxy-YYYYMMDD.log` 是否出现 `language_server.exe`、`Antigravity IDE.exe` 或 `node.exe` 的注入日志。
2. 确认日志中 `child_injection=true` 且 `child_injection_mode=filtered`。
3. 确认实际启动目录里的 `config.json` 包含新版 `target_processes`。
4. 如果注入成功但 Agent 仍失败，继续按 `docs/agent-ip-diagnosis.md` 排查出口 IP。
