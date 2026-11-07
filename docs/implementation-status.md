# Zero Music — 实施状态

对照 `docs/zero-music-implementation-plan.md`（源文件在参考项目
`/Users/haibo/workspace/labs/mygo-music/docs/`）记录当前进度。

## 已完成

### M0 后端复制（core/）

- 从 MyGo Music 复制 `music`、`plugin`、`player`、`local`、`lyrics`、`i18n`
  与 `examples/demo-plugin`，模块名改为 `zeromusic/core`，import 全部重写。
- `player` 去掉了对 `github.com/egoist/mygo` 的 `NowPlaying` 依赖，新增
  `MediaSession` 接口与 `NoopMedia`；设备上不接系统媒体控制。
- 插件限制按设备收紧：内存 32 MB、卡死判定 4 s、安装包 256 MB、解包 512 MB。
- `webSignIn` / `waitForRedirect` 不再注入 `host`；`spawn` 只在清单声明
  process 权限且用户授权后注入。
- 后端测试全绿：
  ```sh
  cd core
  go test ./...
  ```

### M1 core IPC 与本地播放

- 新增 `core/internal/ipc`：Unix socket、4 字节大端长度帧、8 MB 上限、
  请求/响应按 `id` 匹配、事件广播、客户端落后时丢事件而不拖住 core。
- 新增 `core/internal/core`：
  - `hello`（协议、版本、能力、320×170）
  - `sources.list` / `sources.setEnabled`
  - `library.folders` / `addFolder` / `removeFolder` / `rescan`
  - `player.playTracks` / `toggle` / `next` / `prev` / `seek` / `volume` /
    `shuffle` / `repeat` / `jump` / `state`
  - `settings.get` / `settings.set`
  - `session.get` / `session.clear`
  - `source.call`（本地与插件共用 `music.Source` 方法）
  - `plugin.settings` / `setSetting` / `signIn` / `signOut` / `account` /
    `grant`、`plugins.list`
  - `quit`
- 事件：`player.state`（变化时带队列，播放中每 500 ms 一次不带队列）、
  `sources.changed`、`plugin.prompt`、`toast`。
- 数据目录按 XDG 布局，设置/会话原子写（`.part` + rename，0600）。
- 启动恢复上次会话为暂停态；播放中每 2 s 落盘。
- `cmd/zero-music-core` 支持 `--socket`、XDG 目录覆盖、`--log`、`GOMEMLIMIT`
  180 MiB / `GOGC=40`、SIGINT/SIGTERM 清理。

### M2 插件源（已打通发现与播放）

- 启动时 `plugin.Discover($XDG_DATA_HOME/zero-music/plugins,
  .../plugin-data)`，写入 `sources.list`、`plugins.list`。
- `sources.setEnabled` 持久化到 `settings.json` 的 `disabledSources`。
- 演示插件可从 `core/examples/demo-plugin` 复制到插件目录，完成搜索与
  `file://` 流播放。
- 登录提示经 `plugin.prompt` 推给 UI。
- 尚未做：插件安装/更新/删除、GitHub 下载、图片服务、插件设置页。

### 验证记录

- `go test ./...`：全部通过。
- 真机同构桌面冒烟：本地 WAV 扫描、搜索、播放、暂停、音量、退出；
  demo 插件搜索、播放（`plugin demo: played t1`）。
- 冒烟用 Unix socket 客户端发送 `hello`、`sources.list`、`player.playTracks`、
  `player.state`、`quit`，core 正常停止并删除 socket。

## 下一步

1. **M2 剩余**：`plugin.install/update/remove`、GitHub `owner/repo` 安装、
   图片服务（fetch → RGB565 → 缓存）、`scan.progress`。
2. **M3 UI**：`src/ipc/client.{h,cpp}`（JSON + 线程 + 重连）、状态
   model/viewmodel、屏幕栈、Home / Now Playing、全局键位。
3. **M4**：本地库 Browse/Search/Detail/Lyrics/Queue。
4. **M5**：Plugins/Settings/Sign-in 页面。
5. **M6**：CMake 集成 Go core、wrapper、`.desktop`、`.deb`、截图、文档。

## 开发命令

```sh
# core 测试
cd core && go test ./...

# core 单独运行（仓库内 .run/）
./scripts/run-core.sh --dev

# 只构建 core
./scripts/build-core.sh

# 设备交叉
GOOS=linux GOARCH=arm64 ./scripts/build-core.sh
```

UI 尚未接入 core；桌面 UI 仍为 Template 示例，下一阶段替换。
