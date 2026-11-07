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

### M3 UI 基础（已开始）

- 新增 `src/ipc/client.{h,cpp}`：Unix socket、4 字节大端帧、非阻塞读写、
  JSON 请求/响应、事件回调、断线重连；网络线程与 LVGL 线程分离，`poll()`
  在 LVGL 线程投递回调。
- `src/model/base_model` 与 `src/viewmodel/base_viewmodel` 改为播放器状态：
  连接状态、源、曲目、进度、音量、循环、随机、页面栈；操作通过 IPC
  发送 `player.*`。
- 新页面：Home（菜单/源/连接状态）、Now Playing（封面占位、标题/艺人/
  专辑、进度、时间、两行键位提示）、其余页面先用 MessageScreen 占位。
- 全局键位：`4` Now Playing、`5` 上一首、`6`/Space 播放暂停、
  `7` 下一首、`8` 循环模式、短 Esc 返回、长 Esc 退出、Fn+H 帮助、
  Fn+J 截图。
- 桌面 SDL 构建已通过；与 core 真实连接并显示 demo 插件曲目。
- 新增开发钩子：`ZERO_MUSIC_SCREENSHOT`、`ZERO_MUSIC_SCREENSHOT_EXIT`、
  `ZERO_MUSIC_SCREENSHOT_DIR` 用于无头截图；`ZERO_MUSIC_START_PAGE=now-playing`
  可直接打开 Now Playing。
- 截图：[Home](../screenshot/zero-music-home.png) /
  [Now Playing](../screenshot/zero-music-now-playing.png)。

```sh
cmake -S . -B build/dev -G "Unix Makefiles" -DUSE_DESKTOP=ON \
  -DCMAKE_BUILD_TYPE=Debug -DFETCHCONTENT_BASE_DIR="$PWD/.cache/dev-deps"
cmake --build build/dev -j8
./build/dev/zero-music --socket .run/zero-music.sock
```

### M4 本地库（已开始）

- 新增 `src/view/widgets/track_list`：6 行焦点列表、上下移动、Enter 播放、
  滚动窗口、时长显示。
- Browse 页读取当前源的 `browse "songs"`，支持本地库与插件源。
- Search 页支持 ASCII 输入、Backspace、Enter 查询；结果用同一列表，
  Enter 播放整张结果队列并从选中项开始。
- 截图：[Browse](../screenshot/zero-music-browse.png) /
  [Search](../screenshot/zero-music-search.png)。

- Queue 页按播放顺序显示队列，Enter 跳转，Backspace 删除。
- Lyrics 页显示当前行及前后行，自动跟随播放，上下可手动滚动；core 在歌词
  加载后补发一次完整 `player.state`。
- Settings 页可调音量、源、音质、缓存、语言，并触发重扫。
- Plugins 页列出插件、Enter 启停、`G` 授权 process、`S` 登录、
  `E` 打开插件设置；设置值用 ASCII 键盘编辑，Enter 保存。
- Plugins 页 `I` 打开 GitHub 安装输入页，`A` 打开账户页（`X` 退出登录）。
- Sign-in 页用 LVGL 内置 QRCode 渲染 `plugin.prompt.qr`，文本来自
  `plugin.prompt.text`。
- core 新增 `player.remove`，以及 `plugin.install` / `update` / `remove`
  （GitHub `owner/repo`，后台下载，完成后重载源）。
- 截图：[Queue](../screenshot/zero-music-queue.png) /
  [Lyrics](../screenshot/zero-music-lyrics.png) /
  [Settings](../screenshot/zero-music-settings.png) /
  [Plugins](../screenshot/zero-music-plugins.png)。

- 本地扫描通过 `local.OnProgress` 发出 `scan.progress {done,total,path}`，
  UI 状态栏显示扫描进度，完成后显示 `Library ready`。
- core 新增 `image.get`：按 URL 抓取、解码、居中裁方、缩放到指定尺寸，
  返回 base64 的 RGB565；内存缓存 16 MB。Now Playing 用它显示真实封面。
  （协议里图片用 base64 JSON，未走二进制帧；尺寸小，先这样。）
- 截图更新为带封面的 Now Playing。

- Browse 增加 Songs/Albums/Artists/Playlists 入口；专辑、艺人、歌单列表
  通过 `browse albums|artists|playlists` 与 `album|artist|playlist` 拉取。
- Detail 页显示专辑/艺人/歌单名称与曲目，Enter 从选中曲目播放。
- 截图：[Albums](../screenshot/zero-music-albums.png) /
  [Detail](../screenshot/zero-music-detail.png)。

- Plugins 页 `I` 打开 GitHub 安装输入页，输入 `owner/repo` 后 Enter 安装；
  进度经 `plugin.install` 事件显示。
- 截图：[Install](../screenshot/zero-music-install.png) /
  [Plugin settings](../screenshot/zero-music-plugin-settings.png)。

- Plugins 页 `A` 打开账户页，显示登录名，`X` 退出登录。
- 截图：[Install](../screenshot/zero-music-install.png) /
  [Plugin settings](../screenshot/zero-music-plugin-settings.png)。

未做：真机验证。
v1 不做中文输入：只保证 ASCII 输入与 CJK 显示。

### M6 构建与打包（已开始）

- CMake 增加 `zero_music_core` 目标：按宿主/交叉平台设置 `GOOS/GOARCH`，
  输出 `build/<preset>/zero-music-core`；Go 缓存放在仓库 `.cache/`。
- UI 输出名改为 `zero-music-ui`；新增 `packaging/zero-music` wrapper：
  先起 core，再起 UI，UI 退出后停止 core，转发 SIGINT/SIGTERM。
- `cmake --install` 安装 `zero-music`、`zero-music-core`、`zero-music-ui`，
  `/etc/zero-music.conf`、桌面项和资源；systemd 服务默认关闭。
- 依赖加入 `libpulse0 | libasound2`、`ca-certificates`、`fonts-noto-cjk`；
  `.deb` 文件名改为 `zero-music_<version>_<rev>_<arch>.deb`。
- 许可证改为 AGPL-3.0，新增 `NOTICE`；store 名称/简介/分类/截图/网络权限
  已改为 Zero Music。
- 已验证：`cmake --build` 同时构建 UI 与 Go core；wrapper 启动/退出全流程
  正常，core 收到信号后删除 socket。

已验证 Go core 的 `GOOS=linux GOARCH=arm64` 交叉编译（`scripts/build-core.sh`）。
容器交叉构建：`packaging/cross/Dockerfile` + `packaging/cross/build.sh`
在 arm64 Ubuntu 容器内装 `aarch64-linux-gnu-gcc`、CMake 3.31、Go 1.27.3，
然后跑 `cp0-cross-package`。

已验证产物（2026-11-07）：

- `dist/zero-music_0.2.1_rev1_arm64.deb`，7.6 MB；
- `Architecture: arm64`，`Maintainer: H.B. Yu <mail@yuhaibo.com>`；
- 包内含 `/usr/bin/zero-music`（wrapper）、`zero-music-core`（aarch64 ELF）、
  `zero-music-ui`（aarch64 ELF）、`/etc/zero-music.conf`、APPLaunch 桌面项与
  图标、字体/音频资源、`LICENSE`/`NOTICE`；
- 依赖含 `libpulse0 | libasound2`、`ca-certificates`、`fonts-noto-cjk`；
- manifest + package 校验脚本通过。

## 下一步

1. **M5 完成**：插件设置、安装与账户页已可用。
2. **M2 剩余**：`scan.progress`、图片磁盘缓存/并发限制。
3. **M6 剩余**：cp0-cross 构建、`.deb` 实际打包与安装验证、
   APPLaunch 真机启动/退出、内存与帧率。

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
